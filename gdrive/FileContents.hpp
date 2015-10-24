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
        static FileContents& addNewChunk(CacheNode& cacheNode);
        
        virtual ~FileContents();
        
        /**
         * Note: This may invalidate the object or pointer on which it is
         * called. If necessary, the associated CacheNode's contents pointer
         * will be adjusted.
         * @param offset
         */
        void deleteAfterOffset(off_t offset);

        void freeAll();

        FileContents* findChunk(off_t offset);

        int fillChunk(const std::string& fileId, off_t start, 
            size_t size);

        size_t read(char* destBuf, off_t offset, size_t size) const;

        off_t write(const char* buf, off_t offset, 
            size_t size, bool extendChunk);

        int truncate(size_t size);
        
    private:
        off_t mStart;
        off_t mEnd;
        FILE* mFh;
        CacheNode& mCacheNode;
        FileContents* mpNext;
        
        FileContents(CacheNode& cacheNode);
        
        
        FileContents(const FileContents& orig);
        
    };

}

#endif	/* FILECONTENTS_HPP */

