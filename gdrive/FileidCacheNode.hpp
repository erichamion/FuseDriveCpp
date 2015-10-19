/* 
 * File:   FileidCacheNode.hpp
 * Author: me
 *
 * Created on October 19, 2015, 8:49 AM
 */

#ifndef FILEIDCACHENODE_HPP
#define	FILEIDCACHENODE_HPP

#include <time.h>
#include <string>

namespace fusedrive
{
    class FileidCacheNode {
    public:
        static int gdrive_fidnode_add(FileidCacheNode** ppHead, 
                const std::string& path, const std::string& fileId);

        static void gdrive_fidnode_remove_by_id(FileidCacheNode** ppHead, 
            const std::string& fileId);

        void gdrive_fidnode_clear_all();


        time_t gdrive_fidnode_get_lastupdatetime() const;

        std::string gdrive_fidnode_get_fileid() const;


        FileidCacheNode* gdrive_fidnode_get_node(const std::string& path);

    private:
        time_t lastUpdateTime;
        std::string path;
        std::string fileId;
        struct FileidCacheNode* pNext;
        
//        FileidCacheNode* gdrive_fidnode_create(const char* filename, 
//                const char* fileId);
        FileidCacheNode(const std::string& filename, const std::string& fileId);
        
        virtual ~FileidCacheNode();

        int gdrive_fidnode_update_item(const std::string& updatedFileId);

        FileidCacheNode(const FileidCacheNode& orig);
        
    };
}

#endif	/* FILEIDCACHENODE_HPP */

