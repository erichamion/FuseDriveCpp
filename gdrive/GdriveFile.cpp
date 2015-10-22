/* 
 * File:   File.cpp
 * Author: me
 * 
 * Created on October 19, 2015, 7:08 PM
 */

#include "GdriveFile.hpp"
#include "Gdrive.hpp"
#include "NullStream.hpp"

#include <assert.h>
#include <fcntl.h>
#include <sstream>

using namespace std;
namespace fusedrive
{
//    GdriveFile::GdriveFile(const GdriveFile& orig)
//    : gInfo(orig.gInfo), cacheNode(orig.cacheNode)
//    {
//        // No body
//    }
    
    GdriveFile::~GdriveFile()
    {
        // Nothing special needs to happen. Deleting the file handle does
        // NOT close the file or delete the CacheNode.
    }

    
    
    GdriveFile* GdriveFile::gdrive_file_open(Gdrive& gInfo, 
            const std::string& fileId, int flags, int& error)
    {
        assert(!fileId.empty());

        // Get the cache node from the cache if it exists.  If it doesn't exist,
        // don't make a node with an empty Gdrive_Fileinfo.  Instead, use 
        // gdrive_file_info_from_id() to create the node and fill out the struct, 
        // then try again to get the node.
        Cache& cache = gInfo.gdrive_get_cache();
        CacheNode* pNode;
        while ((pNode = cache.getNode(fileId, false)) 
                == NULL)
        {
            try
            {
                Fileinfo::gdrive_finfo_get_by_id(gInfo, fileId);
            }
            catch (const exception& e)
            {
                // Problem getting the file info.  Return failure.
                error = ENOENT;
                throw new exception();
            }
        }

        // If the file is deleted, existing filehandles will still work, but nobody
        // new can open it.
        if (pNode->gdrive_cnode_isdeleted())
        {
            error = ENOENT;
            throw new exception();
        }

        // Don't open directories, only regular files.
        if (pNode->gdrive_cnode_get_fileinfo().type == GDRIVE_FILETYPE_FOLDER)
        {
            // Return failure
            error = EISDIR;
            throw new exception();
        }


        if (!pNode->gdrive_cnode_check_perm(flags))
        {
            // Access error
            error = EACCES;
            throw new exception();
        }


        // Increment the open counter
        pNode->gdrive_cnode_increment_open_count((flags & O_WRONLY) || (flags & O_RDWR));

        // Return file handle containing the cache node
        return new GdriveFile(*pNode);
    }

    std::string GdriveFile::gdrive_file_new(Gdrive& gInfo, const std::string& path, 
        bool createFolder, int& error)
    {
        assert(!path.empty() && path[0] == '/');
            
        // Separate path into basename and parent folder.
        Gdrive_Path* pGpath = gdrive_path_create(path.c_str());
        if (pGpath == NULL)
        {
            // Memory error
            error = ENOMEM;
            return "";
        }
        const char* folderName = gdrive_path_get_dirname(pGpath);
        const char* filename = gdrive_path_get_basename(pGpath);

        // Check basename for validity (non-NULL, not a directory link such as "..")
        if (filename == NULL || filename[0] == '/' || 
                strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
        {
            error = EISDIR;
            gdrive_path_free(pGpath);
            return "";
        }

        // Check folder for validity (non-NULL, starts with '/', and is an existing
        // folder)
        if (folderName == NULL || folderName[0] != '/')
        {
            // Path wasn't in the form of an absolute path
            error = ENOTDIR;
            gdrive_path_free(pGpath);
            return "";
        }
        string parentId = gInfo.gdrive_filepath_to_id(folderName);
        if (parentId.empty())
        {
            // Folder doesn't exist
            error = ENOTDIR;
            gdrive_path_free(pGpath);
            return "";
        }
        CacheNode* pFolderNode = 
                gInfo.gdrive_get_cache().getNode(parentId, true);
        if (pFolderNode == NULL)
        {
            // Couldn't get a node for the parent folder
            error = EIO;
            gdrive_path_free(pGpath);
            return "";
        }
        const Fileinfo& folderinfo = pFolderNode->gdrive_cnode_get_fileinfo();
        if (folderinfo.type != GDRIVE_FILETYPE_FOLDER)
        {
            // Not an actual folder
            error = ENOTDIR;
            gdrive_path_free(pGpath);
            return "";
        }

        // Make sure we have write access to the folder
        if (!pFolderNode->gdrive_cnode_check_perm(O_WRONLY))
        {
            // Don't have the needed permission
            error = EACCES;
            gdrive_path_free(pGpath);
            return "";
        }


        string fileId = gdrive_file_sync_metadata_or_create(gInfo, NULL, 
                parentId, filename, createFolder, error);
        gdrive_path_free(pGpath);

        // TODO: See if gdrive_cache_add_fileid() can be modified to return a 
        // pointer to the cached ID (which is a new copy of the ID that was passed
        // in). This will avoid the need to look up the ID again after adding it,
        // and it will also help with multiple files that have identical paths.
        int result = 
            gInfo.gdrive_get_cache().addFileid(path, fileId);
        if (result != 0)
        {
            // Probably a memory error
            error = ENOMEM;
            return "";
        }

        return gInfo.gdrive_filepath_to_id(path);
    }

    void GdriveFile::gdrive_file_close(int flags)
    {
        if ((flags & O_WRONLY) || (flags & O_RDWR))
        {
            // Was opened for writing

            // Upload any changes back to Google Drive
            gdrive_file_sync();
            gdrive_file_sync_metadata();

            // Close the file
            cacheNode.gdrive_cnode_decrement_open_count(true);
        }
        else
        {
            // Decrement open file counts.
            cacheNode.gdrive_cnode_decrement_open_count(false);
        }


        // Get rid of any downloaded temp files if they aren't needed.
        if (cacheNode.gdrive_cnode_get_open_count() == 0)
        {
            cacheNode.gdrive_cnode_clear_contents();
            if (cacheNode.gdrive_cnode_isdeleted())
            {
                gInfo.gdrive_get_cache().deleteNode(&cacheNode);
            }
        }
        
        delete this;
    }

    int GdriveFile::gdrive_file_read(char* buf, size_t size, off_t offset)
    {
        assert(offset >= (off_t) 0);
        // Make sure we have at least read access for the file.
        if (!cacheNode.gdrive_cnode_check_perm(O_RDONLY))
        {
            // Access error
            return -EACCES;
        }

        off_t nextOffset = offset;
        //off_t bufferOffset = 0;
        
        // Starting offset must be within the file
        size_t fileSize = gdrive_file_get_info().size;
        if (offset >= (off_t) fileSize)
        {
            return 0;
        }

        // Don't read past the current file size
        size_t realSize = (size + offset <= fileSize) ? 
            size : fileSize - offset;

        size_t bytesRemaining = realSize;
        size_t bufferOffset = 0;
        while (bytesRemaining > 0)
        {
            // Read into the current position if we're given a real buffer, or pass
            // in NULL otherwise
            char* bufPos = (buf != NULL) ? buf + bufferOffset : NULL;
            off_t bytesRead = gdrive_file_read_next_chunk(bufPos, nextOffset, 
                    bytesRemaining);
            if (bytesRead < 0)
            {
                // Read error.  bytesRead is the negative error number
                return bytesRead;
            }
            if (bytesRead == 0)
            {
                // EOF. Return the total number of bytes actually read.
                //assert(realSize - bytesRemaining == 
                //        buf.str().length() - originalLength);
                return realSize - bytesRemaining;
            }
            nextOffset += bytesRead;
            bufferOffset += bytesRead;
            bytesRemaining -= bytesRead;
        }
        
        //assert (buf.str().length() - originalLength == realSize);
        return realSize;
    }

    int GdriveFile::gdrive_file_write(const char* buf, size_t size, off_t offset)
    {
        // Make sure we have read and write access for the file.
        if (!cacheNode.gdrive_cnode_check_perm(O_RDWR))
        {
            // Access error
            return -EACCES;
        }

        // Read any needed chunks into the cache.
        off_t readOffset = offset;
        size_t readSize = size;
        if (offset == (off_t) gdrive_file_get_info().size)
        {
            if (readOffset > 0)
            {
                readOffset--;
            }
            readSize++;
        }
        
        gdrive_file_read(NULL, readSize, readOffset);

        off_t nextOffset = offset;
        off_t bufferOffset = 0;
        size_t bytesRemaining = size;

        while (bytesRemaining > 0)
        {
            off_t bytesWritten = gdrive_file_write_next_chunk(
                    buf + bufferOffset, nextOffset, bytesRemaining);
            if (bytesWritten < 0)
            {
                // Write error.  bytesWritten is the negative error number
                return bytesWritten;
            }
            nextOffset += bytesWritten;
            bufferOffset += bytesWritten;
            bytesRemaining -= bytesWritten;
        }

        return size;
    }

    int GdriveFile::gdrive_file_truncate(off_t size)
    {
        Fileinfo& fileinfo = gdrive_file_get_info();
        /* 4 possible cases:
         *      A. size is current size
         *      B. size is 0 (and current size is non-zero)
         *      C. size is greater than current size
         *      D. size is less than current size (and non-zero)
         * A and B are special cases. C and D share some similarities with each 
         * other.
         */

        // Check for write permissions
        if (!cacheNode.gdrive_cnode_check_perm(O_RDWR))
        {
            return -EACCES;
        }

        // Case A: Do nothing, return success.
        if (fileinfo.size == (size_t) size)
        {
            return 0;
        }

        // Case B: Delete all cached file contents, set the length to 0.
        if (size == 0)
        {
            cacheNode.gdrive_cnode_clear_contents();
            fileinfo.size = 0;
            cacheNode.gdrive_cnode_set_dirty();
            return 0;
        }

        // Cases C and D: Identify the final chunk (or the chunk that will become 
        // final, make sure it is cached, and  truncate it. Afterward, set the 
        // file's length.
        
        
        Gdrive_File_Contents* pFinalChunk = NULL;
        if (fileinfo.size < (size_t) size)
        {
            // File is being lengthened. The current final chunk will remain final.
            if (fileinfo.size > 0)
            {
                // If the file is non-zero length, read the last byte of the file to
                // cache it.
                if (gdrive_file_null_read(1, fileinfo.size - 1) < 0)
                {
                    // Read error
                    return -EIO;
                }

                // Grab the final chunk
                pFinalChunk = 
                        cacheNode.gdrive_cnode_find_chunk(fileinfo.size - 1);
            }
            else
            {
                // The file is zero-length to begin with. If a chunk exists, use it,
                // but we'll probably need to create one.
                pFinalChunk = cacheNode.gdrive_cnode_has_contents() ?
                    cacheNode.gdrive_cnode_find_chunk(0) :
                    cacheNode.gdrive_cnode_create_chunk(0, size, false);
//                if ((pFinalChunk = gdrive_fcontents_find_chunk(fh->pContents, 0))
//                        == NULL)
//                {
//                    pFinalChunk = gdrive_fcontents_create_chunk(gInfo, fh, 0, size, false);
//                }
            }
        }
        else
        {
            // File is being shortened.

            // The (new) final chunk is the one that contains what will become the
            // last byte of the truncated file. Read this byte in order to cache the
            // chunk.
            if (gdrive_file_null_read(1, size - 1) < 0)
            {
                // Read error
                return -EIO;
            }

            // Grab the final chunk
            pFinalChunk = cacheNode.gdrive_cnode_find_chunk(size - 1);

            // Delete any chunks past the new EOF
            cacheNode.gdrive_cnode_delete_contents_after_offset(size - 1);
        }

        // Make sure we received the final chunk
        if (pFinalChunk == NULL)
        {
            // Error
            return -EIO;
        }

        int returnVal = gdrive_fcontents_truncate(pFinalChunk, size);

        if (returnVal == 0)
        {
            // Successfully truncated the chunk. Update the file's size.
            fileinfo.size = size;
            cacheNode.gdrive_cnode_set_dirty();
        }

        return returnVal;
    }

    int GdriveFile::gdrive_file_sync()
    {
        if (!cacheNode.gdrive_cnode_is_dirty())
        {
            // Nothing to do
            return 0;
        }

        // Check for write permissions
        if (!cacheNode.gdrive_cnode_check_perm(O_RDWR))
        {
            return -EACCES;
        }

        // Just using simple upload for now.
        // TODO: Consider using resumable upload, possibly only for large files.
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(gInfo);
        if (pTransfer == NULL)
        {
            // Memory error
            return -ENOMEM;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_PUT);

        // Assemble the URL
        stringstream url;
        url << Gdrive::GDRIVE_URL_UPLOAD <<  "/" << gdrive_file_get_info().id;
        if (gdrive_xfer_set_url(pTransfer, url.str().c_str()) != 0)
        {
            // Error, probably memory
            gdrive_xfer_free(pTransfer);
            return -ENOMEM;
        }

        // Add query parameter(s)
        if (gdrive_xfer_add_query(gInfo, pTransfer, "uploadType", "media") != 0)
        {
            // Error, probably memory
            gdrive_xfer_free(pTransfer);
            return -ENOMEM;
        }

        // Set upload callback
        gdrive_xfer_set_uploadcallback(pTransfer, gdrive_file_uploadcallback, this);

        // Do the transfer
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(gInfo, pTransfer);
        gdrive_xfer_free(pTransfer);
        int returnVal = (pBuf == NULL || gdrive_dlbuf_get_httpresp(pBuf) >= 400);
        if (returnVal == 0)
        {
            // Success. Clear the dirty flag
            cacheNode.gdrive_cnode_set_dirty(false);
        }
        gdrive_dlbuf_free(pBuf);
        return returnVal;
    }

    int GdriveFile::gdrive_file_sync_metadata()
    {
        Fileinfo& fileinfo = gdrive_file_get_info();
        if (!fileinfo.dirtyMetainfo)
        {
            // Nothing to sync, do nothing
            return 0;
        }

        // Check for write permissions
        if (!cacheNode.gdrive_cnode_check_perm(O_RDWR))
        {
            return -EACCES;
        }

        int error = 0;
        string dummy = gdrive_file_sync_metadata_or_create(gInfo, &fileinfo, 
                "", "", (fileinfo.type == GDRIVE_FILETYPE_FOLDER), error);
        return error;
    }

    int GdriveFile::gdrive_file_set_atime(const struct timespec* ts)
    {
        // Make sure we have write permission
        if (!cacheNode.gdrive_cnode_check_perm(O_RDWR))
        {
            return -EACCES;
        }

        return gdrive_file_get_info().gdrive_finfo_set_atime(ts);
    }

    int GdriveFile::gdrive_file_set_mtime(const struct timespec* ts)
    {
        // Make sure we have write permission
        if (!cacheNode.gdrive_cnode_check_perm(O_RDWR))
        {
            return -EACCES;
        }

        return gdrive_file_get_info().gdrive_finfo_set_mtime(ts);
    }

    Fileinfo& GdriveFile::gdrive_file_get_info()
    {
        // No need to check permissions. This is just metadata, and metadata 
        // permissions were already needed just to access the filesystem and get a
        // filehandle in the first place.
        return cacheNode.gdrive_cnode_get_fileinfo();
    }

    unsigned int GdriveFile::gdrive_file_get_perms()
    {
        return gdrive_file_get_info().gdrive_finfo_real_perms();
    }
    
    int GdriveFile::gdrive_file_null_read(size_t size, off_t offset)
    {
        return gdrive_file_read(NULL, size, offset);
    }
    
    size_t GdriveFile::gdrive_file_read_next_chunk(char* buf, 
            off_t offset, size_t size)
    {
        // Do we already have a chunk that includes the starting point?
        Gdrive_File_Contents* pChunkContents = 
                cacheNode.gdrive_cnode_find_chunk(offset);

        if (pChunkContents == NULL)
        {
            // Chunk doesn't exist, need to create and download it.
            pChunkContents = cacheNode.gdrive_cnode_create_chunk(offset, size, true);

            if (pChunkContents == NULL)
            {
                // Error creating the chunk
                // TODO: size_t is (or should be) unsigned. Rather than returning
                // a negative value for error, we should probably return 0 and add
                // a parameter for a pointer to an error value.
                return -EIO;
            }
        }

        // Actually read to the buffer and return the number of bytes read (which
        // may be less than size if we hit the end of the chunk), or return any 
        // error up to the caller.
        return gdrive_fcontents_read(pChunkContents, buf, offset, size);
    }

    off_t GdriveFile::gdrive_file_write_next_chunk(const char* buf, 
        off_t offset, size_t size)
    {
        Fileinfo& fileinfo = gdrive_file_get_info();
        // If the starting point is 1 byte past the end of the file, we'll extend 
        // the final chunk. Otherwise, we'll write to the end of the chunk and stop.
        bool extendChunk = (offset == (off_t) fileinfo.size);

        // Find the chunk that includes the starting point, or the last chunk if
        // the starting point is 1 byte past the end.
        off_t searchOffset = (extendChunk && offset > 0) ? offset - 1 : offset;
        Gdrive_File_Contents* pChunkContents = 
                cacheNode.gdrive_cnode_find_chunk(searchOffset);

        if (pChunkContents == NULL)
        {
            // Chunk doesn't exist. This is an error unless the file size is 0.
            if (fileinfo.size == 0)
            {
                // File size is 0, and there is no existing chunk. Create one and 
                // try again.
                cacheNode.gdrive_cnode_create_chunk(0, 1, false);
                pChunkContents = 
                        cacheNode.gdrive_cnode_find_chunk(searchOffset);
            }
        }
        if (pChunkContents == NULL)
        {
            // Chunk still doesn't exist, return error.
            // TODO: size_t is (or should be) unsigned. Rather than returning
            // a negative value for error, we should probably return 0 and add
            // a parameter for a pointer to an error value.
            return -EINVAL;
        }

        // Actually write to the buffer and return the number of bytes read (which
        // may be less than size if we hit the end of the chunk), or return any 
        // error up to the caller.
        off_t bytesWritten = gdrive_fcontents_write(pChunkContents, buf, 
                offset, size, extendChunk);

        if (bytesWritten > 0)
        {
            // Mark the file as having been written
            cacheNode.gdrive_cnode_set_dirty();

            if ((size_t)(offset + bytesWritten) > fileinfo.size)
            {
                // Update the file size
                fileinfo.size = offset + bytesWritten;
            }
        }

        return bytesWritten;
    }

    size_t GdriveFile::gdrive_file_uploadcallback(Gdrive& gInfo, char* buffer, 
            off_t offset, size_t size, void* userdata)
    {
        // All we need to do is read from a Gdrive_File* file handle into a buffer.
        // We already know how to do exactly that.
        GdriveFile* pFile = (GdriveFile*) userdata;
        int returnVal = pFile->gdrive_file_read(buffer, size, offset);
        return (returnVal >= 0) ? (size_t) returnVal: (size_t)(-1);
    }

    std::string 
    GdriveFile::gdrive_file_sync_metadata_or_create(Gdrive& gInfo, 
            Fileinfo* pFileinfo,const std::string& parentId, 
            const std::string& filename, bool isFolder, int& error)
    {
        // For existing file, pFileinfo must be non-NULL. For creating new file,
        // both parentId and filename must be non-empty.
        assert(pFileinfo || !(parentId.empty() || filename.empty()));

        Fileinfo myFileinfo(gInfo);
        Fileinfo* pMyFileinfo;
        if (pFileinfo != NULL)
        {
            pMyFileinfo = pFileinfo;
            isFolder = (pMyFileinfo->type == GDRIVE_FILETYPE_FOLDER);
        }
        else
        {
            myFileinfo.filename = filename;
            myFileinfo.type = isFolder ? 
                GDRIVE_FILETYPE_FOLDER : GDRIVE_FILETYPE_FILE;
            struct timespec ts;
            if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
            {
                myFileinfo.creationTime = ts;
                myFileinfo.accessTime = ts;
                myFileinfo.modificationTime = ts;
            }
            // else leave the times at 0 on failure

            pMyFileinfo = &myFileinfo;
        }


        // Set up the file resource as a JSON object
        Json uploadResourceJson;
        if (!uploadResourceJson.gdrive_json_is_valid())
        {
            error = ENOMEM;
            return NULL;
        }
        uploadResourceJson.gdrive_json_add_string("title", pMyFileinfo->filename);
        if (pFileinfo == NULL)
        {
            // Only set parents when creating a new file
            Json parentsArray = 
                    uploadResourceJson.gdrive_json_add_new_array("parents");
            if (!parentsArray.gdrive_json_is_valid())
            {
                error = ENOMEM;
                return NULL;
            }
            Json parentIdObj;
            parentIdObj.gdrive_json_add_string("id", parentId);
            parentsArray.gdrive_json_array_append_object(parentIdObj);
        }
        if (isFolder)
        {
            uploadResourceJson.gdrive_json_add_string("mimeType", 
                    "application/vnd.google-apps.folder");
        }
    //    char* timeString = (char*) malloc(Fileinfo::GDRIVE_TIMESTRING_LENGTH);
    //    if (timeString == NULL)
    //    {
    //        // Memory error
    //        gdrive_json_kill(uploadResourceJson);
    //        *pError = ENOMEM;
    //        return NULL;
    //    }
    //    // Reuse the same timeString for atime and mtime. Can't change ctime.
        string timeString = pMyFileinfo->gdrive_finfo_get_atime_string();
        if (!timeString.empty())
        {
            uploadResourceJson.gdrive_json_add_string("lastViewedByMeDate", 
                    timeString);
        }

        bool hasMtime = false;
        timeString = pMyFileinfo->gdrive_finfo_get_mtime_string();
        if (!timeString.empty())
        {
            uploadResourceJson.gdrive_json_add_string("modifiedDate", timeString);
            hasMtime = true;
        }

        // Convert the JSON into a string
        string uploadResourceStr = uploadResourceJson.gdrive_json_to_string(false);
        if (uploadResourceStr.empty())
        {
            error = ENOMEM;
            return NULL;
        }


        // Full URL has '/' and the file ID appended for an existing file, or just
        // the base URL for a new one.
        stringstream url;
        url << Gdrive::GDRIVE_URL_FILES;
        if (pFileinfo)
        {
            // Existing file, need base URL + '/' + file ID
            url << '/' << pMyFileinfo->id;
        }

        // Set up the network request
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(gInfo);
        if (pTransfer == NULL)
        {
            error = ENOMEM;
            gdrive_xfer_free(pTransfer);
            return NULL;
        }
        // URL, header, and updateViewedDate query parameter always get added. The 
        // setModifiedDate query parameter only gets set when hasMtime is true. Any 
        // of these can fail with an out of memory error (returning non-zero).
        if ((gdrive_xfer_set_url(pTransfer, url.str().c_str()) || 
                gdrive_xfer_add_header(pTransfer, "Content-Type: application/json"))
                || 
                (hasMtime && 
                gdrive_xfer_add_query(gInfo, pTransfer, "setModifiedDate", "true")) || 
                gdrive_xfer_add_query(gInfo, pTransfer, "updateViewedDate", "false")
            )
        {
            error = ENOMEM;
            gdrive_xfer_free(pTransfer);
            return NULL;
        }
        gdrive_xfer_set_requesttype(pTransfer, (pFileinfo != NULL) ? 
            GDRIVE_REQUEST_PATCH : GDRIVE_REQUEST_POST);
        gdrive_xfer_set_body(pTransfer, uploadResourceStr.c_str());

        // Do the transfer
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(gInfo, pTransfer);
        gdrive_xfer_free(pTransfer);

        if (pBuf == NULL || gdrive_dlbuf_get_httpresp(pBuf) >= 400)
        {
            // Transfer was unsuccessful
            error = EIO;
            gdrive_dlbuf_free(pBuf);
            return NULL;
        }

        // Extract the file ID from the returned resource
        Json jsonObj(gdrive_dlbuf_get_data(pBuf));
        gdrive_dlbuf_free(pBuf);
        if (!jsonObj.gdrive_json_is_valid())
        {
            // Either memory error, or couldn't convert the response to JSON.
            // More likely memory.
            error = ENOMEM;
            return NULL;
        }
        string fileId = jsonObj.gdrive_json_get_string("id");
        if (fileId.empty())
        {
            // Either memory error, or couldn't extract the desired string, can't
            // tell which.
            error = EIO;
            return NULL;
        }

        pMyFileinfo->dirtyMetainfo = false;
        return fileId;
    }
    
    
    

    GdriveFile::GdriveFile(CacheNode& cacheNode)
    : gInfo(cacheNode.gdrive_cnode_get_gdrive()), cacheNode(cacheNode)
    {
        // No body needed
    }
    
    

}