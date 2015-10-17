/* 
 * File:   Gdrive.hpp
 * Author: me
 *
 * Created on October 15, 2015, 1:21 AM
 */

#ifndef GDRIVE_HPP
#define	GDRIVE_HPP

namespace fusedrive
{
    enum Gdrive_Interaction
    {
        GDRIVE_INTERACTION_NEVER,
        GDRIVE_INTERACTION_STARTUP,
        GDRIVE_INTERACTION_ALWAYS
    };

    enum Gdrive_Filetype
    {
        // May add a Google Docs file type or others
        GDRIVE_FILETYPE_FILE,
        GDRIVE_FILETYPE_FOLDER
    };
}

//#include "GdriveInfo.hpp"
#include "gdrive-fileinfo.hpp"
#include "gdrive-fileinfo-array.hpp"
#include "gdrive-sysinfo.hpp"
#include "gdrive-file.hpp"

#include <string>


#include "gdrive-transfer.hpp"
#include "gdrive-util.h"
#include "gdrive-download-buffer.hpp"
//#include "gdrive-query.hpp"
//#include "gdrive.h"
#include "gdrive-fileinfo-array.hpp"
    
#include <curl/curl.h>

namespace fusedrive
{
    class Gdrive
    {
    public:
        /*
         * Access levels for Google Drive files.
         * GDRIVE_ACCESS_META:  File metadata only. Will enable directory listing, but
         *                      cannot open files.
         * GDRIVE_ACCESS_READ:  Read-only access to files. Implies GDRIVE_ACCESS_META.
         * GDRIVE_ACCESS_WRITE: Full read-write access to files. Implies
         *                      GDRIVE_ACCESS_READ and GDRIVE_ACCESS_META.
         * GDRIVE_ACCESS_APPS:  Read-only access to list of installed Google Drive apps.
         * GDRIVE_ACCESS_ALL:   Convenience value, includes all of the above.
         */
        static const unsigned int GDRIVE_ACCESS_META;
        static const unsigned int GDRIVE_ACCESS_READ;
        static const unsigned int GDRIVE_ACCESS_WRITE;
        static const unsigned int GDRIVE_ACCESS_APPS;
        static const unsigned int GDRIVE_ACCESS_ALL;

        static const unsigned long GDRIVE_BASE_CHUNK_SIZE;
        
        
        Gdrive(int access, std::string authFilename, time_t cacheTTL, 
                enum Gdrive_Interaction interactionMode, 
                size_t minFileChunkSize, int maxChunksPerFile, bool initCurl=true);
        
        size_t gdrive_get_minchunksize(void);

        int gdrive_get_maxchunks(void);

        int gdrive_get_filesystem_perms(enum Gdrive_Filetype type);


        Gdrive_Fileinfo_Array*  gdrive_folder_list(const std::string& folderId);

        std::string gdrive_filepath_to_id(const std::string& path);

        int gdrive_remove_parent(const std::string& fileId, 
            const std::string& parentId);

        int gdrive_delete(const std::string& fileId, const std::string& parentId);

        int gdrive_add_parent(const std::string& fileId, const std::string& parentId);

        int gdrive_change_basename(const std::string& fileId, const std::string& newName);
        
        ~Gdrive();
        
        /**************************************
         * Members and functions below here are 
         * intended for internal use
         **************************************/
        
        static const std::string GDRIVE_URL_FILES;
        static const std::string GDRIVE_URL_UPLOAD;
        static const std::string GDRIVE_URL_ABOUT;
        static const std::string GDRIVE_URL_CHANGES;
        
        CURL* gdrive_get_curlhandle();

        const std::string& gdrive_get_access_token();

        int gdrive_auth();
        
    private:
        //GdriveInfo gdriveInfo;
        
        static const std::string GDRIVE_REDIRECT_URI;

        static const std::string GDRIVE_FIELDNAME_ACCESSTOKEN;
        static const std::string GDRIVE_FIELDNAME_REFRESHTOKEN;
        static const std::string GDRIVE_FIELDNAME_CODE;
        static const std::string GDRIVE_FIELDNAME_CLIENTID;
        static const std::string GDRIVE_FIELDNAME_CLIENTSECRET;
        static const std::string GDRIVE_FIELDNAME_GRANTTYPE;
        static const std::string GDRIVE_FIELDNAME_REDIRECTURI;

        static const std::string GDRIVE_GRANTTYPE_CODE;
        static const std::string GDRIVE_GRANTTYPE_REFRESH;

        static const std::string GDRIVE_URL_AUTH_TOKEN;
        static const std::string GDRIVE_URL_AUTH_TOKENINFO;
        static const std::string GDRIVE_URL_AUTH_NEWAUTH;
        // GDRIVE_URL_FILES, GDRIVE_URL_ABOUT, and GDRIVE_URL_CHANGES defined in 
        // gdrive-internal.h because they are used elsewhere

        static const std::string GDRIVE_SCOPE_META;
        static const std::string GDRIVE_SCOPE_READ;
        static const std::string GDRIVE_SCOPE_WRITE;
        static const std::string GDRIVE_SCOPE_APPS;
        static const unsigned int GDRIVE_SCOPE_MAXLENGTH;

        static const unsigned int GDRIVE_RETRY_LIMIT;


        static const unsigned int GDRIVE_ACCESS_MODE_COUNT;
        static const unsigned int GDRIVE_ACCESS_MODES[];
        static const std::string GDRIVE_ACCESS_SCOPES[];

        
        
        size_t minChunkSize;
        int maxChunks;
        
        int mode;
        bool userInteractionAllowed;
        std::string authFilename;
        std::string accessToken;
        std::string refreshToken;
        // accessTokenLength and refreshTokenLenth are the allocated sizes, not the
        // actual size of the strings.
        std::string clientId;
        std::string clientSecret;
        std::string redirectUri;
        bool needsCurlCleanup;
        CURL* curlHandle;
        
        void initWithCurl(int access, time_t cacheTTL, 
            enum Gdrive_Interaction interactionMode, size_t minFileChunkSize);
        
        void initNoCurl(int access, time_t cacheTTL, 
            enum Gdrive_Interaction interactionMode, size_t minFileChunkSize);
        
        int gdrive_read_auth_file(const std::string& filename);
        
        int gdrive_refresh_auth_token(const std::string& grantType, 
        const std::string& tokenString);

        int gdrive_prompt_for_auth();

        int gdrive_check_scopes();

        std::string gdrive_get_root_folder_id();

        std::string gdrive_get_child_id_by_name(const std::string& parentId, 
        const std::string& childName);

        int gdrive_save_auth();

        void gdrive_curlhandle_setup(CURL* curlHandle);
        
        
        
        Gdrive(const Gdrive& orig);
    };
    
}

#endif	/* GDRIVE_HPP */

