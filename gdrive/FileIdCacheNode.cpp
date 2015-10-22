/* 
 * File:   FileidCacheNode.cpp
 * Author: me
 * 
 * Created on October 19, 2015, 8:49 AM
 */

#include "FileIdCacheNode.hpp"

#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

namespace fusedrive
{
    FileIdCacheNode::~FileIdCacheNode()
    {
        
    }

    int FileIdCacheNode::addFileIdNode(FileIdCacheNode** ppHead, 
            const string& path, const string& fileId)
    {
        if (*ppHead == NULL)
        {
            // Add the current element as the first one in the list.
            *ppHead = new FileIdCacheNode(path, fileId);
            return 0;
        }


        FileIdCacheNode** ppFromPrev = ppHead;
        FileIdCacheNode* pNext = *ppFromPrev;


        while (true)
        {
            // Find the string comparison.  If pNext is NULL, pretend pNext->path
            // is greater than path (we insert after pPrev in both cases).
            int cmp = (pNext != NULL) ? path.compare(pNext->mPath) : -1;

            if (cmp == 0)
            {
                // Item already exists, update it.
                return pNext->updateItem(fileId);
            }
            else if (cmp < 0)
            {
                // Item doesn't exist yet, insert it between pPrev and pNext.
                FileIdCacheNode* pNew = new FileIdCacheNode(path, fileId);
                *ppFromPrev = pNew;
                pNew->mpNext = pNext;
                return 0;
            }
            // else keep searching
            ppFromPrev = &((*ppFromPrev)->mpNext);
            pNext = *ppFromPrev;
        }
    }

    void FileIdCacheNode::removeFileIdNode(FileIdCacheNode** ppHead, 
                                     const string& fileId)
    {
        // Need to walk through the whole list, since it's not keyed by fileId.
        FileIdCacheNode** ppFromPrev = ppHead;
        FileIdCacheNode* pNext = *ppHead;

        while (pNext != NULL)
        {

            // Compare the given fileId to the current one.
            int cmp = fileId.compare(pNext->mFileId);

            if (cmp == 0)
            {
                // Found it!
                *ppFromPrev = pNext->mpNext;
                delete pNext;

                // Don't exit here. Still need to keep searching, since one file ID 
                // can correspond to many paths.
            }
            else
            {
                // Move on to the next one
                ppFromPrev = &(*ppFromPrev)->mpNext;
            }
            pNext = *ppFromPrev;
        }
    }

    void FileIdCacheNode::clearAll()
    {
        // Free the rest of the chain.
        if (mpNext != NULL)
        {
            mpNext->clearAll();
        }

        // Free the head node.
        delete this;
    }


    time_t FileIdCacheNode::getLastUpdateTime() const
    {
        return mLastUpdateTime;
    }

    string FileIdCacheNode::getFileId() const
    {
        return mFileId;
    }


    FileIdCacheNode* FileIdCacheNode::getNode(const string& path)
    {
        FileIdCacheNode* pNode = this;

        while (pNode != NULL)
        {
            int cmp = path.compare(pNode->mPath);
            if (cmp == 0)
            {
                // Found it!
                return pNode;
            }
            else if (cmp < 0)
            {
                // We've gone too far.  It's not here.
                return NULL;
            }
            // else keep searching
            pNode = pNode->mpNext;
        }

        // We've hit the end of the list without finding path.
        return NULL;
    }

//        FileidCacheNode* gdrive_fidnode_create(const char* filename, 
//                                                            const char* fileId)
    FileIdCacheNode::FileIdCacheNode(const string& filename, const string& fileId)
    : mPath(filename), mFileId(fileId)
    {
        mpNext = NULL;
        // Set the updated time.
        mLastUpdateTime = time(NULL);
    }

    int FileIdCacheNode::updateItem(const string& updatedFileId)
    {
        // Update the time.
        mLastUpdateTime = time(NULL);

        if (mFileId.empty() || (updatedFileId.compare(mFileId) != 0))
        {
            // Node doesn't have a fileId or the IDs don't match. Copy the new
            // fileId in.
            mFileId.assign(updatedFileId);
            return 0;
        }
        // else the IDs already match.
        return 0;
    }

    

}

