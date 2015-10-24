/* 
 * File:   FileContents.hpp
 * Author: me
 *
 * Created on October 23, 2015, 12:45 AM
 */

#ifndef FILECONTENTS_HPP
#define	FILECONTENTS_HPP

#include <sys/types.h>
#include <string>


namespace fusedrive
{
    class Gdrive;
    class CacheNode;

    class FileContents {
    public:
        static FileContents& gdrive_fcontents_add_new(CacheNode& cacheNode);
        
        virtual ~FileContents();
        
        void gdrive_fcontents_delete();
        
        /**
         * Note: This may invalidate the object or pointer on which it is
         * called. If necessary, the associated CacheNode's contents pointer
         * will be adjusted.
         * @param offset
         */
        void gdrive_fcontents_delete_after_offset(off_t offset);

        void gdrive_fcontents_free_all();

        FileContents* gdrive_fcontents_find_chunk(off_t offset);

        int gdrive_fcontents_fill_chunk(const std::string& fileId, off_t start, 
            size_t size);

        size_t gdrive_fcontents_read(char* destBuf, off_t offset, size_t size);

        off_t gdrive_fcontents_write(const char* buf, off_t offset, 
            size_t size, bool extendChunk);

        int gdrive_fcontents_truncate(size_t size);
        
    private:
        off_t start;
        off_t end;
        FILE* fh;
        CacheNode& cacheNode;
        FileContents* pNext;
        
        FileContents(CacheNode& cacheNode);
        
        
        FileContents(const FileContents& orig);
        
    };

}

#endif	/* FILECONTENTS_HPP */

