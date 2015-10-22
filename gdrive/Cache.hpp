/* 
 * File:   Cache.hpp
 * Author: me
 *
 * Created on October 18, 2015, 8:12 PM
 */

#ifndef CACHE_HPP
#define	CACHE_HPP

#include "FileidCacheNode.hpp"
#include "CacheNode.hpp"

#include <sys/types.h>


namespace fusedrive
{
    class Gdrive;
    class Fileinfo;
    class CacheNode;

    class Cache {
        friend CacheNode** gdrive_cache_get_head_ptr(Cache& cache);
    public:
        Cache(Gdrive& gInfo, time_t cacheTTL);
        
        void init();
        
        Gdrive& getGdrive() const;
        
        const FileidCacheNode* getFileidCacheHead() const;

        time_t getTTL() const;

        time_t getLastUpdateTime() const;

        int64_t getNextChangeId() const;

        int UpdateIfStale();

        int update();

        Fileinfo* getItem(const std::string& fileId, bool addIfDoesntExist, 
            bool& pAlreadyExists);
        
        Fileinfo* getItem(const std::string& fileId, bool addIfDoesntExist);
        
        const CacheNode* getHead() const;
        
        void setHead(CacheNode* newHead);
        

        int addFileid(const std::string& path, const std::string& fileId);

        CacheNode* getNode(const std::string& fileId, bool addIfDoesntExist, 
            bool& alreadyExists);

        CacheNode* getNode(const std::string& fileId, bool addIfDoesntExist);
        
        std::string getFileid(const std::string& path);

        void deleteId(const std::string& fileId);

        void deleteNode(CacheNode* pNode);
        
        virtual ~Cache();
        
    private:
        Gdrive& mGInfo;
        time_t mCacheTTL;
        time_t mLastUpdateTime;
        int64_t mNextChangeId;
        CacheNode* mpCacheHead;
        FileidCacheNode* mpFileIdCacheHead;
        bool mInitialized;
        
        void removeId(Gdrive& gInfo, const std::string& fileId);
        
        Cache(const Cache& orig);
    };

}

#endif	/* CACHE_HPP */

