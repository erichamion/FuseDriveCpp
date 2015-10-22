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
    class Gdrive;
    
    class GdriveFile {
    public:
        //GdriveFile(const GdriveFile& orig);
        virtual ~GdriveFile();
        
        static GdriveFile* gdrive_file_open(Gdrive& gInfo, 
                const std::string& fileId, int flags, int& error);

        static std::string gdrive_file_new(Gdrive& gInfo, const std::string& path, 
            bool createFolder, int& error);

        void gdrive_file_close(int flags);

        int gdrive_file_read(char* buf, size_t size, off_t offset);

        int gdrive_file_write(const char* buf, size_t size, off_t offset);

        int gdrive_file_truncate(off_t size);

        int gdrive_file_sync();

        int gdrive_file_sync_metadata();

        int gdrive_file_set_atime(const struct timespec* ts);

        int gdrive_file_set_mtime(const struct timespec* ts);

        Fileinfo& gdrive_file_get_info();

        unsigned int gdrive_file_get_perms();
    private:
        Gdrive& gInfo;
        CacheNode& cacheNode;
        
        int gdrive_file_null_read(size_t size, off_t offset);
        
        size_t gdrive_file_read_next_chunk(char* buf, off_t offset, 
            size_t size);

        off_t gdrive_file_write_next_chunk(const char* buf, 
            off_t offset, size_t size);

        static size_t gdrive_file_uploadcallback(Gdrive& gInfo, char* buffer, 
            off_t offset, size_t size, void* userdata);

        static std::string 
        gdrive_file_sync_metadata_or_create(Gdrive& gInfo, Fileinfo* pFileinfo,
            const std::string& parentId, const std::string& filename, 
            bool isFolder, int& error);
        
        GdriveFile(CacheNode& cacheNode);
        
    };
}

#endif	/* FILE_HPP */

