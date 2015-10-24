/* 
 * File:   Fileinfo.cpp
 * Author: me
 * 
 * Created on October 17, 2015, 5:15 PM
 */

#include "Fileinfo.hpp"

#include "Gdrive.hpp"
#include "Cache.hpp"
#include "Util.hpp"

#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <exception>
#include <sstream>
#include <iomanip>

using namespace std;


namespace fusedrive
{
    
    
    
    /**************************
    * Public Methods
    **************************/
    
    
    void Fileinfo::clear()
    {
        filename.clear();
        id.clear();
        
        type = (Gdrive_Filetype) 0;
        size = 0;
        basePermission = 0;
        creationTime = {0};
        modificationTime = {0};
        accessTime = {0};
        nParents = 0;
        nChildren = 0;
        dirtyMetainfo = false;
    }

    string Fileinfo::getAtimeString() const
    {
        
        return Util::epochTimeNSToRfc3339(&accessTime);
    }

    int Fileinfo::setAtime(const struct timespec* ts)
    {
        
        return setTime(GDRIVE_FINFO_ATIME, ts);
    }

    string Fileinfo::getCtimeString() const
    {
        
        return Util::epochTimeNSToRfc3339(&creationTime);
    }

    string Fileinfo::getMtimeString() const
    {
        
        return Util::epochTimeNSToRfc3339(&modificationTime);
    }

    int Fileinfo::setMtime(const struct timespec* ts)
    {
        
        return setTime(GDRIVE_FINFO_MTIME, ts);
    }

    void Fileinfo::readJson(const Json& jsonObj)
    {
        
        filename.assign(jsonObj.getString("title"));
        id.assign(jsonObj.getString("id"));
        
        bool success;
        size = jsonObj.getInt64("fileSize", true, success);
        if (!success)
        {
            size = 0;
        }
        
        string mimeType(jsonObj.getString("mimeType"));
        if (!mimeType.empty())
        {
            if (mimeType == GDRIVE_MIMETYPE_FOLDER)
            {
                // Folder
                type = GDRIVE_FILETYPE_FOLDER;
            }
            else if (false)
            {
                // TODO: Add any other special file types.  This
                // will likely include Google Docs.
            }
            else
            {
                // Regular file
                type = GDRIVE_FILETYPE_FILE;
            }
        }

        // Get the user's permissions for the file on the Google Drive account.
        string role(jsonObj.getString("userPermission/role"));
        if (!role.empty())
        {
            int basePerm = 0;
            if (role == "owner")
            {
                // Full read-write access
                basePerm = S_IWOTH | S_IROTH;
            }
            else if (role == "writer")
            {
                // Full read-write access
                basePerm = S_IWOTH | S_IROTH;
            }
            else if (role == "reader")
            {
                // Read-only access
                basePerm = S_IROTH;
            }

            basePermission = basePerm;

            // Directories need read and execute permissions to be navigable, and 
            // write permissions to create files. 
            if (type == GDRIVE_FILETYPE_FOLDER)
            {
                basePermission = S_IROTH | S_IWOTH | S_IXOTH;
            }
        }

        string cTime(jsonObj.getString("createdDate"));
        if (cTime.empty() || 
                Util::rfc3339ToEpochTimeNS(cTime, &creationTime) != 0)
        {
            // Didn't get a createdDate or failed to convert it.
            memset(&creationTime, 0, sizeof(struct timespec));
        }

        string mTime(jsonObj.getString("modifiedDate"));
        if (mTime.empty() || 
                Util::rfc3339ToEpochTimeNS(mTime, &modificationTime) != 0)
        {
            // Didn't get a modifiedDate or failed to convert it.
            memset(&modificationTime, 0, sizeof(struct timespec));
        }

        string aTime(jsonObj.getString("lastViewedByMeDate"));
        if (aTime.empty() || 
                Util::rfc3339ToEpochTimeNS(aTime, &accessTime) != 0)
        {
            // Didn't get an accessed date or failed to convert it.
            memset(&accessTime, 0, sizeof(struct timespec));
        }
        
        bool dummy;
        nParents = jsonObj.getArrayLength("parents", dummy);

        dirtyMetainfo = false;
    }

    unsigned int Fileinfo::getRealPermissions() const
    {
        
        
        // Get the overall system permissions, which are different for a folder
        // or for a regular file.
        int systemPerm = gInfo.getFilesystemPermissions(type);

        // Combine the system permissions with the actual file permissions.
        return systemPerm & basePermission;
    }

    Fileinfo::Fileinfo(Gdrive& gInfo)
    : gInfo(gInfo)
    {
        clear();
    }
    
    Fileinfo::Fileinfo(Gdrive& gInfo, const Json& jsonObj)
    : gInfo(gInfo)
    {
        clear();
        readJson(jsonObj);
    }

    Fileinfo::~Fileinfo() {
        
        clear();
    }
    
    
    /**************************
    * Private Constants
    **************************/
    
    const std::string Fileinfo::GDRIVE_MIMETYPE_FOLDER = 
        "application/vnd.google-apps.folder";
    
    
    /**************************
    * Private Methods
    **************************/
    

     int Fileinfo::setTime(enum GDRIVE_FINFO_TIME whichTime, 
             const struct timespec* ts)
    {
        
        assert(whichTime == GDRIVE_FINFO_ATIME || 
                whichTime == GDRIVE_FINFO_MTIME);

        struct timespec* pDest = NULL;
        switch (whichTime)
        {
            case GDRIVE_FINFO_ATIME:
                pDest = &accessTime;
                break;
            case GDRIVE_FINFO_MTIME:
                pDest = &modificationTime;
                break;
        }

        // Set current time if ts is a NULL pointer
        const struct timespec* pTime = ts;
        if (pTime == NULL)
        {
            struct timespec currentTime;
            if (clock_gettime(CLOCK_REALTIME, &currentTime) != 0)
            {
                // Fail
                return -1;
            }
            pTime = &currentTime;
        }

        if (pTime->tv_sec == pDest->tv_sec && 
                pTime->tv_nsec == pDest->tv_nsec)
        {
            // Time already set, do nothing
            return 0;
        }


        dirtyMetainfo = true;
        // Make a copy of *pTime, and store the copy at the place pDest
        // points to (which is either the accessTime or modificationTime member
        // of pFileinfo)/
        *pDest = *pTime;
        return 0;
    }
     
     
}