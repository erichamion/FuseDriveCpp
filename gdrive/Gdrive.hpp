/* 
 * File:   Gdrive.hpp
 * Author: me
 *
 * Created on October 15, 2015, 1:21 AM
 */

#ifndef GDRIVE_HPP
#define	GDRIVE_HPP

#include "GdriveEnums.hpp"

#include "FileinfoArray.hpp"
#include "Sysinfo.hpp"
#include "Cache.hpp"
//#include "GdriveFile.hpp"

#include <string>


#include "HttpTransfer.hpp"
#include "gdrive-util.h"
#include "DownloadBuffer.hpp"

    
#include <curl/curl.h>

namespace fusedrive
{
    class Cache;
    class GdriveFile;
    
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
        
        size_t getMinChunkSize(void) const;

        int getMaxChunks(void) const;

        int getFilesystemPermissions(enum Gdrive_Filetype type);

        int ListFolderContents(const std::string& folderId, 
            FileinfoArray& fileArray);

        std::string getFileIdFromPath(const std::string& path);
        
        const Fileinfo& getFileinfoById(const std::string& fileId);
        
        GdriveFile* openFile(const std::string& fileId, int flags, int& error);

        std::string createFile(const std::string& path, bool createFolder, 
            int& error);

        int removeParent(const std::string& fileId, 
            const std::string& parentId);

        int deleteFile(const std::string& fileId, const std::string& parentId);

        int addParent(const std::string& fileId, const std::string& parentId);

        int changeBasename(const std::string& fileId, const std::string& newName);
        
        Sysinfo& sysinfo();
        
        ~Gdrive();
        
        /**************************************
         * Members and functions below here are 
         * intended for internal use
         **************************************/
        
        static const std::string GDRIVE_URL_FILES;
        static const std::string GDRIVE_URL_UPLOAD;
        static const std::string GDRIVE_URL_ABOUT;
        static const std::string GDRIVE_URL_CHANGES;
        
        CURL* getCurlHandle();
        
        std::string syncMetadataOrCreate(Fileinfo* pFileinfo, 
            const std::string& parentId, const std::string& filename, 
            bool isFolder, int& error);
        
        Cache& getCache();

        const std::string& getAccessToken();

        int authenticate();
        
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

        
        
        size_t mMinChunkSize;
        int mMaxChunks;
        
        int mMode;
        bool mUserInteractionAllowed;
        std::string mAuthFilename;
        std::string mAccessToken;
        std::string mRefreshToken;
        std::string mClientId;
        std::string mClientSecret;
        std::string mRedirectUri;
        bool mNeedsCurlCleanup;
        CURL* mCurlHandle;
        Cache mCache;
        Sysinfo mSysinfo;
        
        void initWithCurl(int access, time_t cacheTTL, 
            enum Gdrive_Interaction interactionMode, size_t minFileChunkSize);
        
        void initNoCurl(int access, time_t cacheTTL, 
            enum Gdrive_Interaction interactionMode, size_t minFileChunkSize);
        
        int readAuthFile(const std::string& filename);
        
        int refreshAuthToken(const std::string& grantType, 
            const std::string& tokenString);

        int promptForAuth();

        int checkScopes();

        std::string getChildFileId(const std::string& parentId, 
            const std::string& childName);

        int writeAuthCredentials();

        void SetupCurlHandle(CURL* curlHandle);
        
        
        
        Gdrive(const Gdrive& orig);
    };
    
}

#endif	/* GDRIVE_HPP */

