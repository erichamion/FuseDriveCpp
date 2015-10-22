/* 
 * File:   Cache.cpp
 * Author: me
 * 
 * Created on October 18, 2015, 8:12 PM
 */

#include "Cache.hpp"
#include "Gdrive.hpp"
#include "CacheNode.hpp"

#include <string.h>
#include <assert.h>
#include <sstream>

using namespace std;
namespace fusedrive
{
    /**************************
    * Public Methods
    **************************/
    
    Cache::Cache(Gdrive& gInfo, time_t cacheTTL)
    : mGInfo(gInfo), mCacheTTL(cacheTTL)
    {
        mLastUpdateTime = 0;
        mNextChangeId = 0;
        mpCacheHead = NULL;
        mpFileIdCacheHead = 0;
        mInitialized = false;
    }
    
    void Cache::init()
    {
        // Prepare and send the network request
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(mGInfo);
        if (pTransfer == NULL)
        {
            // Memory error
            throw new bad_alloc();
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);
        if (
                gdrive_xfer_set_url(pTransfer, Gdrive::GDRIVE_URL_ABOUT.c_str()) || 
                gdrive_xfer_add_query(mGInfo, pTransfer, "includeSubscribed", "false") || 
                gdrive_xfer_add_query(mGInfo, pTransfer, "fields", "largestChangeId")
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
            throw new exception();
        }
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(mGInfo, pTransfer);
        gdrive_xfer_free(pTransfer);

        bool success = false;
        if (pBuf != NULL && gdrive_dlbuf_get_httpresp(pBuf) < 400)
        {
            // Response was good, try extracting the data.
            Json jsonObj(gdrive_dlbuf_get_data(pBuf));
            int newNextChangeId = 
                    jsonObj.getInt64("largestChangeId", true, 
                    success) + 1;
            if (success)
            {
                mNextChangeId = newNextChangeId;
            }
        }

        gdrive_dlbuf_free(pBuf);
        if (!success)
        {
            // Some error occurred.
            throw new exception();
        }
        
        mInitialized = true;
    }
    
    Gdrive& Cache::getGdrive() const
    {
        return mGInfo;
    }

    const FileIdCacheNode* Cache::getFileidCacheHead() const
    {
        assert(mInitialized);
        return mpFileIdCacheHead;
    }

    time_t Cache::getTTL() const
    {
        assert(mInitialized);
        return mCacheTTL;
    }

    time_t Cache::getLastUpdateTime() const
    {
        assert(mInitialized);
        return mLastUpdateTime;
    }

    int64_t Cache::getNextChangeId() const
    {
        assert(mInitialized);
        return mNextChangeId;
    }

    int Cache::UpdateIfStale()
    {
        assert(mInitialized);
        if (mLastUpdateTime + mCacheTTL < time(NULL))
        {
            return update();
        }

        return 0;
    }

    int Cache::update()
    {
        assert(mInitialized);
        // Convert the numeric largest change ID into a string
        stringstream changeIdStream;
        changeIdStream << mNextChangeId;

        // Prepare the request, using the string change ID, and send it
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(mGInfo);
        if (pTransfer == NULL)
        {
            // Memory error
            return -1;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);
        if (
                gdrive_xfer_set_url(pTransfer, Gdrive::GDRIVE_URL_CHANGES.c_str()) || 
                gdrive_xfer_add_query(mGInfo, pTransfer, "startChangeId", 
                                      changeIdStream.str().c_str()) || 
                gdrive_xfer_add_query(mGInfo, pTransfer, "includeSubscribed", "false")
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
        }
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(mGInfo,pTransfer);
        gdrive_xfer_free(pTransfer);


        int returnVal = -1;
        if (pBuf != NULL && gdrive_dlbuf_get_httpresp(pBuf) < 400)
        {
            // Response was good, try extracting the data.
            Json jsonObj(gdrive_dlbuf_get_data(pBuf));
            if (jsonObj.isValid())
            {
                // Update or remove cached data for each item in the "items" array.
                Json changeArray = 
                        jsonObj.getNestedObject("items");
                bool dummy;
                int arraySize = changeArray.getArrayLength(dummy);
                for (int i = 0; i < arraySize; i++)
                {
                    Json jsonItem(changeArray.arrayGet(i));
                    if (!jsonItem.isValid())
                    {
                        // Couldn't get this item, skip to the next one.
                        continue;
                    }
                    string fileId = jsonItem.getString("fileId");
                    if (fileId.empty())
                    {
                        // Couldn't get an ID for the changed file, skip to the
                        // next one.
                        continue;
                    }
                    
                    // We don't know whether the file has been renamed or moved,
                    // so remove it from the fileId cache.
                    FileIdCacheNode::removeFileIdNode(&mpFileIdCacheHead, fileId);

                    // Update the file metadata cache, but only if the file is not
                    // opened for writing with dirty data.
                    CacheNode* pCacheNode = CacheNode::retrieveNode(*this,
                            NULL, &mpCacheHead, fileId, false);
                    if (pCacheNode != NULL && !pCacheNode->isDirty())
                    {
                        // If this file was in the cache, update its information
                        Json jsonFile = 
                                jsonItem.getNestedObject("file");
                        pCacheNode->updateFromJson(jsonFile);
                    }
                    // else either not in the cache, or there is dirty data we don't
                    // want to overwrite.


                    // The file's parents may now have a different number of 
                    // children.  Remove the parents from the cache.
                    bool dummy;
                    int numParents = 
                        jsonItem.getArrayLength("file/parents", 
                            dummy);
                    for (int nParent = 0; nParent < numParents; nParent++)
                    {
                        // Get the fileId of the current parent in the array.
                        Json parentObj = 
                                jsonItem.arrayGet("file/parents", 
                                nParent);
                        string parentId = parentObj.isValid() ?
                            parentObj.getString("id") :
                            "";
                        if (!parentId.empty())
                        {
                            this->deleteId(parentId.c_str());
                        }
                    }

                }

                bool success = false;
                
                int64_t newNextChangeId = 
                        jsonObj.getInt64("largestChangeId", 
                        true, success) + 1;
                if (success)
                {
                    mNextChangeId = newNextChangeId;
                }
                
                returnVal = success ? 0 : -1;
            }
        }

        // Reset the last updated time
        mLastUpdateTime = time(NULL);

        gdrive_dlbuf_free(pBuf);
        return returnVal;
    }

    Fileinfo* Cache::getItem(const string& fileId, 
            bool addIfDoesntExist, bool& alreadyExists)
    {
        assert(mInitialized);
        // Get the existing node (or a new one) from the cache.
        CacheNode* pNode = CacheNode::retrieveNode(*this, NULL, 
                &mpCacheHead, fileId, addIfDoesntExist, alreadyExists);
        if (pNode == NULL)
        {
            // There was an error, or the node doesn't exist and we aren't allowed
            // to create a new one.
            return NULL;
        }

        // Test whether the cached information is too old.  Use last updated time
        // for either the individual node or the entire cache, whichever is newer.
        // If the node's update time is 0, always update it.
        time_t cacheUpdated = mLastUpdateTime;
        time_t nodeUpdated = pNode->getUpdateTime();
        time_t expireTime = (nodeUpdated > cacheUpdated ? 
            nodeUpdated : cacheUpdated) + mCacheTTL;
        if (expireTime < time(NULL) || nodeUpdated == (time_t) 0)
        {
            // Update the cache and try again.

            // Folder nodes may be deleted by cache updates, but regular file nodes
            // are safe.
            bool isFolder = (pNode->getFiletype() == 
                    GDRIVE_FILETYPE_FOLDER);

            update();

            return (isFolder ? 
                    getItem(fileId, addIfDoesntExist, alreadyExists) :
                    &pNode->getFileinfo());
        }

        // We have a good node that's not too old.
        return &pNode->getFileinfo();
    }
    
    Fileinfo* Cache::getItem(const string& fileId, 
            bool addIfDoesntExist)
    {
        bool dummy = false;
        return getItem(fileId, addIfDoesntExist, dummy);
    }
    
    const CacheNode* Cache::getHead() const
    {
        return mpCacheHead;
    }
        
    void Cache::setHead(CacheNode* pNewHead)
    {
        mpCacheHead = pNewHead;
    }

    int Cache::addFileid(const string& path, const string& fileId)
    {
        assert(mInitialized);
        return FileIdCacheNode::addFileIdNode(&mpFileIdCacheHead, path, fileId);
    }

    CacheNode* Cache::getNode(const string& fileId, 
            bool addIfDoesntExist, bool& alreadyExists)
    {
        assert(mInitialized);
        return CacheNode::retrieveNode(*this, NULL, &mpCacheHead, fileId,
                addIfDoesntExist, alreadyExists);
    }
    
    CacheNode* Cache::getNode(const string& fileId, 
            bool addIfDoesntExist)
    {
        bool dummy = false;
        return getNode(fileId, addIfDoesntExist, dummy);
    }

    string Cache::getFileid(const string& path)
    {
        assert(mInitialized);
        // Get the cached node if it exists.  If it doesn't exist, fail.
        if (!mpFileIdCacheHead)
        {
            // No cache to search
            return "";
        }
        FileIdCacheNode* pNode = 
                mpFileIdCacheHead->getNode(path);
        if (pNode == NULL)
        {
            // The path isn't cached.  Return null.
            return "";
        }

        // We have the cached item.  Test whether it's too old.  Use the last update
        // either of the entire cache, or of the individual item, whichever is
        // newer.
        time_t cacheUpdateTime = getLastUpdateTime();
        time_t nodeUpdateTime = pNode->getLastUpdateTime();
        time_t expireTime = ((nodeUpdateTime > cacheUpdateTime) ? 
            nodeUpdateTime : cacheUpdateTime) + mCacheTTL;
        if (time(NULL) > expireTime)
        {
            // Item is expired.  Check for updates and try again.
            update();
            return getFileid(path);
        }

        return pNode->getFileId();
    }

    void Cache::deleteId(const string& fileId)
    {
        assert(mInitialized);
        assert(!fileId.empty());

        // Remove the ID from the file Id cache
        FileIdCacheNode::removeFileIdNode(&mpFileIdCacheHead, fileId);

        // If the file isn't opened by anybody, delete it from the cache 
        // immediately. Otherwise, mark it for delete on close.

        // Find the node we want to remove.
        CacheNode* pNode = CacheNode::retrieveNode(*this, NULL, 
                &mpCacheHead, fileId, false);
        if (pNode == NULL)
        {
            // Didn't find it.  Do nothing.
            return;
        }
        pNode->markDeleted();
    }

    void Cache::deleteNode(CacheNode* pNode)
    {
        assert(mInitialized);
        pNode->deleteNode();
    }
    
    Cache::~Cache() {
        mpFileIdCacheHead->clearAll();
        mpCacheHead->freeAll();
    }
    
    
    /**************************
    * Private Methods
    **************************/
    
    void Cache::removeId(Gdrive& gInfo, const string& fileId)
    {
        // Find the node we want to remove.
        CacheNode* pNode = CacheNode::retrieveNode(*this, NULL, 
                &mpCacheHead, fileId, false);
        if (pNode == NULL)
        {
            // Didn't find it.  Do nothing.
            return;
        }


        pNode->deleteNode();
    }
    
    

    

}