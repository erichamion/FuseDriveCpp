/* 
 * File:   CacheNode.hpp
 * Author: me
 *
 * Created on October 19, 2015, 11:11 AM
 */

#ifndef CACHENODE_HPP
#define	CACHENODE_HPP

#include "FileContents.hpp"
#include "Fileinfo.hpp"

#include <string>


namespace fusedrive
{
    class Cache;
    
    class CacheNode {
        
    public:
        static CacheNode* retrieveNode(Cache& cache, CacheNode* pParent, 
                CacheNode** ppNode, const std::string& fileId, 
                bool addIfDoesntExist, bool& alreadyExists);
        
        static CacheNode* retrieveNode(Cache& cache, CacheNode* pParent, 
            CacheNode** ppNode, const std::string& fileId, 
            bool addIfDoesntExist);
        
        Gdrive& getGdrive() const;

        void deleteNode();

        void markDeleted();
        
        bool isDeleted() const;

        void freeAll();

        time_t getUpdateTime() const;

        enum Gdrive_Filetype getFiletype() const;

        Fileinfo& getFileinfo();

        void updateFromJson(Json& jsonObj);

        void deleteFileContents(FileContents& contentsToDelete);

        bool isDirty() const;
        
        void setDirty(bool val=true);
        
        int getOpenCount() const;
        
        int getOpenWriteCount() const;
        
        void incrementOpenCount(bool isWrite);
        
        void decrementOpenCount(bool isWrite);
        
        bool checkPermissions(int accessFlags) const;
        
        void clearContents();
        
        bool hasContents() const;
        
        FileContents* createChunk(off_t offset, size_t size, 
            bool fillChunk);
        
        FileContents* findChunk(off_t offset) const;
        
        FileContents** getContentsListPtr();
        
        void deleteContentsAfterOffset(off_t offset);
        
        
        
        
    private:
        Gdrive& gInfo;
        time_t lastUpdateTime;
        int openCount;
        int openWrites;
        bool dirty;
        bool deleted;
        Fileinfo fileinfo;
        FileContents* pContents;
        Cache& mCache;
        struct CacheNode* pParent;
        struct CacheNode* pLeft;
        struct CacheNode* pRight;
        
        CacheNode(CacheNode* pParent);
        
        CacheNode(Gdrive& gInfo);
        
        void init();
        
        static void SwapNodes(CacheNode** ppFromParentOne, CacheNode* pNodeOne,
            CacheNode** ppFromParentTwo, CacheNode* pNodeTwo);

        FileContents& addContents();
        
        
        //CacheNode* gdrive_cnode_get_head();
        
        //void gdrive_cnode_set_head(CacheNode* newHead);

        virtual ~CacheNode();
        
        CacheNode(const CacheNode& orig);
        
    };
}

#endif	/* CACHENODE_HPP */
