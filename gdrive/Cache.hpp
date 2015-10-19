/* 
 * File:   Cache.hpp
 * Author: me
 *
 * Created on October 18, 2015, 8:12 PM
 */

#ifndef CACHE_HPP
#define	CACHE_HPP

#include "FileidCacheNode.hpp"

#include <sys/types.h>

typedef struct Gdrive_Cache_Node Gdrive_Cache_Node;

namespace fusedrive
{
    class Gdrive;
    class Fileinfo;

    class Cache {
    public:
        Cache(Gdrive& gInfo, time_t cacheTTL);
        
        void gdrive_cache_init();
        
        FileidCacheNode* gdrive_cache_get_fileidcachehead();

        time_t gdrive_cache_get_ttl();

        time_t gdrive_cache_get_lastupdatetime();

        int64_t gdrive_cache_get_nextchangeid();

        int gdrive_cache_update_if_stale();

        int gdrive_cache_update();

        Fileinfo* gdrive_cache_get_item(const std::string& fileId, 
        bool addIfDoesntExist, bool* pAlreadyExists);

        int gdrive_cache_add_fileid(const std::string& path, const std::string& fileId);

        Gdrive_Cache_Node* gdrive_cache_get_node(const std::string& fileId, 
        bool addIfDoesntExist, bool* pAlreadyExists);

        std::string gdrive_cache_get_fileid(const std::string& path);

        void gdrive_cache_delete_id(const std::string& fileId);

        void gdrive_cache_delete_node(Gdrive_Cache_Node* pNode);
        
        virtual ~Cache();
    private:
        Gdrive& gInfo;
        time_t cacheTTL;
        time_t lastUpdateTime;
        int64_t nextChangeId;
        Gdrive_Cache_Node* pCacheHead;
        FileidCacheNode* pFileIdCacheHead;
        bool initialized;
        
        void gdrive_cache_remove_id(Gdrive& gInfo, const std::string& fileId);
        
        Cache(const Cache& orig);
    };

}

#endif	/* CACHE_HPP */

