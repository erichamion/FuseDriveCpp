/* 
 * File:   CacheNode.hpp
 * Author: me
 *
 * Created on October 19, 2015, 11:11 AM
 */

#ifndef CACHENODE_HPP
#define	CACHENODE_HPP

#include "gdrive-file-contents.hpp"
#include "Fileinfo.hpp"

#include <string>


namespace fusedrive
{
    class Cache;
    
    class CacheNode {
    public:
        static CacheNode* gdrive_cnode_get(Cache& cache, CacheNode* pParent, 
                CacheNode** ppNode, const std::string& fileId, 
                bool addIfDoesntExist, bool& alreadyExists);
        
        static CacheNode* gdrive_cnode_get(Cache& cache, CacheNode* pParent, 
            CacheNode** ppNode, const std::string& fileId, 
            bool addIfDoesntExist);
        
        Gdrive& gdrive_cnode_get_gdrive();

        void gdrive_cnode_delete();

        void gdrive_cnode_mark_deleted();
        
        bool gdrive_cnode_isdeleted();

        void gdrive_cnode_free_all();

        time_t gdrive_cnode_get_update_time();

        enum Gdrive_Filetype gdrive_cnode_get_filetype();

        Fileinfo& gdrive_cnode_get_fileinfo();


        void gdrive_cnode_update_from_json(Json& jsonObj);

        void gdrive_cnode_delete_file_contents(Gdrive_File_Contents* pContentsToDelete);

        bool gdrive_cnode_is_dirty();
        
        void gdrive_cnode_set_dirty(bool val=true);
        
        int gdrive_cnode_get_open_count();
        
        int gdrive_cnode_get_open_write_count();
        
        void gdrive_cnode_increment_open_count(bool isWrite);
        
        void gdrive_cnode_decrement_open_count(bool isWrite);
        
        bool gdrive_cnode_check_perm(int accessFlags);
        
        void gdrive_cnode_clear_contents();
        
        bool gdrive_cnode_has_contents();
        
        Gdrive_File_Contents* gdrive_cnode_create_chunk(off_t offset, 
            size_t size, bool fillChunk);
        
        Gdrive_File_Contents* gdrive_cnode_find_chunk(off_t offset);
        
        void gdrive_cnode_delete_contents_after_offset(off_t offset);
        

        
        
    private:
        Gdrive& gInfo;
        time_t lastUpdateTime;
        int openCount;
        int openWrites;
        bool dirty;
        bool deleted;
        Fileinfo fileinfo;
        Gdrive_File_Contents* pContents;
        Cache& mCache;
        struct CacheNode* pParent;
        struct CacheNode* pLeft;
        struct CacheNode* pRight;
        
        CacheNode(CacheNode* pParent);
        
        CacheNode(Gdrive& gInfo);
        
        void gdrive_cnode_init();
        
        static void gdrive_cnode_swap(CacheNode** ppFromParentOne, 
                                      CacheNode* pNodeOne, 
                                      CacheNode** ppFromParentTwo, 
                                      CacheNode* pNodeTwo);

        Gdrive_File_Contents* gdrive_cnode_add_contents();
        
        
        CacheNode* gdrive_cnode_get_head();
        
        void gdrive_cnode_set_head(CacheNode* newHead);

        virtual ~CacheNode();
        
        CacheNode(const CacheNode& orig);
        
    };
}

#endif	/* CACHENODE_HPP */
