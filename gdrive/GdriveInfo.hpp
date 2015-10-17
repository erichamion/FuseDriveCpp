/* 
 * File:   GdriveInfo.hpp
 * Author: me
 *
 * Created on October 15, 2015, 8:47 AM
 */
foo bar;

#define IGNOREME
#ifndef IGNOREME

#ifndef GDRIVEINFO_HPP
#define	GDRIVEINFO_HPP

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

#include "gdrive-transfer.hpp"
#include "gdrive-util.h"
#include "gdrive-download-buffer.hpp"
//#include "gdrive-query.hpp"
//#include "gdrive.h"
#include "gdrive-fileinfo-array.hpp"
    
#include <curl/curl.h>
#include <string>

namespace fusedrive
{
    class GdriveInfo : Gdrive {
    public:
        static const std::string GDRIVE_URL_FILES;
        static const std::string GDRIVE_URL_UPLOAD;
        static const std::string GDRIVE_URL_ABOUT;
        static const std::string GDRIVE_URL_CHANGES;
        
        GdriveInfo(int access, const std::string& authFilename, time_t cacheTTL, 
                enum Gdrive_Interaction interactionMode, 
                size_t minFileChunkSize, int maxChunksPerFile, bool initCurl);
        
        virtual ~GdriveInfo();
        
        //size_t gdrive_get_minchunksize();

        //int gdrive_get_maxchunks();

        //int gdrive_get_filesystem_perms(enum Gdrive_Filetype type);

        //Gdrive_Fileinfo_Array*  gdrive_folder_list(const std::string& folderId);

        //std::string gdrive_filepath_to_id(const std::string& path);

//        int gdrive_remove_parent(const std::string& fileId, 
//            const std::string& parentId);

        //int gdrive_delete(const std::string& fileId, const std::string& parentId);

        //int gdrive_add_parent(const std::string& fileId, const std::string& parentId);

        //int gdrive_change_basename(const std::string& fileId, const std::string& newName);
        
        

        /******************
         * Semi-public getter and setter functions
         ******************/

        /*
         * gdrive_get_curlhandle(): Retrieves a duplicate of the curl easy handle 
         *                          currently stored in the Gdrive_Info struct.
         * Return value (CURL*):
         *      A curl easy handle. The caller is responsible for calling 
         *      curl_easy_cleanup() on this handle.
         * NOTES:
         *      The Gdrive_Info struct stores a curl easy handle with several options
         *      pre-set. In order to maintain consistency and avoid corrupting the
         *      handle's options (as could happen if an option intended for one specific
         *      operation is not reset to the default after the operation finishes),
         *      this function returns a new copy of the handle, not a reference to the
         *      existing handle.
         */
        CURL* gdrive_get_curlhandle();

        /*
         * gdrive_get_access_token():   Retrieve the current access token.
         * Return value (const char*):
         *      A pointer to a null-terminated string, or a NULL pointer if there is no
         *      current access token. The pointed-to memory should not be altered or
         *      freed.
         */
        const std::string& gdrive_get_access_token();


        /******************
         * Other semi-public accessible functions
         ******************/

        /*
         * gdrive_auth():   Authenticate and obtain permissions from the user for Google
         *                  Drive.  If passed the address of a Gdrive_Info struct which 
         *                  has existing authentication information, will attempt to 
         *                  reuse this information first. The new credentials (if 
         *                  different from the credentials initially passed in) are
         *                  written back into the Gdrive_Info struct.
         * Returns:
         *      0 for success, other value on error.
         */
        int gdrive_auth();
        
        
    private:
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
        
        GdriveInfo(const GdriveInfo& orig);

    };

}

#endif	/* GDRIVEINFO_HPP */

#endif  /* IGNOREME */
#undef IGNOREME