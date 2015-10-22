/* 
 * File:   File.hpp
 * Author: me
 *
 * Created on October 19, 2015, 7:08 PM
 */

#ifndef FILE_HPP
#define	FILE_HPP

#include "Fileinfo.hpp"
#include "CacheNode.hpp"

#include <string>
#include <iostream>

namespace fusedrive
{
    class GdriveFile {
        friend GdriveFile* openFileHelper(CacheNode& cacheNode);
        
    public:
        //GdriveFile(const GdriveFile& orig);
        virtual ~GdriveFile();
        
        void close(int flags);

        int read(char* buf, size_t size, off_t offset);

        int write(const char* buf, size_t size, off_t offset);

        int truncate(off_t size);

        int sync();

        int syncMetadata();

        int setAtime(const struct timespec* ts);

        int setMtime(const struct timespec* ts);

        Fileinfo& getFileinfo();

        unsigned int getPermissions();
    private:
        Gdrive& mGInfo;
        CacheNode& mCacheNode;
        
        int nullRead(size_t size, off_t offset);
        
        size_t readNextChunk(char* buf, off_t offset, 
            size_t size);

        off_t writeNextChunk(const char* buf, 
            off_t offset, size_t size);

        static size_t uploadCallback(Gdrive& gInfo, char* buffer, 
            off_t offset, size_t size, void* userdata);

        static std::string 
        syncMetadataOrCreate(Gdrive& gInfo, Fileinfo* pFileinfo,
            const std::string& parentId, const std::string& filename, 
            bool isFolder, int& error);
        
        GdriveFile(CacheNode& cacheNode);
        
    };
    
    GdriveFile* openFileHelper(CacheNode& cacheNode);
}

#endif	/* FILE_HPP */

