/* 
 * File:   CacheNode.cpp
 * Author: me
 * 
 * Created on October 19, 2015, 11:11 AM
 */



#include "CacheNode.hpp"
#include "Cache.hpp"
#include "Gdrive.hpp"
#include "GdriveFile.hpp"
#include "Util.hpp"

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <sstream>


using namespace std;
namespace fusedrive
{
    CacheNode** gdrive_cache_get_head_ptr(Cache& cache)
    {
        return &cache.mpCacheHead;
    }

    CacheNode* CacheNode::gdrive_cnode_get(Cache& cache, CacheNode* pParent, 
            CacheNode** ppNode, const string& fileId, bool addIfDoesntExist, 
            bool& alreadyExists)
    {
        Gdrive& gInfo = cache.getGdrive();
        
        alreadyExists = false;

        if (*ppNode == NULL)
        {
            // Item doesn't exist in the cache. Either fail, or create a new item.
            if (!addIfDoesntExist)
            {
                // Not allowed to create a new item, return failure.
                return NULL;
            }
            // else create a new item.
            *ppNode = pParent ? new CacheNode(pParent) : new CacheNode(gInfo);
            
            // Convenience to avoid things like "return &((*ppNode)->fileinfo);"
            CacheNode* pNode = *ppNode;

            // Get the fileinfo
            stringstream url;
            url << Gdrive::GDRIVE_URL_FILES <<  "/" << fileId;
            Gdrive_Transfer* pTransfer = gdrive_xfer_create(gInfo);
            if (!pTransfer)
            {
                // Memory error
                return NULL;
            }
            if (gdrive_xfer_set_url(pTransfer, url.str().c_str()))
            {
                // Error, probably memory
                return NULL;
            }
            gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);
            Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(gInfo, pTransfer);
            gdrive_xfer_free(pTransfer);
            if (!pBuf || gdrive_dlbuf_get_httpresp(pBuf) >= 400)
            {
                // Download or request error
                free(pBuf);
                return NULL;
            }
            Json jsonObj(gdrive_dlbuf_get_data(pBuf));
            gdrive_dlbuf_free(pBuf);
            if (!jsonObj.gdrive_json_is_valid())
            {
                // Couldn't convert network response to JSON
                return NULL;
            }
            pNode->gdrive_cnode_update_from_json(jsonObj);

            return pNode;
        }

        // Convenience to avoid things like "&((*ppNode)->pRight)"
        CacheNode* pNode = *ppNode;

        // Root node exists, try to find the fileId in the tree.
        int cmp = fileId.compare(pNode->fileinfo.id);
        if (cmp == 0)
        {
            // Found it at the current node.
            alreadyExists = true;
            return pNode;
        }
        else if (cmp < 0)
        {
            // fileId is less than the current node. Look for it on the left.
            return gdrive_cnode_get(cache, pNode, &(pNode->pLeft), fileId, 
                    addIfDoesntExist, alreadyExists);
        }
        else
        {
            // fileId is greater than the current node. Look for it on the right.
            return gdrive_cnode_get(cache, pNode, &(pNode->pRight), fileId, 
                    addIfDoesntExist, alreadyExists);
        }
    }

    CacheNode* CacheNode::gdrive_cnode_get(Cache& cache, CacheNode* pParent, 
            CacheNode** ppNode, const string& fileId, bool addIfDoesntExist)
    {
        bool dummy = false;
        return gdrive_cnode_get(cache, pParent, ppNode, fileId, 
                addIfDoesntExist, dummy);
    }
    
    Gdrive& CacheNode::gdrive_cnode_get_gdrive()
    {
        return gInfo;
    }
    
    void CacheNode::gdrive_cnode_delete()
    {
        // The address of the pointer aimed at this node. If this is the root node,
        // then it will be a pointer passed in from outside. Otherwise, it is the
        // pLeft or pRight member of the parent.
        CacheNode** ppFromParent;
        if (pParent == NULL)
        {
            // This is the root. Take the pointer that was passed in.
            ppFromParent = gdrive_cache_get_head_ptr(mCache);
        }
        else
        {
            // Not the root. Find whether the node hangs from the left or right side
            // of its parent.
            assert(pParent->pLeft == this || 
                    pParent->pRight == this);
            ppFromParent = (pParent->pLeft == this) ? 
                &pParent->pLeft : &pParent->pRight;

        }

        // Simplest special case. pNode has no descendents.  Just delete it, and
        // set the pointer from the parent to NULL.
        if (pLeft == NULL && pRight == NULL)
        {
            *ppFromParent = NULL;
            delete this;
            return;
        }

        // Second special case. pNode has one side empty. Promote the descendent on
        // the other side into pNode's place.
        if (pLeft == NULL)
        {
            *ppFromParent = pRight;
            pRight->pParent = pParent;
            delete this;
            return;
        }
        if (pRight == NULL)
        {
            *ppFromParent = pLeft;
            pLeft->pParent = pParent;
            delete this;
            return;
        }

        // General case with descendents on both sides. Find the node with the 
        // closest value to pNode in one of its subtrees (leftmost node of the right
        // subtree, or rightmost node of the left subtree), and switch places with
        // pNode.  Which side we use doesn't really matter.  We'll rather 
        // arbitrarily decide to use the same side subtree as the side from which
        // pNode hangs off its parent (if pNode is on the right side of its parent,
        // find the leftmost node of the right subtree), and treat the case where
        // pNode is the root the same as if it were on the left side of its parent.
        CacheNode* pSwap = NULL;
        CacheNode** ppToSwap = NULL;
        if (pParent != NULL && pParent->pRight == this)
        {
            // Find the leftmost node of the right subtree.
            pSwap = pRight;
            ppToSwap = &pRight;
            while (pSwap->pLeft != NULL)
            {
                ppToSwap = &pSwap->pLeft;
                pSwap = pSwap->pLeft;
            }
        }
        else
        {
            // Find the rightmost node of the left subtree.
            pSwap = pLeft;
            ppToSwap = &pLeft;
            while (pSwap->pRight != NULL)
            {
                ppToSwap = &pSwap->pRight;
                pSwap = pSwap->pRight;
            }
        }

        // Swap the nodes
        gdrive_cnode_swap(ppFromParent, this, ppToSwap, pSwap);

        // Now delete the node from its new position.
        gdrive_cnode_delete();
    }

    void CacheNode::gdrive_cnode_mark_deleted()
    {
        deleted = true;
        if (openCount == 0)
        {
            gdrive_cnode_delete();
        }
    }

    bool CacheNode::gdrive_cnode_isdeleted()
{
    return deleted;
}
    
    void CacheNode::gdrive_cnode_free_all()
    {
        // Free all the descendents first.
        if (pLeft)
        {
            pLeft->gdrive_cnode_free_all();
        }
        if (pRight)
        {
            pRight->gdrive_cnode_free_all();
        }

        delete this;
    }

    time_t CacheNode::gdrive_cnode_get_update_time()
    {
        return lastUpdateTime;
    }

    enum Gdrive_Filetype CacheNode::gdrive_cnode_get_filetype()
    {
        return fileinfo.type;
    }

    Fileinfo& CacheNode::gdrive_cnode_get_fileinfo()
    {
        return fileinfo;
    }


    void CacheNode::gdrive_cnode_update_from_json(Json& jsonObj)
    {
        fileinfo.gdrive_finfo_cleanup();
        fileinfo.gdrive_finfo_read_json(jsonObj);

        // Mark the node as having been updated.
        lastUpdateTime = time(NULL);
    }

    void CacheNode::gdrive_cnode_delete_file_contents(Gdrive_File_Contents* pContentsToDelete)
    {
        gdrive_fcontents_delete(pContentsToDelete, &pContents);
    }


    bool CacheNode::gdrive_cnode_is_dirty()
    {
        return dirty;
    }
    
    void CacheNode::gdrive_cnode_set_dirty(bool val)
    {
        dirty = val;
    }
    
    int CacheNode::gdrive_cnode_get_open_count()
    {
        return openCount;
    }
        
    int CacheNode::gdrive_cnode_get_open_write_count()
    {
        return openWrites;
    }

    void CacheNode::gdrive_cnode_increment_open_count(bool isWrite)
    {
        openCount++;
        if (isWrite)
        {
            openWrites++;
        }
    }

    void CacheNode::gdrive_cnode_decrement_open_count(bool isWrite)
    {
        openCount--;
        if (isWrite)
        {
            openWrites--;
        }
    }
    
    bool CacheNode::gdrive_cnode_check_perm(int accessFlags)
    {
        // What permissions do we have?
        unsigned int perms = fileinfo.gdrive_finfo_real_perms();

        // What permissions do we need?
        unsigned int neededPerms = 0;
        // At least on my system, O_RDONLY is 0, which prevents testing for the
        // individual bit flag. On systems like mine, just assume we always need
        // read access. If there are other systems that have a different O_RDONLY
        // value, we'll test for the flag on those systems.
        if ((O_RDONLY == 0) || (accessFlags & O_RDONLY) || (accessFlags & O_RDWR))
        {
            neededPerms = neededPerms | S_IROTH;
        }
        if ((accessFlags & O_WRONLY) || (accessFlags & O_RDWR))
        {
            neededPerms = neededPerms | S_IWOTH;
        }

        // If there is anything we need but don't have, return false.
        return !(neededPerms & ~perms);
    }

    void CacheNode::gdrive_cnode_clear_contents()
    {
        gdrive_fcontents_free_all(&pContents);
        pContents = NULL;
    }
    
    bool CacheNode::gdrive_cnode_has_contents()
    {
        return (pContents != NULL);
    }
        
    Gdrive_File_Contents* CacheNode::gdrive_cnode_find_chunk(off_t offset)
    {
        if (!pContents)
        {
            // No contents to search, return failure
            return NULL;
        }
        
        return gdrive_fcontents_find_chunk(pContents, offset);
    }
    
    void CacheNode::gdrive_cnode_delete_contents_after_offset(off_t offset)
    {
        if (pContents)
        {
            return gdrive_fcontents_delete_after_offset(gInfo, &pContents, offset);
        }
    }
    
    CacheNode::CacheNode(CacheNode* pParent)
    : gInfo(pParent->gInfo), fileinfo(pParent->gInfo), 
            mCache(pParent->gInfo.gdrive_get_cache()), pParent(pParent)
    {
        gdrive_cnode_init();
    }
        
    CacheNode::CacheNode(Gdrive& gInfo)
    : gInfo(gInfo), fileinfo(gInfo), mCache(gInfo.gdrive_get_cache()), 
            pParent(NULL)
    {
        gdrive_cnode_init();
    }
        
    void CacheNode::gdrive_cnode_init()
    {
        lastUpdateTime = 0;
        openCount = 0;
        openWrites = 0;
        dirty = false;
        deleted = false;
        pContents = NULL;
        pLeft = NULL;
        pRight = NULL;
    }

    void CacheNode::gdrive_cnode_swap(CacheNode** ppFromParentOne, 
                                  CacheNode* pNodeOne, 
                                  CacheNode** ppFromParentTwo, 
                                  CacheNode* pNodeTwo)
    {
        // Make sure pNodeOne is not a descendent of pNodeTwo
        CacheNode* pParent = pNodeOne->pParent;
        while (pParent != NULL)
        {
            if (pParent == pNodeTwo)
            {
                // Reverse the order of the parameters
                gdrive_cnode_swap(ppFromParentTwo, pNodeTwo, ppFromParentOne, 
                        pNodeOne);
                return;
            }
            pParent = pParent->pParent;
        }

        CacheNode* pTempParent = pNodeOne->pParent;
        CacheNode* pTempLeft = pNodeOne->pLeft;
        CacheNode* pTempRight = pNodeOne->pRight;

        if (pNodeOne->pLeft == pNodeTwo || pNodeOne->pRight == pNodeTwo)
        {
            // Node Two is a direct child of Node One

            pNodeOne->pLeft = pNodeTwo->pLeft;
            pNodeOne->pRight = pNodeTwo->pRight;

            if (pTempLeft == pNodeTwo)
            {
                pNodeTwo->pLeft = pNodeOne;
                pNodeTwo->pRight = pTempRight;
            }
            else
            {
                pNodeTwo->pLeft = pTempLeft;
                pNodeTwo->pRight = pNodeOne;
            }

            // Don't touch *ppFromParentTwo - it's either pNodeOne->pLeft 
            // or pNodeOne->pRight.
        }
        else
        {
            // Not direct parent/child

            pNodeOne->pParent = pNodeTwo->pParent;
            pNodeOne->pLeft = pNodeTwo->pLeft;
            pNodeOne->pRight = pNodeTwo->pRight;

            pNodeTwo->pLeft = pTempLeft;
            pNodeTwo->pRight = pTempRight;

            *ppFromParentTwo = pNodeOne;
        }

        pNodeTwo->pParent = pTempParent;
        *ppFromParentOne = pNodeTwo;

        // Fix the pParent pointers in each of the descendents. If pNodeTwo was
        // originally a direct child of pNodeOne, this also updates pNodeOne's
        // pParent.
        if (pNodeOne->pLeft)
        {
            pNodeOne->pLeft->pParent = pNodeOne;
        }
        if (pNodeOne->pRight)
        {
            pNodeOne->pRight->pParent = pNodeOne;
        }
        if (pNodeTwo->pLeft)
        {
            pNodeTwo->pLeft->pParent = pNodeTwo;
        }
        if (pNodeTwo->pRight)
        {
            pNodeTwo->pRight->pParent = pNodeTwo; 
        }
    }


    Gdrive_File_Contents* CacheNode::gdrive_cnode_add_contents()
    {
        // Create the actual Gdrive_File_Contents struct, and add it to the existing
        // chain if there is one.
        Gdrive_File_Contents* pNewContents = gdrive_fcontents_add(pContents);
        if (pNewContents == NULL)
        {
            // Memory or file creation error
            return NULL;
        }

        // If there is no existing chain, point to the new struct as the start of a
        // new chain.
        if (pContents == NULL)
        {
            pContents = pNewContents;
        }


        return pNewContents;
    }

    Gdrive_File_Contents* CacheNode::gdrive_cnode_create_chunk(off_t offset, 
        size_t size, bool fillChunk)
    {
        // Get the normal chunk size for this file, the smallest multiple of
        // minChunkSize that results in maxChunks or fewer chunks. Avoid creating
        // a chunk of size 0 by forcing fileSize to be at least 1.
        size_t fileSize = (fileinfo.size > 0) ? fileinfo.size : 1;
        int maxChunks = gInfo.gdrive_get_maxchunks();
        size_t minChunkSize = gInfo.gdrive_get_minchunksize();

        size_t perfectChunkSize = Util::gdrive_divide_round_up(fileSize, maxChunks);
        size_t chunkSize = Util::gdrive_divide_round_up(perfectChunkSize, minChunkSize) * 
                minChunkSize;

        // The actual chunk may be a multiple of chunkSize.  A read that starts at
        // "offset" and is "size" bytes long should be within this single chunk.
        off_t chunkStart = (offset / chunkSize) * chunkSize;
        off_t chunkOffset = offset % chunkSize;
        off_t endChunkOffset = chunkOffset + size - 1;
        size_t realChunkSize = Util::gdrive_divide_round_up(endChunkOffset, chunkSize) * 
                chunkSize;

        Gdrive_File_Contents* pContents = gdrive_cnode_add_contents();
        if (pContents == NULL)
        {
            // Memory or file creation error
            return NULL;
        }

        if (fillChunk)
        {
            int success = gdrive_fcontents_fill_chunk(gInfo, pContents,
                    fileinfo.id.c_str(), chunkStart, realChunkSize);
            if (success != 0)
            {
                // Didn't write the file.  Clean up the new Gdrive_File_Contents 
                // struct
                gdrive_cnode_delete_file_contents(pContents);
                return NULL;
            }
        }
        // else we're not filling the chunk, do nothing

        // Success
        return pContents;
    }

    CacheNode::~CacheNode()
    {
        
    }

}