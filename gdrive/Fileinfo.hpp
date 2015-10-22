/* 
 * File:   Fileinfo.hpp
 * Author: me
 *
 * Created on October 17, 2015, 5:15 PM
 */

#ifndef FILEINFO_HPP
#define	FILEINFO_HPP

#include <time.h>
#include "GdriveEnums.hpp"
#include "Json.hpp"

#include <string>

namespace fusedrive
{
    class Gdrive;

    class Fileinfo {
    public:
        // id: The Google Drive file ID of the file
        std::string id;
        // filename: The filename with extension (not the full path)
        std::string filename;
        // type: The type of file
        enum fusedrive::Gdrive_Filetype type;
        // size: File size in bytes
        size_t size;
        // basePermission: File permission, does not consider the access mode.
        int basePermission;
        struct timespec creationTime;
        struct timespec modificationTime;
        struct timespec accessTime;
        // nParents: Number of parent directories
        int nParents;
        // nChildren: Number of children if type is GDRIVE_FILETYPE_FOLDER
        int nChildren;
        // dirtyMetainfo: Currently only tracks accessTime and modificationTime
        bool dirtyMetainfo;
        Gdrive& gInfo;
        
        static const Fileinfo& getFileinfoById(fusedrive::Gdrive& gInfo, 
            const std::string& fileId);

        void Cleanup();

        std::string getAtimeString() const;

        int setAtime(const struct timespec* ts);

        std::string getCtimeString() const;

        std::string getMtimeString() const;

        int setMtime(const struct timespec* ts);

        void readJson(Json& jsonObj);

        unsigned int getRealPermissions() const;

        Fileinfo(Gdrive& gInfo);
        
        virtual ~Fileinfo();
        
    private:
        enum GDRIVE_FINFO_TIME
        {
            GDRIVE_FINFO_ATIME,
            GDRIVE_FINFO_MTIME
        };
        
        static const std::string GDRIVE_MIMETYPE_FOLDER;
        
        static int rfc3339ToEpochTimeNS(const std::string& rfcTime, 
            struct timespec* pResultTime);

        static std::string 
        epochTimeNSToRfc3339(const struct timespec* ts);

        int setTime(enum GDRIVE_FINFO_TIME whichTime, 
            const struct timespec* ts);
        
        Fileinfo(const Fileinfo& orig);
    };
    
    

}

#endif	/* FILEINFO_HPP */

