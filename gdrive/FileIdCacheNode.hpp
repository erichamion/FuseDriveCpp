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
    class FileIdCacheNode {
    public:
        static int addFileIdNode(FileIdCacheNode** ppHead, 
                const std::string& path, const std::string& fileId);

        static void removeFileIdNode(FileIdCacheNode** ppHead, 
            const std::string& fileId);

        void clearAll();


        time_t getLastUpdateTime() const;

        std::string getFileId() const;


        FileIdCacheNode* getNode(const std::string& path);

    private:
        time_t mLastUpdateTime;
        std::string mPath;
        std::string mFileId;
        struct FileIdCacheNode* mpNext;
        
//        FileidCacheNode* gdrive_fidnode_create(const char* filename, 
//                const char* fileId);
        FileIdCacheNode(const std::string& filename, const std::string& fileId);
        
        virtual ~FileIdCacheNode();

        int updateItem(const std::string& updatedFileId);

        FileIdCacheNode(const FileIdCacheNode& orig);
        
    };
}

#endif	/* FILEIDCACHENODE_HPP */

