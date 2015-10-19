/* 
 * File:   FileidCacheNode.cpp
 * Author: me
 * 
 * Created on October 19, 2015, 8:49 AM
 */

#include "FileidCacheNode.hpp"

#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

namespace fusedrive
{
    FileidCacheNode::~FileidCacheNode()
    {
        
    }

    int FileidCacheNode::gdrive_fidnode_add(FileidCacheNode** ppHead, 
            const string& path, const string& fileId)
    {
        if (*ppHead == NULL)
        {
            // Add the current element as the first one in the list.
            *ppHead = new FileidCacheNode(path, fileId);
            return 0;
        }


        FileidCacheNode** ppFromPrev = ppHead;
        FileidCacheNode* pNext = *ppFromPrev;


        while (true)
        {
            // Find the string comparison.  If pNext is NULL, pretend pNext->path
            // is greater than path (we insert after pPrev in both cases).
            int cmp = (pNext != NULL) ? path.compare(pNext->path) : -1;

            if (cmp == 0)
            {
                // Item already exists, update it.
                return pNext->gdrive_fidnode_update_item(fileId);
            }
            else if (cmp < 0)
            {
                // Item doesn't exist yet, insert it between pPrev and pNext.
                FileidCacheNode* pNew = new FileidCacheNode(path, fileId);
                *ppFromPrev = pNew;
                pNew->pNext = pNext;
                return 0;
            }
            // else keep searching
            ppFromPrev = &((*ppFromPrev)->pNext);
            pNext = *ppFromPrev;
        }
    }

    void FileidCacheNode::gdrive_fidnode_remove_by_id(FileidCacheNode** ppHead, 
                                     const string& fileId)
    {
        // Need to walk through the whole list, since it's not keyed by fileId.
        FileidCacheNode** ppFromPrev = ppHead;
        FileidCacheNode* pNext = *ppHead;

        while (pNext != NULL)
        {

            // Compare the given fileId to the current one.
            int cmp = fileId.compare(pNext->fileId);

            if (cmp == 0)
            {
                // Found it!
                *ppFromPrev = pNext->pNext;
                delete pNext;

                // Don't exit here. Still need to keep searching, since one file ID 
                // can correspond to many paths.
            }
            else
            {
                // Move on to the next one
                ppFromPrev = &(*ppFromPrev)->pNext;
            }
            pNext = *ppFromPrev;
        }
    }

    void FileidCacheNode::gdrive_fidnode_clear_all()
    {
        // Free the rest of the chain.
        if (pNext != NULL)
        {
            pNext->gdrive_fidnode_clear_all();
        }

        // Free the head node.
        delete this;
    }


    time_t FileidCacheNode::gdrive_fidnode_get_lastupdatetime() const
    {
        return lastUpdateTime;
    }

    string FileidCacheNode::gdrive_fidnode_get_fileid() const
    {
        return fileId;
    }


    FileidCacheNode* FileidCacheNode::gdrive_fidnode_get_node(const string& path)
    {
        FileidCacheNode* pNode = this;

        while (pNode != NULL)
        {
            int cmp = path.compare(pNode->path);
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
            pNode = pNode->pNext;
        }

        // We've hit the end of the list without finding path.
        return NULL;
    }

//        FileidCacheNode* gdrive_fidnode_create(const char* filename, 
//                                                            const char* fileId)
    FileidCacheNode::FileidCacheNode(const string& filename, const string& fileId)
    : path(filename), fileId(fileId)
    {
        pNext = NULL;
        // Set the updated time.
        lastUpdateTime = time(NULL);
    }

    int FileidCacheNode::gdrive_fidnode_update_item(const string& updatedFileId)
    {
        // Update the time.
        lastUpdateTime = time(NULL);

        if (fileId.empty() || (updatedFileId.compare(fileId) != 0))
        {
            // Node doesn't have a fileId or the IDs don't match. Copy the new
            // fileId in.
            fileId.assign(updatedFileId);
            return 0;
        }
        // else the IDs already match.
        return 0;
    }

    

}

