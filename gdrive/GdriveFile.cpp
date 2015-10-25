/* 
 * File:   File.cpp
 * Author: me
 * 
 * Created on October 19, 2015, 7:08 PM
 */

#include "GdriveFile.hpp"
#include "Gdrive.hpp"

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

    
    
    void GdriveFile::close(int flags)
    {
        if ((flags & O_WRONLY) || (flags & O_RDWR))
        {
            // Was opened for writing

            // Upload any changes back to Google Drive
            sync();
            syncMetadata();

            // Close the file
            mCacheNode.decrementOpenCount(true);
        }
        else
        {
            // Decrement open file counts.
            mCacheNode.decrementOpenCount(false);
        }


        // Get rid of any downloaded temp files if they aren't needed.
        if (mCacheNode.getOpenCount() == 0)
        {
            mCacheNode.clearContents();
            if (mCacheNode.isDeleted())
            {
                mGInfo.getCache().deleteNode(&mCacheNode);
            }
        }
        
        delete this;
    }

    int GdriveFile::read(char* buf, size_t size, off_t offset)
    {
        assert(offset >= (off_t) 0);
        // Make sure we have at least read access for the file.
        if (!mCacheNode.checkPermissions(O_RDONLY))
        {
            // Access error
            return -EACCES;
        }

        off_t nextOffset = offset;
        //off_t bufferOffset = 0;
        
        // Starting offset must be within the file
        size_t fileSize = getFileinfo().size;
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
            off_t bytesRead = readNextChunk(bufPos, nextOffset, 
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

    int GdriveFile::write(const char* buf, size_t size, off_t offset)
    {
        // Make sure we have read and write access for the file.
        if (!mCacheNode.checkPermissions(O_RDWR))
        {
            // Access error
            return -EACCES;
        }

        // Read any needed chunks into the cache.
        off_t readOffset = offset;
        size_t readSize = size;
        if (offset == (off_t) getFileinfo().size)
        {
            if (readOffset > 0)
            {
                readOffset--;
            }
            readSize++;
        }
        
        read(NULL, readSize, readOffset);

        off_t nextOffset = offset;
        off_t bufferOffset = 0;
        size_t bytesRemaining = size;

        while (bytesRemaining > 0)
        {
            off_t bytesWritten = writeNextChunk(
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

    int GdriveFile::truncate(off_t size)
    {
        Fileinfo& fileinfo = getFileinfo();
        /* 4 possible cases:
         *      A. size is current size
         *      B. size is 0 (and current size is non-zero)
         *      C. size is greater than current size
         *      D. size is less than current size (and non-zero)
         * A and B are special cases. C and D share some similarities with each 
         * other.
         */

        // Check for write permissions
        if (!mCacheNode.checkPermissions(O_RDWR))
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
            mCacheNode.clearContents();
            fileinfo.size = 0;
            mCacheNode.setDirty();
            return 0;
        }

        // Cases C and D: Identify the final chunk (or the chunk that will become 
        // final, make sure it is cached, and  truncate it. Afterward, set the 
        // file's length.
        
        
        FileContents* pFinalChunk = NULL;
        if (fileinfo.size < (size_t) size)
        {
            // File is being lengthened. The current final chunk will remain final.
            if (fileinfo.size > 0)
            {
                // If the file is non-zero length, read the last byte of the file to
                // cache it.
                if (nullRead(1, fileinfo.size - 1) < 0)
                {
                    // Read error
                    return -EIO;
                }

                // Grab the final chunk
                pFinalChunk = mCacheNode.findChunk(fileinfo.size - 1);
            }
            else
            {
                // The file is zero-length to begin with. If a chunk exists, use it,
                // but we'll probably need to create one.
                pFinalChunk = mCacheNode.hasContents() ?
                    mCacheNode.findChunk(0) :
                    mCacheNode.createChunk(0, size, false);
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
            if (nullRead(1, size - 1) < 0)
            {
                // Read error
                return -EIO;
            }

            // Grab the final chunk
            pFinalChunk = mCacheNode.findChunk(size - 1);

            // Delete any chunks past the new EOF
            mCacheNode.deleteContentsAfterOffset(size - 1);
        }

        // Make sure we received the final chunk
        if (pFinalChunk == NULL)
        {
            // Error
            return -EIO;
        }

        int returnVal = pFinalChunk->truncate(size);

        if (returnVal == 0)
        {
            // Successfully truncated the chunk. Update the file's size.
            fileinfo.size = size;
            mCacheNode.setDirty();
        }

        return returnVal;
    }

    int GdriveFile::sync()
    {
        if (!mCacheNode.isDirty())
        {
            // Nothing to do
            return 0;
        }

        // Check for write permissions
        if (!mCacheNode.checkPermissions(O_RDWR))
        {
            return -EACCES;
        }

        // Assemble the URL
        stringstream url;
        url << Gdrive::GDRIVE_URL_UPLOAD <<  "/" << getFileinfo().id;
        
        // Just using simple upload for now.
        // TODO: Consider using resumable upload, possibly only for large files.
        HttpTransfer xfer(mGInfo);
        
        int result =
            xfer.setRequestType(HttpTransfer::PUT)
                .setUrl(url.str())
                .addQuery("uploadType", "media")
                // Set upload callback
                .setUploadCallback(uploadCallback, this)
                .execute();
            
        int returnVal = (result != 0 || xfer.getHttpResponse() >= 400);
        if (returnVal == 0)
        {
            // Success. Clear the dirty flag
            mCacheNode.setDirty(false);
        }
        return returnVal;
    }

    int GdriveFile::syncMetadata()
    {
        Fileinfo& fileinfo = getFileinfo();
        if (!fileinfo.dirtyMetainfo)
        {
            // Nothing to sync, do nothing
            return 0;
        }

        // Check for write permissions
        if (!mCacheNode.checkPermissions(O_RDWR))
        {
            return -EACCES;
        }

        int error = 0;
        string dummy = syncMetadataOrCreate(mGInfo, &fileinfo, 
                "", "", (fileinfo.type == GDRIVE_FILETYPE_FOLDER), error);
        return error;
    }

    int GdriveFile::setAtime(const struct timespec* ts)
    {
        // Make sure we have write permission
        if (!mCacheNode.checkPermissions(O_RDWR))
        {
            return -EACCES;
        }

        return getFileinfo().setAtime(ts);
    }

    int GdriveFile::setMtime(const struct timespec* ts)
    {
        // Make sure we have write permission
        if (!mCacheNode.checkPermissions(O_RDWR))
        {
            return -EACCES;
        }

        return getFileinfo().setMtime(ts);
    }

    Fileinfo& GdriveFile::getFileinfo()
    {
        // No need to check permissions. This is just metadata, and metadata 
        // permissions were already needed just to access the filesystem and get a
        // filehandle in the first place.
        return mCacheNode.getFileinfo();
    }

    unsigned int GdriveFile::getPermissions()
    {
        return getFileinfo().getRealPermissions();
    }
    
    int GdriveFile::nullRead(size_t size, off_t offset)
    {
        return read(NULL, size, offset);
    }
    
    size_t GdriveFile::readNextChunk(char* buf, 
            off_t offset, size_t size)
    {
        // Do we already have a chunk that includes the starting point?
        FileContents* pChunkContents = mCacheNode.findChunk(offset);

        if (pChunkContents == NULL)
        {
            // Chunk doesn't exist, need to create and download it.
            pChunkContents = mCacheNode.createChunk(offset, size, true);

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
        return pChunkContents->read(buf, offset, size);
    }

    off_t GdriveFile::writeNextChunk(const char* buf, 
        off_t offset, size_t size)
    {
        Fileinfo& fileinfo = getFileinfo();
        // If the starting point is 1 byte past the end of the file, we'll extend 
        // the final chunk. Otherwise, we'll write to the end of the chunk and stop.
        bool extendChunk = (offset == (off_t) fileinfo.size);

        // Find the chunk that includes the starting point, or the last chunk if
        // the starting point is 1 byte past the end.
        off_t searchOffset = (extendChunk && offset > 0) ? offset - 1 : offset;
        FileContents* pChunkContents = mCacheNode.findChunk(searchOffset);

        if (pChunkContents == NULL)
        {
            // Chunk doesn't exist. This is an error unless the file size is 0.
            if (fileinfo.size == 0)
            {
                // File size is 0, and there is no existing chunk. Create one and 
                // try again.
                mCacheNode.createChunk(0, 1, false);
                pChunkContents = mCacheNode.findChunk(searchOffset);
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
        off_t bytesWritten = pChunkContents->write(buf, 
                offset, size, extendChunk);

        if (bytesWritten > 0)
        {
            // Mark the file as having been written
            mCacheNode.setDirty();

            if ((size_t)(offset + bytesWritten) > fileinfo.size)
            {
                // Update the file size
                fileinfo.size = offset + bytesWritten;
            }
        }

        return bytesWritten;
    }

    size_t GdriveFile::uploadCallback(Gdrive& gInfo, char* buffer, 
            off_t offset, size_t size, void* userdata)
    {
        // All we need to do is read from a Gdrive_File* file handle into a buffer.
        // We already know how to do exactly that.
        GdriveFile* pFile = (GdriveFile*) userdata;
        int returnVal = pFile->read(buffer, size, offset);
        return (returnVal >= 0) ? (size_t) returnVal: (size_t)(-1);
    }

    std::string 
    GdriveFile::syncMetadataOrCreate(Gdrive& gInfo, 
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
        if (!uploadResourceJson.isValid())
        {
            error = ENOMEM;
            return NULL;
        }
        uploadResourceJson.addString("title", pMyFileinfo->filename);
        if (pFileinfo == NULL)
        {
            // Only set parents when creating a new file
            Json parentsArray = 
                    uploadResourceJson.addNewArray("parents");
            if (!parentsArray.isValid())
            {
                error = ENOMEM;
                return NULL;
            }
            Json parentIdObj;
            parentIdObj.addString("id", parentId);
            parentsArray.arrayAppendObject(parentIdObj);
        }
        if (isFolder)
        {
            uploadResourceJson.addString("mimeType", 
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
        string timeString = pMyFileinfo->getAtimeString();
        if (!timeString.empty())
        {
            uploadResourceJson.addString("lastViewedByMeDate", 
                    timeString);
        }

        bool hasMtime = false;
        timeString = pMyFileinfo->getMtimeString();
        if (!timeString.empty())
        {
            uploadResourceJson.addString("modifiedDate", timeString);
            hasMtime = true;
        }

        // Convert the JSON into a string
        string uploadResourceStr = uploadResourceJson.toString(false);
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
        HttpTransfer xfer(gInfo);
        
        // URL, header, and updateViewedDate query parameter always get added. The 
        // setModifiedDate query parameter only gets set when hasMtime is true. Any 
        // of these can fail with an out of memory error (returning non-zero).
        if (hasMtime)
        {
            xfer.addQuery("setModifiedDate", "true");
        }
        int result =
            xfer.setUrl(url.str())
                .addHeader("Content-Type: application/json")
                .addQuery("updateViewedDate", "false")
                .setRequestType((pFileinfo != NULL) ? 
                    HttpTransfer::PATCH : 
                    HttpTransfer::POST)
                .setBody(uploadResourceStr)
                .execute();

        if (result != 0 || xfer.getHttpResponse() >= 400)
        {
            // Transfer was unsuccessful
            error = EIO;
            return NULL;
        }

        // Extract the file ID from the returned resource
        Json jsonObj(xfer.getData());
        if (!jsonObj.isValid())
        {
            // Either memory error, or couldn't convert the response to JSON.
            // More likely memory.
            error = ENOMEM;
            return NULL;
        }
        string fileId = jsonObj.getString("id");
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
    : mGInfo(cacheNode.getGdrive()), mCacheNode(cacheNode)
    {
        // No body needed
    }

    GdriveFile* openFileHelper(CacheNode& cacheNode)
    {
        return new GdriveFile(cacheNode);
    }    
    

}