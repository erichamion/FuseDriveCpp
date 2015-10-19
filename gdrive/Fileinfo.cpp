/* 
 * File:   Fileinfo.cpp
 * Author: me
 * 
 * Created on October 17, 2015, 5:15 PM
 */

#include "Fileinfo.hpp"

#include "Gdrive.hpp"
#include "Cache.hpp"
#include "gdrive-file.hpp"
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
    * Public Constants
    **************************/
    
    const unsigned int Fileinfo::GDRIVE_TIMESTRING_LENGTH = 31;
    
    
    /**************************
    * Public Methods
    **************************/
    
    const Fileinfo& Fileinfo::gdrive_finfo_get_by_id(Gdrive& gInfo, const string& fileId)
    {
        // Get the information from the cache, or put it in the cache if it isn't
        // already there.
        bool alreadyCached = false;

        Fileinfo* pFileinfo = gInfo.gdrive_get_cache()
                .gdrive_cache_get_item(fileId.c_str(), true, &alreadyCached);
        if (pFileinfo == NULL)
        {
            // An error occurred, probably out of memory.
            throw new exception();
        }

        if (alreadyCached)
        {
            // Don't need to do anything else.
            return *pFileinfo;
        }
        // else it wasn't cached, need to fill in the struct

        // Prepare the request
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(gInfo);
        if (pTransfer == NULL)
        {
            // Memory error
            throw new exception();
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);

        // Add the URL.
        // String to hold the url.  Add 2 to the end to account for the '/' before
        // the file ID, as well as the terminating null.
        string baseUrl(Gdrive::GDRIVE_URL_FILES);
        baseUrl += "/";
        baseUrl += fileId;
        if (gdrive_xfer_set_url(pTransfer, baseUrl.c_str()) != 0)
        {
            // Error
            gdrive_xfer_free(pTransfer);
            throw new exception();
        }

        // Add query parameters
        if (gdrive_xfer_add_query(gInfo, pTransfer, "fields", 
                                  "title,id,mimeType,fileSize,createdDate,"
                                  "modifiedDate,lastViewedByMeDate,parents(id),"
                                  "userPermission") != 0)
        {
            // Error
            gdrive_xfer_free(pTransfer);
            throw new exception();
        }

        // Perform the request
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(gInfo, pTransfer);
        gdrive_xfer_free(pTransfer);

        if (pBuf == NULL)
        {
            // Download error
            throw new exception();
        }

        if (gdrive_dlbuf_get_httpresp(pBuf) >= 400)
        {
            // Server returned an error that couldn't be retried, or continued
            // returning an error after retrying
            gdrive_dlbuf_free(pBuf);
            throw new exception();
        }

        // If we're here, we have a good response.  Extract the ID from the 
        // response.

        // Convert to a JSON object.
        Json jsonObj(gdrive_dlbuf_get_data(pBuf));
        gdrive_dlbuf_free(pBuf);
        if (!jsonObj.gdrive_json_is_valid())
        {
            // Couldn't convert to Json object
            throw new exception();
        }
        
        pFileinfo->gdrive_finfo_read_json(jsonObj);

        // If it's a folder, get the number of children.
        if (pFileinfo->type == GDRIVE_FILETYPE_FOLDER)
        {
            Gdrive_Fileinfo_Array* pFileArray = gInfo.gdrive_folder_list(fileId);
            if (pFileArray != NULL)
            {

                pFileinfo->nChildren = gdrive_finfoarray_get_count(pFileArray);
            }
            gdrive_finfoarray_free(pFileArray);
        }
        return *pFileinfo;
    }

    void Fileinfo::gdrive_finfo_cleanup()
    {
        TestStop();
        filename.clear();
        id.clear();
    }

    string Fileinfo::gdrive_finfo_get_atime_string()
    {
        TestStop();
        return gdrive_epoch_timens_to_rfc3339(&accessTime);
    }

    int Fileinfo::gdrive_finfo_set_atime(const struct timespec* ts)
    {
        TestStop();
        return gdrive_finfo_set_time(GDRIVE_FINFO_ATIME, ts);
    }

    string Fileinfo::gdrive_finfo_get_ctime_string()
    {
        TestStop();
        return gdrive_epoch_timens_to_rfc3339(&creationTime);
    }

    string Fileinfo::gdrive_finfo_get_mtime_string()
    {
        TestStop();
        return gdrive_epoch_timens_to_rfc3339(&modificationTime);
    }

    int Fileinfo::gdrive_finfo_set_mtime(const struct timespec* ts)
    {
        TestStop();
        return gdrive_finfo_set_time(GDRIVE_FINFO_MTIME, ts);
    }

    void Fileinfo::gdrive_finfo_read_json(Json& jsonObj)
    {
        TestStop();
        filename.assign(jsonObj.gdrive_json_get_string("title"));
        id.assign(jsonObj.gdrive_json_get_string("id"));
        
        bool success;
        size = jsonObj.gdrive_json_get_int64("fileSize", true, success);
        if (!success)
        {
            size = 0;
        }
        
        string mimeType(jsonObj.gdrive_json_get_string("mimeType"));
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
        string role(jsonObj.gdrive_json_get_string("userPermission/role"));
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

        string cTime(jsonObj.gdrive_json_get_string("createdDate"));
        if (cTime.empty() || 
                gdrive_rfc3339_to_epoch_timens(cTime.c_str(), &creationTime) != 0)
        {
            // Didn't get a createdDate or failed to convert it.
            memset(&creationTime, 0, sizeof(struct timespec));
        }

        string mTime(jsonObj.gdrive_json_get_string("modifiedDate"));
        if (mTime.empty() || 
                gdrive_rfc3339_to_epoch_timens
                (mTime.c_str(), &modificationTime) != 0)
        {
            // Didn't get a modifiedDate or failed to convert it.
            memset(&modificationTime, 0, sizeof(struct timespec));
        }

        string aTime(jsonObj.gdrive_json_get_string("lastViewedByMeDate"));
        if (aTime.empty() || 
                gdrive_rfc3339_to_epoch_timens
                (aTime.c_str(), &accessTime) != 0)
        {
            // Didn't get an accessed date or failed to convert it.
            memset(&accessTime, 0, sizeof(struct timespec));
        }
        
        bool dummy;
        nParents = jsonObj.gdrive_json_array_length("parents", dummy);

        dirtyMetainfo = false;
    }

    unsigned int Fileinfo::gdrive_finfo_real_perms() const
    {
        TestStop();
        
        // Get the overall system permissions, which are different for a folder
        // or for a regular file.
        int systemPerm = gInfo.gdrive_get_filesystem_perms(type);

        // Combine the system permissions with the actual file permissions.
        return systemPerm & basePermission;
    }

    Fileinfo::Fileinfo(Gdrive& gInfo)
    : gInfo(gInfo)
    {
        TestStop();
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

    

    Fileinfo::~Fileinfo() {
        TestStop();
        gdrive_finfo_cleanup();
    }
    
    
    /**************************
    * Private Constants
    **************************/
    
    const std::string Fileinfo::GDRIVE_MIMETYPE_FOLDER = 
        "application/vnd.google-apps.folder";
    
    
    /**************************
    * Private Methods
    **************************/
    
    int Fileinfo::gdrive_rfc3339_to_epoch_timens(const string& rfcTime, 
            struct timespec* pResultTime)
    {
        // Get the time down to seconds. Don't do anything with it yet, because
        // we still need to confirm the timezone.
        struct tm epochTime = {0};
        size_t remainderStart;
        try
        {
            remainderStart = Util::strptime(rfcTime, "%Y-%m-%dT%H:%M:%S", &epochTime);
        }
        catch (const exception& e)
        {
            // Conversion failure.  
            return -1;
        }

        // Get the fraction of a second.  The remainder variable points to the next 
        // character after seconds.  If and only if there are fractional seconds 
        // (which Google Drive does use but which are optional per the RFC 3339 
        // specification),  this will be the '.' character.
        if (remainderStart != string::npos && rfcTime[remainderStart] == '.')
        {
            // Rather than getting the integer after the decimal and needing to 
            // count digits or count leading "0" characters, it's easier just to
            // get a floating point (or double) fraction between 0 and 1, then
            // multiply by 1000000000 to get nanoseconds.
            //char* start = remainder;
            pResultTime->tv_nsec = 
                    lround(1000000000L * Util::strtod(rfcTime, remainderStart));
        }
        else
        {
            // No fractional part.
            pResultTime->tv_nsec = 0;
        }

        // Get the timezone offset from UTC. Google Drive appears to use UTC (offset
        // is "Z"), but I don't know whether that's guaranteed. If not using UTC,
        // the offset will start with either '+' or '-'.
        if (remainderStart >= rfcTime.length())
        {
            // Invalid/no timezone offset
            return -1;
        }
        const char tzSep = rfcTime[remainderStart];
        if (tzSep != '+' && tzSep != '-' && toupper(tzSep) != 'Z')
        {
            // Invalid offset.
            return -1;
        }
        if (toupper(tzSep) != 'Z')
        {
            // Get the hour portion of the offset.
            size_t start = remainderStart;
            long offHour = Util::strtol(rfcTime, remainderStart, 10);
            if (remainderStart != start + 2 || rfcTime[remainderStart] != ':')
            {
                // Invalid offset, not in the form of "+HH:MM" / "-HH:MM"
                return -1;
            }

            // Get the minute portion of the offset
            start = remainderStart + 1;
            long offMinute = Util::strtol(rfcTime, remainderStart, 10);
            if (remainderStart != start + 2)
            {
                // Invalid offset, minute isn't a 2-digit number.
                return -1;
            }

            // Subtract the offset from the hour/minute parts of the tm struct.
            // This may give out-of-range values (e.g., tm_hour could be -2 or 26),
            // but mktime() is supposed to handle those.
            epochTime.tm_hour -= offHour;
            epochTime.tm_min -= offMinute;
        }

        // Convert the broken-down time into seconds.
        pResultTime->tv_sec = mktime(&epochTime);

        // Failure if mktime returned -1, success otherwise.
        if (pResultTime->tv_sec == (time_t) (-1))
        {
            return -1;
        }

        // Correct for local timezone, converting back to UTC
        // (Probably unnecessary to call tzset(), but it doesn't hurt)
        tzset();
        pResultTime->tv_sec -= timezone;
        return 0;
    }

    string Fileinfo::gdrive_epoch_timens_to_rfc3339(const struct timespec* ts)
    {
        // A max of 31 (or GDRIVE_TIMESTRING_LENGTH) should be the minimum that will
        // be successful.

        // If nanoseconds were greater than this number, they would be seconds.
        assert(ts->tv_nsec < 1000000000L);

        // Get everything down to whole seconds
        struct tm* pTime = gmtime(&(ts->tv_sec));
        stringstream ss;
        ss << Util::strftime("%Y-%m-%dT%H:%M:%S", pTime);
        
        // strftime() doesn't do fractional seconds. Add the '.', the fractional
        // part, and the 'Z' for timezone.
        ss << "." << setfill('0') << setw(9) << ts->tv_nsec << "Z";
//        int bytesWritten = baseLength;
//        bytesWritten += snprintf(dest + baseLength, max - baseLength, 
//                                 ".%09luZ", ts->tv_nsec);

        return ss.str();

    }

     int Fileinfo::gdrive_finfo_set_time(enum GDRIVE_FINFO_TIME whichTime, 
             const struct timespec* ts)
    {
        TestStop();
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
     
     void Fileinfo::TestStop() const
     {
         if (&gInfo == NULL)
         {
             throw new exception();
         }
     }
}