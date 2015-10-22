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
        
        static const unsigned int GDRIVE_TIMESTRING_LENGTH;
        
        /*
         * gdrive_finfo_get_by_id():    Retrieves a Gdrive_Fileinfo struct describing
         *                              the file corresponding to a given Google Drive
         *                              file ID.
         * Parameters:
         *      fileId (const char*):
         *              The Google Drive file ID of the file for which to get 
         *              information.
         * Return value (const Gdrive_Fileinfo*):
         *      A pointer to a Gdrive_Fileinfo struct describing the file. The memory
         *      at the pointed-to location should not be freed or altered.
         */
        // TODO: This may belong elsewhere.
        static const Fileinfo& gdrive_finfo_get_by_id(fusedrive::Gdrive& gInfo, 
            const std::string& fileId);

        /*
         * gdrive_finfo_cleanup():  Safely frees any memory pointed to by members of a
         *                          Gdrive_Fileinfo struct, then sets all the members to
         *                          0 or NULL. Does NOT free the struct itself.
         * Parameters:
         *      pFileinfo (Gdrive_Fileinfo*):
         *              A pointer to the Gdrive_Fileinfo struct to clear.
         */
        void gdrive_finfo_cleanup();


        /*************************************************************************
         * Getter and setter functions
         *************************************************************************/

        /*
         * gdrive_finfo_get_atime_string(): Retrieve a file's access time as a string
         *                                  in RFC3339 format.
         * Parameters:
         *      pFileinfo (Gdrive_Fileinfo*):
         *              The Gdrive_Fileinfo struct from which to retrieve the time.
         *      dest (char*):
         *              A destination buffer which will receive the time string. This
         *              should already be allocated with at least max bytes.
         *      max (size_t):
         *              The maximum number of bytes, including the terminating null, to
         *              place into dest.
         * Return value (int):
         *      The number of bytes that were placed in dest, excluding the terminating
         *      null.
         */
        std::string gdrive_finfo_get_atime_string();

        /*
         * gdrive_finfo_set_atime():    Set the access time in a Gdrive_Fileinfo struct.
         * Parameters:
         *      pFileinfo (Gdrive_Fileinfo*):
         *              A pointer to the struct whose time should be set.
         *      ts (const struct timespec*):
         *              A pointer to a timespec struct representing the time.
         * Return value:
         *      0 on success, non-zero on failure.
         */
        int gdrive_finfo_set_atime(const struct timespec* ts);

        /*
         * gdrive_finfo_get_atime_string(): Retrieve a file's creation time as a string
         *                                  in RFC3339 format.
         * Parameters:
         *      pFileinfo (Gdrive_Fileinfo*):
         *              The Gdrive_Fileinfo struct from which to retrieve the time.
         *      dest (char*):
         *              A destination buffer which will receive the time string. This
         *              should already be allocated with at least max bytes.
         *      max (size_t):
         *              The maximum number of bytes, including the terminating null, to
         *              place into dest.
         * Return value (int):
         *      The number of bytes that were placed in dest, excluding the terminating
         *      null.
         */
        std::string gdrive_finfo_get_ctime_string();

        /*
         * gdrive_finfo_get_atime_string(): Retrieve a file's modification time as a 
         *                                  string in RFC3339 format.
         * Parameters:
         *      pFileinfo (Gdrive_Fileinfo*):
         *              The Gdrive_Fileinfo struct from which to retrieve the time.
         *      dest (char*):
         *              A destination buffer which will receive the time string. This
         *              should already be allocated with at least max bytes.
         *      max (size_t):
         *              The maximum number of bytes, including the terminating null, to
         *              place into dest.
         * Return value (int):
         *      The number of bytes that were placed in dest, excluding the terminating
         *      null.
         */
        std::string gdrive_finfo_get_mtime_string();

        /*
         * gdrive_finfo_set_atime():    Set the modification time in a Gdrive_Fileinfo 
         *                              struct.
         * Parameters:
         *      pFileinfo (Gdrive_Fileinfo*):
         *              A pointer to the struct whose time should be set.
         *      ts (const struct timespec*):
         *              A pointer to a timespec struct representing the time.
         * Return value:
         *      0 on success, non-zero on failure.
         */
        int gdrive_finfo_set_mtime(const struct timespec* ts);


        /*************************************************************************
         * Other accessible functions
         *************************************************************************/

        /*
         * gdrive_finfo_read_json():    Fill the given Gdrive_Fileinfo struct with 
         *                              information contained in a JSON object.
         * Parameters:
         *      pFileinfo (Gdrive_Fileinfo*):
         *              A pointer to the fileinfo struct to fill.
         *      pObj (gdrive_json_object*):
         *              A JSON object representing a File resource on Google Drive.
         */
        // TODO: Should this be a constructor?
        void gdrive_finfo_read_json(Json& jsonObj);

        /*
         * gdrive_finfo_real_perms():   Retrieve the actual effective permissions for
         *                              the file described by a given Gdrive_Fileinfo
         *                              struct.
         * Parameters:
         *      pFileinfo (const Gdrive_Fileinfo*):
         *              Pointer to an existing Gdrive_Fileinfo struct that has at least 
         *              the basePermission and type members filled in.
         * Return value (unsigned int):
         *      An integer value from 0 to 7 representing Unix filesystem permissions
         *      for the file. A permission needs to be present in both the Google Drive
         *      user's roles for the particular file and the overall access mode for the
         *      system. For example, if the Google Drive user has the owner role (both
         *      read and write access), but the system only has GDRIVE_ACCESS_READ, the
         *      returned value will be 4 (read access only).
         */
        unsigned int gdrive_finfo_real_perms() const;

        
        Fileinfo(Gdrive& gInfo);
        
        virtual ~Fileinfo();
        
    private:
        enum GDRIVE_FINFO_TIME
        {
            GDRIVE_FINFO_ATIME,
            GDRIVE_FINFO_MTIME
        };
        
        static const std::string GDRIVE_MIMETYPE_FOLDER;
        
        static int gdrive_rfc3339_to_epoch_timens(const std::string& rfcTime, 
            struct timespec* pResultTime);

        static std::string 
        gdrive_epoch_timens_to_rfc3339(const struct timespec* ts);

        int gdrive_finfo_set_time(enum GDRIVE_FINFO_TIME whichTime, 
                                         const struct timespec* ts);
        
        Fileinfo(const Fileinfo& orig);
        
        
        
        void TestStop() const;
    };
    
    

}

#endif	/* FILEINFO_HPP */

