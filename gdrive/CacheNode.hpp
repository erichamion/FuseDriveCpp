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

namespace fusedrive
{
    class CacheNode {
    public:
        Gdrive_Cache_Node* gdrive_cnode_get(fusedrive::Gdrive& gInfo, Gdrive_Cache_Node* pParent, 
                                    Gdrive_Cache_Node** ppNode, 
                                    const char* fileId, bool addIfDoesntExist, 
                                    bool* pAlreadyExists);

        void gdrive_cnode_delete(Gdrive_Cache_Node* pNode, 
                                 Gdrive_Cache_Node** ppToRoot);

        void gdrive_cnode_mark_deleted(Gdrive_Cache_Node* pNode, 
                                       Gdrive_Cache_Node** ppToRoot);

        void gdrive_cnode_free_all(Gdrive_Cache_Node* pRoot);


        time_t gdrive_cnode_get_update_time(Gdrive_Cache_Node* pNode);

        enum fusedrive::Gdrive_Filetype gdrive_cnode_get_filetype(Gdrive_Cache_Node* pNode);

        fusedrive::Fileinfo* gdrive_cnode_get_fileinfo(Gdrive_Cache_Node* pNode);


        void gdrive_cnode_update_from_json(Gdrive_Cache_Node* pNode, 
                                           fusedrive::Json& jsonObj);

        void gdrive_cnode_delete_file_contents(Gdrive_Cache_Node* pNode, 
                                        Gdrive_File_Contents* pContents);

        
        bool gdrive_cnode_is_dirty(const Gdrive_Cache_Node* pNode);
        
    private:
        time_t lastUpdateTime;
        int openCount;
        int openWrites;
        bool dirty;
        bool deleted;
        Fileinfo* pFileinfo;
        Gdrive_File_Contents* pContents;
        struct CacheNode* pParent;
        struct CacheNode* pLeft;
        struct CacheNode* pRight;
        Cache& mCache;
        
        CacheNode(Gdrive& gInfo, CacheNode* pParent);

        void gdrive_cnode_swap(Gdrive_Cache_Node** ppFromParentOne, 
                                      Gdrive_Cache_Node* pNodeOne, 
                                      Gdrive_Cache_Node** ppFromParentTwo, 
                                      Gdrive_Cache_Node* pNodeTwo);

        void gdrive_cnode_free(Gdrive_Cache_Node* pNode);

        Gdrive_File_Contents* 
        gdrive_cnode_add_contents(Gdrive_Cache_Node* pNode);

        Gdrive_File_Contents* 
        gdrive_cnode_create_chunk(Gdrive& gInfo, Gdrive_Cache_Node* pNode, off_t offset, size_t size, 
                                  bool fillChunk);

        size_t gdrive_file_read_next_chunk(Gdrive& gInfo, Gdrive_File* pNode, char* destBuf, 
                                                  off_t offset, size_t size);

        off_t gdrive_file_write_next_chunk(Gdrive& gInfo, Gdrive_File* pFile, const char* buf, 
                                                  off_t offset, size_t size);

        bool gdrive_file_check_perm(Gdrive& gInfo, const Gdrive_Cache_Node* pNode, 
                                           int accessFlags);

        size_t gdrive_file_uploadcallback(Gdrive& gInfo, char* buffer, off_t offset, 
                                                 size_t size, void* userdata);

        char* gdrive_file_sync_metadata_or_create(fusedrive::Gdrive& gInfo, Fileinfo* pFileinfo, 
                                                         const char* parentId, 
                                                         const char* filename, 
                                                         bool isFolder, int* pError);
        
        virtual ~CacheNode();
        
        CacheNode(const CacheNode& orig);
        
    };
}

#endif	/* CACHENODE_HPP */

