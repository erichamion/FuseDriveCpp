/* 
 * File:   Cache.cpp
 * Author: me
 * 
 * Created on October 18, 2015, 8:12 PM
 */

#include "Cache.hpp"
#include "Gdrive.hpp"
#include "gdrive-cache-node.hpp"

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
    : gInfo(gInfo), cacheTTL(cacheTTL)
    {
        lastUpdateTime = 0;
        nextChangeId = 0;
        pCacheHead = NULL;
        pFileIdCacheHead = 0;
        initialized = false;
    }
    
    void Cache::gdrive_cache_init()
    {
        // Prepare and send the network request
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(gInfo);
        if (pTransfer == NULL)
        {
            // Memory error
            throw new bad_alloc();
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);
        if (
                gdrive_xfer_set_url(pTransfer, Gdrive::GDRIVE_URL_ABOUT.c_str()) || 
                gdrive_xfer_add_query(gInfo, pTransfer, "includeSubscribed", "false") || 
                gdrive_xfer_add_query(gInfo, pTransfer, "fields", "largestChangeId")
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
            throw new exception();
        }
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(gInfo, pTransfer);
        gdrive_xfer_free(pTransfer);

        bool success = false;
        if (pBuf != NULL && gdrive_dlbuf_get_httpresp(pBuf) < 400)
        {
            // Response was good, try extracting the data.
            Json jsonObj(gdrive_dlbuf_get_data(pBuf));
            int newNextChangeId = 
                    jsonObj.gdrive_json_get_int64("largestChangeId", true, 
                    success) + 1;
            if (success)
            {
                nextChangeId = newNextChangeId;
            }
        }

        gdrive_dlbuf_free(pBuf);
        if (!success)
        {
            // Some error occurred.
            throw new exception();
        }
        
        initialized = true;
    }

    FileidCacheNode* Cache::gdrive_cache_get_fileidcachehead()
    {
        assert(initialized);
        return pFileIdCacheHead;
    }

    time_t Cache::gdrive_cache_get_ttl()
    {
        assert(initialized);
        return cacheTTL;
    }

    time_t Cache::gdrive_cache_get_lastupdatetime()
    {
        assert(initialized);
        return lastUpdateTime;
    }

    int64_t Cache::gdrive_cache_get_nextchangeid()
    {
        assert(initialized);
        return nextChangeId;
    }

    int Cache::gdrive_cache_update_if_stale()
    {
        assert(initialized);
        if (lastUpdateTime + cacheTTL < time(NULL))
        {
            return gdrive_cache_update();
        }

        return 0;
    }

    int Cache::gdrive_cache_update()
    {
        assert(initialized);
        // Convert the numeric largest change ID into a string
        stringstream changeIdStream;
        changeIdStream << nextChangeId;

        // Prepare the request, using the string change ID, and send it
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(gInfo);
        if (pTransfer == NULL)
        {
            // Memory error
            return -1;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);
        if (
                gdrive_xfer_set_url(pTransfer, Gdrive::GDRIVE_URL_CHANGES.c_str()) || 
                gdrive_xfer_add_query(gInfo, pTransfer, "startChangeId", 
                                      changeIdStream.str().c_str()) || 
                gdrive_xfer_add_query(gInfo, pTransfer, "includeSubscribed", "false")
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
        }
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(gInfo,pTransfer);
        gdrive_xfer_free(pTransfer);


        int returnVal = -1;
        if (pBuf != NULL && gdrive_dlbuf_get_httpresp(pBuf) < 400)
        {
            // Response was good, try extracting the data.
            Json jsonObj(gdrive_dlbuf_get_data(pBuf));
            if (jsonObj.gdrive_json_is_valid())
            {
                // Update or remove cached data for each item in the "items" array.
                Json changeArray = 
                        jsonObj.gdrive_json_get_nested_object("items");
                bool dummy;
                int arraySize = changeArray.gdrive_json_array_length(dummy);
                for (int i = 0; i < arraySize; i++)
                {
                    Json jsonItem(changeArray.gdrive_json_array_get(i));
                    if (!jsonItem.gdrive_json_is_valid())
                    {
                        // Couldn't get this item, skip to the next one.
                        continue;
                    }
                    string fileId = jsonItem.gdrive_json_get_string("fileId");
                    if (fileId.empty())
                    {
                        // Couldn't get an ID for the changed file, skip to the
                        // next one.
                        continue;
                    }
                    
                    // We don't know whether the file has been renamed or moved,
                    // so remove it from the fileId cache.
                    FileidCacheNode::gdrive_fidnode_remove_by_id(&pFileIdCacheHead, fileId);

                    // Update the file metadata cache, but only if the file is not
                    // opened for writing with dirty data.
                    Gdrive_Cache_Node* pCacheNode = 
                            gdrive_cnode_get(gInfo, NULL,
                                                   &pCacheHead, 
                                                   fileId.c_str(), 
                                                   false, 
                                                   NULL
                            );
                    if (pCacheNode != NULL && !gdrive_cnode_is_dirty(pCacheNode))
                    {
                        // If this file was in the cache, update its information
                        Json jsonFile = 
                                jsonItem.gdrive_json_get_nested_object("file");
                        gdrive_cnode_update_from_json(pCacheNode, jsonFile);
                    }
                    // else either not in the cache, or there is dirty data we don't
                    // want to overwrite.


                    // The file's parents may now have a different number of 
                    // children.  Remove the parents from the cache.
                    bool dummy;
                    int numParents = 
                        jsonItem.gdrive_json_array_length("file/parents", 
                            dummy);
                    for (int nParent = 0; nParent < numParents; nParent++)
                    {
                        // Get the fileId of the current parent in the array.
                        Json parentObj = 
                                jsonItem.gdrive_json_array_get("file/parents", 
                                nParent);
                        string parentId = parentObj.gdrive_json_is_valid() ?
                            parentObj.gdrive_json_get_string("id") :
                            "";
                        if (!parentId.empty())
                        {
                            this->gdrive_cache_delete_id(parentId.c_str());
                        }
                    }

                }

                bool success = false;
                
                int64_t newNextChangeId = 
                        jsonObj.gdrive_json_get_int64("largestChangeId", 
                        true, success) + 1;
                if (success)
                {
                    nextChangeId = newNextChangeId;
                }
                
                returnVal = success ? 0 : -1;
            }
        }

        // Reset the last updated time
        lastUpdateTime = time(NULL);

        gdrive_dlbuf_free(pBuf);
        return returnVal;
    }

    fusedrive::Fileinfo* Cache::gdrive_cache_get_item(const string& fileId, 
            bool addIfDoesntExist, bool* pAlreadyExists)
    {
        assert(initialized);
        // Get the existing node (or a new one) from the cache.
        Gdrive_Cache_Node* pNode = gdrive_cnode_get(gInfo, NULL,&pCacheHead,
                fileId.c_str(), addIfDoesntExist, pAlreadyExists);
        if (pNode == NULL)
        {
            // There was an error, or the node doesn't exist and we aren't allowed
            // to create a new one.
            return NULL;
        }

        // Test whether the cached information is too old.  Use last updated time
        // for either the individual node or the entire cache, whichever is newer.
        // If the node's update time is 0, always update it.
        time_t cacheUpdated = lastUpdateTime;
        time_t nodeUpdated = gdrive_cnode_get_update_time(pNode);
        time_t expireTime = (nodeUpdated > cacheUpdated ? 
            nodeUpdated : cacheUpdated) + cacheTTL;
        if (expireTime < time(NULL) || nodeUpdated == (time_t) 0)
        {
            // Update the cache and try again.

            // Folder nodes may be deleted by cache updates, but regular file nodes
            // are safe.
            bool isFolder = (gdrive_cnode_get_filetype(pNode) == 
                    GDRIVE_FILETYPE_FOLDER);

            gdrive_cache_update();

            return (isFolder ? 
                    gdrive_cache_get_item(fileId, addIfDoesntExist, pAlreadyExists) :
                    gdrive_cnode_get_fileinfo(pNode));
        }

        // We have a good node that's not too old.
        return gdrive_cnode_get_fileinfo(pNode);
    }

    int Cache::gdrive_cache_add_fileid(const string& path, const string& fileId)
    {
        assert(initialized);
        return FileidCacheNode::gdrive_fidnode_add(&pFileIdCacheHead, path, fileId);
    }

    Gdrive_Cache_Node* Cache::gdrive_cache_get_node(const string& fileId, 
            bool addIfDoesntExist, bool* pAlreadyExists)
    {
        assert(initialized);
        return gdrive_cnode_get(gInfo, NULL, &pCacheHead, fileId.c_str(),
                addIfDoesntExist, pAlreadyExists);
    }

    string Cache::gdrive_cache_get_fileid(const string& path)
    {
        assert(initialized);
        // Get the cached node if it exists.  If it doesn't exist, fail.
        if (!pFileIdCacheHead)
        {
            // No cache to search
            return "";
        }
        FileidCacheNode* pNode = 
                pFileIdCacheHead->gdrive_fidnode_get_node(path);
        if (pNode == NULL)
        {
            // The path isn't cached.  Return null.
            return "";
        }

        // We have the cached item.  Test whether it's too old.  Use the last update
        // either of the entire cache, or of the individual item, whichever is
        // newer.
        time_t cacheUpdateTime = gdrive_cache_get_lastupdatetime();
        time_t nodeUpdateTime = pNode->gdrive_fidnode_get_lastupdatetime();
        time_t expireTime = ((nodeUpdateTime > cacheUpdateTime) ? 
            nodeUpdateTime : cacheUpdateTime) + cacheTTL;
        if (time(NULL) > expireTime)
        {
            // Item is expired.  Check for updates and try again.
            gdrive_cache_update();
            return gdrive_cache_get_fileid(path);
        }

        return pNode->gdrive_fidnode_get_fileid();
    }

    void Cache::gdrive_cache_delete_id(const string& fileId)
    {
        assert(initialized);
        assert(!fileId.empty());

        // Remove the ID from the file Id cache
        FileidCacheNode::gdrive_fidnode_remove_by_id(&pFileIdCacheHead, fileId);

        // If the file isn't opened by anybody, delete it from the cache 
        // immediately. Otherwise, mark it for delete on close.

        // Find the node we want to remove.
        Gdrive_Cache_Node* pNode = 
                gdrive_cnode_get(gInfo, NULL, &pCacheHead, fileId.c_str(), 
                                       false, NULL);
        if (pNode == NULL)
        {
            // Didn't find it.  Do nothing.
            return;
        }
        gdrive_cnode_mark_deleted(pNode, &pCacheHead);
    }

    void Cache::gdrive_cache_delete_node(Gdrive_Cache_Node* pNode)
    {
        assert(initialized);
        gdrive_cnode_delete(pNode, &pCacheHead);
    }
    
    Cache::~Cache() {
        pFileIdCacheHead->gdrive_fidnode_clear_all();
        gdrive_cnode_free_all(pCacheHead);
    }
    
    
    /**************************
    * Private Methods
    **************************/
    
    void Cache::gdrive_cache_remove_id(Gdrive& gInfo, const string& fileId)
    {
        // Find the node we want to remove.
        Gdrive_Cache_Node* pNode = 
                gdrive_cnode_get(gInfo, NULL, &pCacheHead, fileId.c_str(), 
                                       false, NULL);
        if (pNode == NULL)
        {
            // Didn't find it.  Do nothing.
            return;
        }


        gdrive_cnode_delete(pNode, &pCacheHead);
    }
    
    

    

}