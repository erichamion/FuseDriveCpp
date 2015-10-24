/* 
 * File:   FileContents.cpp
 * Author: me
 * 
 * Created on October 23, 2015, 12:45 AM
 */

#include "FileContents.hpp"
#include "Gdrive.hpp"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <sstream>
#include <queue>

using namespace std;

namespace fusedrive
{

    FileContents& FileContents::gdrive_fcontents_add_new(CacheNode& cacheNode)
    {
        // Create the actual file contents struct.
        FileContents* pNew = new FileContents(cacheNode);

        // Find the last entry in the file contents list, and add the new one to
        // the end.
        FileContents** ppFromLast = cacheNode.getContentsListPtr();
        while (*ppFromLast)
        {
            ppFromLast = &(*ppFromLast)->pNext;
        }
        *ppFromLast = pNew;

        return *pNew;
    }
    
    FileContents::~FileContents()
    {
        // Find the pointer leading to this object
        FileContents** ppFromLast = cacheNode.getContentsListPtr();
        while (*ppFromLast != this)
        {
            assert(*ppFromLast && "Null pointer while deleting FileContents,"
                "this object is not in the list.");
            ppFromLast = &(*ppFromLast)->pNext;
        }
        
        // Move the next item in the list (or the end of the list) up the chain
        *ppFromLast = this->pNext;
        
        // Close the temp file
        if (fh != NULL)
        {
            fclose(fh);
        }
    }

    void FileContents::gdrive_fcontents_delete_after_offset(off_t offset)
    {
        // Container to store pointers to the chunks that need deleted.
        queue<FileContents*> deleteQueue;

        // Walk through the list of chunks and find the ones to delete
        FileContents* pCurrentContentNode = this;
        do
        {
            if (pCurrentContentNode->start > offset)
            {
                deleteQueue.push(pCurrentContentNode);
            }
            pCurrentContentNode = pCurrentContentNode->pNext;
        } while (pCurrentContentNode);

        // Delete each of the chunks. This can't be the most efficient way to do
        // this (we just walked through the whole list to find which chunks to 
        // delete, and now each call to gdrive_fcontents_delete() will walk through
        // the beginning of the list again). Doing it this way means we don't need
        // to worry about how to delete multiple consecutive chunks (how to make 
        // sure we're never stuck with just a pointer to a deleted node or without 
        // any valid pointers, unless all the chunks are deleted). There will never
        // be a very large number of chunks to go through (no more than 
        // gdrive_get_maxchunks()), and network delays should dwarf any processing
        // inefficiency.
        bool deleteThis = false;
        while (!deleteQueue.empty())
        {
            pCurrentContentNode = deleteQueue.front();
            deleteQueue.pop();
            if (pCurrentContentNode == this)
            {
                // If this object is deleting itself, do it last.
                deleteThis = true;
            }
            else
            {
                delete pCurrentContentNode;
            }
        }
        
        if (deleteThis)
        {
            delete this;
        }

    }

    void FileContents::gdrive_fcontents_free_all()
    {
        // Free the rest of the list after the current item.
        if (pNext)
        {
            delete pNext;
        }
        
        // Free this object
        delete this;
    }

    FileContents* FileContents::gdrive_fcontents_find_chunk(off_t offset)
    {
        if (offset >= start && offset <= end)
        {
            // Found it!
            return this;
        }
        
        if (offset == start && end < start)
        {
            // Found it in a zero-length chunk (probably a zero-length file)
            return this;
        }

        // It's not at this chunk.  Is there another chunk to try?
        if (pNext)
        {
            // Try the next node
            return pNext->gdrive_fcontents_find_chunk(offset);
        }
        else
        {
            // No more chunks to search
            return NULL;
        }
    }

    int FileContents::gdrive_fcontents_fill_chunk(const std::string& fileId, off_t start, 
        size_t size)
    {
        Gdrive& gInfo = cacheNode.getGdrive();
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(gInfo);
        if (pTransfer == NULL)
        {
            // Memory error
            return -1;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);

        // Construct the base URL in the form of "<GDRIVE_URL_FILES>/<fileId>".
        string fileUrl(Gdrive::GDRIVE_URL_FILES);
        fileUrl += "/";
        fileUrl += fileId;
        if (gdrive_xfer_set_url(pTransfer, fileUrl.c_str()) != 0)
        {
            // Error
            gdrive_xfer_free(pTransfer);
            return -1;
        }

        // Construct query parameters
        if (
                gdrive_xfer_add_query(gInfo, pTransfer, "updateViewedDate", "false") || 
                gdrive_xfer_add_query(gInfo, pTransfer, "alt", "media")
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
            return -1;
        }

        // Add the Range header.  Per 
        // http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.35 it is
        // fine for the end of the range to be past the end of the file, so we won't
        // worry about the file size.
        off_t end = start + size - 1;
        stringstream rangeHeader;
        rangeHeader << "Range: bytes=" << start << '-' << end;
        
        if (gdrive_xfer_add_header(pTransfer, rangeHeader.str().c_str()) != 0)
        {
            // Error
            gdrive_xfer_free(pTransfer);
            return -1;
        }

        // Set the destination file to the current chunk's handle
        gdrive_xfer_set_destfile(pTransfer, fh);

        // Make sure the file position is at the start and any stream errors are
        // cleared (this should be redundant, since we should normally have a newly
        // created and opened temporary file).
        rewind(fh);

        // Perform the transfer
        DownloadBuffer* pBuf = gdrive_xfer_execute(gInfo, pTransfer);
        gdrive_xfer_free(pTransfer);

        bool success = (pBuf != NULL && pBuf->getHttpResponse() < 400);
        delete pBuf;
        if (success)
        {
            this->start = start;
            this->end = start + size - 1;
            return 0;
        }
        // else failed
        return -1;
    }

    size_t FileContents::gdrive_fcontents_read(char* destBuf, off_t offset, size_t size)
    {
        // If given a NULL buffer pointer, just return the number of bytes that 
        // would have been read upon success.
        if (destBuf == NULL)
        {
            size_t maxSize = this->end - offset + 1;
            return (size > maxSize) ? maxSize : size;
        }

        // Read the data into the supplied buffer.
        FILE* chunkFile = this->fh;
        fseek(chunkFile, offset - this->start, SEEK_SET);
        size_t bytesRead = fread(destBuf, 1, size, chunkFile);

        // If an error occurred, return negative.
        if (bytesRead < size)
        {
            int err = ferror(chunkFile);
            if (err != 0)
            {
                rewind(chunkFile);
                return -err;
            }
        }

        // Return the number of bytes read (which may be less than size if we hit
        // EOF).
        return bytesRead;
    }

    off_t FileContents::gdrive_fcontents_write(const char* buf, off_t offset, 
        size_t size, bool extendChunk)
    {
        // Only write to the end of the chunk, unless extendChunk is true
        size_t maxSize = this->end - offset;
        size_t realSize = (extendChunk || size <= maxSize) ? size : maxSize;

        // Write the data from the supplied buffer.
        FILE* chunkFile = this->fh;
        fseek(chunkFile, offset - this->start, SEEK_SET);
        size_t bytesWritten = fwrite(buf, 1, size, chunkFile);

        // Extend the chunk's ending offset if needed
        if ((off_t) (offset + bytesWritten - 1) > this->end)
        {
            this->end = offset + bytesWritten - 1;
        }

        // If an error occurred, return negative.
        if (bytesWritten < realSize)
        {
            int err = ferror(chunkFile);
            if (err != 0)
            {
                rewind(chunkFile);
                return -err;
            }
        }

        // Return the number of bytes read (which may be less than size if we hit
        // end of chunk).
        return bytesWritten;
    }

    int FileContents::gdrive_fcontents_truncate(size_t size)
    {
        size_t newSize = size - this->start;
        // Truncate the underlying file
        if (ftruncate(fileno(this->fh), size - this->start) != 0)
        {
            // An error occurred.
            return -errno;
        }

        // If the truncate call extended the file, update the chunk size  to meet
        // the new size
        if ((this->end - this->start) < (off_t) newSize)
        {
            this->end = this->start + newSize - 1;
        }

        // Return success
        return 0;
    }

    FileContents::FileContents(CacheNode& cacheNode)
    : cacheNode(cacheNode)
    {
        start = 0;
        end = 0;
        fh = NULL;
        pNext = NULL;
        // Create a temporary file on disk.  This will automatically be deleted
        // when the file is closed or when this program terminates, so no 
        // cleanup is needed.
        fh = tmpfile();
        if (!fh)
        {
            // File creation error
            throw new exception();
        }
    }
    
}