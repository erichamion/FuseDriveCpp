



#include "Gdrive.hpp"
#include "GdriveFile.hpp"
#include "Cache.hpp"
#include "HttpQuery.hpp"
#include "Util.hpp"
#include "Json.hpp"


#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <string>
#include <sstream>
#include <exception>
#include <fcntl.h>

#include "gdrive-client-secret.h"


using namespace std;

namespace fusedrive
{
    /**************************
    * Public Constants
    **************************/
    
    const unsigned int Gdrive::GDRIVE_ACCESS_META = 0x01;
    const unsigned int Gdrive::GDRIVE_ACCESS_READ = 0x02;
    const unsigned int Gdrive::GDRIVE_ACCESS_WRITE = 0x04;
    const unsigned int Gdrive::GDRIVE_ACCESS_APPS = 0x08;
    const unsigned int Gdrive::GDRIVE_ACCESS_ALL = 0x0F;
    
    const unsigned long Gdrive::GDRIVE_BASE_CHUNK_SIZE = 262144L;
    
    /**************************
    * Public Methods
    **************************/
    
    Gdrive::Gdrive(int access, std::string authFilename, time_t cacheTTL, 
            enum Gdrive_Interaction interactionMode, 
            size_t minFileChunkSize, int maxChunksPerFile, bool initCurl) 
    : mMaxChunks(maxChunksPerFile), mAuthFilename(authFilename), 
            mNeedsCurlCleanup(initCurl), mCurlHandle(NULL), 
            mCache(*this, cacheTTL), mSysinfo(*this)
    {
        if (initCurl)
        {
            initWithCurl(access, cacheTTL, interactionMode, minFileChunkSize);
        }
        else
        {
            initNoCurl(access, cacheTTL, interactionMode, minFileChunkSize);
        }
    }
    
//    Gdrive::Gdrive(int access, std::string authFilename, time_t cacheTTL, 
//            enum Gdrive_Interaction interactionMode, 
//            size_t minFileChunkSize, int maxChunksPerFile, bool initCurl)
//    {
//        
//    }
    
    size_t Gdrive::getMinChunkSize(void) const
    {
        return mMinChunkSize;
    }
    
    int Gdrive::getMaxChunks() const
    {
        return mMaxChunks;
    }

    int Gdrive::getFilesystemPermissions(enum Gdrive_Filetype type)
    {
        // Get the permissions for regular files.
        int perms = 0;
        if (mMode & Gdrive::GDRIVE_ACCESS_READ)
        {
            // Read access
            perms = perms | S_IROTH;
        }
        if (mMode & Gdrive::GDRIVE_ACCESS_WRITE)
        {
            // Write access
            perms = perms | S_IWOTH;
        }
        // No execute access (at least for regular files)

        // Folders always need read and execute access.
        if (type == GDRIVE_FILETYPE_FOLDER)
        {
            perms = perms | S_IROTH | S_IXOTH;
        }

        return perms;
    }


    int Gdrive::ListFolderContents(const string& folderId, 
            FileinfoArray& fileArray)
    {
        // Allow for an initial quote character in addition to the terminating null
        string filter = string("'") + folderId + 
                string("' in parents and trashed=false");

        // Prepare the network request
        HttpTransfer xfer(*this);
        xfer.setRequestType(HttpTransfer::GET)
            .setUrl(GDRIVE_URL_FILES)
            .addQuery("q", filter)
            .addQuery("fields", "items(title,id,mimeType)");

        // Send the network request
        if (xfer.execute() != 0)
        {
            // Request failed
            return -1;
        };


        // TODO: Somehow unify this process with other ways to fill Gdrive_Fileinfo,
        // reducing code duplication and taking advantage of the cache.
        int fileCount = -1;
        if (xfer.getHttpResponse() < 400)
        {
            // Transfer was successful.  Convert result to a JSON object and extract
            // the file meta-info.
            Json jsonObj(xfer.getData());
            if (jsonObj.isValid())
            {
                bool dummy;
                fileCount = jsonObj.getArrayLength("items", dummy);
                if (fileCount > 0)
                {
                    // Extract Fileinfo for each item and insert into a 
                    // FileinfoArray.

                    // Extract the file info from each member of the array.
                    for (int index = 0; index < fileCount; index++)
                    {
                        Json jsonFile = 
                                jsonObj.arrayGet("items",
                                index);
                        if (jsonFile.isValid())
                        {
                            fileArray.addFromJson(jsonFile);
                        }
                    }
                }
                // else either failure (return -1) or 0-length array (return 0),
                // nothing special needs to be done.

            }
            // else do nothing.  Already prepared to return error (-1).
        }

        return fileCount;
    }


    string Gdrive::getFileIdFromPath(const string& path)
    {
        if (path.empty())
        {
            // Invalid path
            return string("");
        }

        // Try to get the ID from the cache.
        string cachedId(mCache.getFileid(path));
        if (!cachedId.empty())
        {
            return cachedId;
        }
        // else ID isn't in the cache yet

        // Is this the root folder?
        string result = string();
        if (path.compare("/") == 0)
        {
            result = mSysinfo.rootId();
            if (!result.empty())
            {
                // Add to the fileId cache.
                mCache.addFileid(path, result);
            }
            return result;
        }

        // Not in cache, and not the root folder.  Some part of the path may be
        // cached, and some part MUST be the root folder, so recursion seems like
        // the easiest solution here.

        // Find the last '/' character (ignoring any trailing slashes, which we
        // shouldn't get anyway). Everything before it is the parent, and everything
        // after is the child.
        int index;
        // Ignore trailing slashes
        for (index = path.length() - 1; path[index] == '/'; index--)
        {
            // Empty loop
        }
        // Find the last '/' before the current index.
        index = path.rfind('/', index);
        

        // Find the parent's fileId.

        // Normally don't include the '/' at the end of the path, EXCEPT if we've
        // reached the start of the string. We expect to see "/" for the root
        // directory, not an empty string.
        int parentLength = (index != 0) ? index : 1;
        string parentPath = path.substr(0, parentLength);
        string childName = path.substr(index + 1);
        
        string parentId = getFileIdFromPath(parentPath);
        if (parentId.empty())
        {
            // An error occurred.
            return string("");
        }
        // Use the parent's ID to find the child's ID.
        string childId = getChildFileId(parentId, childName);

        // Add the ID to the fileId cache.
        if (!childId.empty())
        {
            mCache.addFileid(path, childId);
        }
        
        return childId;
    }

    const Fileinfo& Gdrive::getFileinfoById(const string& fileId)
    {
        // Get the information from the cache, or put it in the cache if it isn't
        // already there.
        bool alreadyCached = false;

        Fileinfo* pFileinfo = 
                getCache().getItem(fileId, true, alreadyCached);
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
        HttpTransfer xfer(*this);
        
        xfer.setRequestType(HttpTransfer::GET);

        // Add the URL.
        // String to hold the url.  Add 2 to the end to account for the '/' before
        // the file ID, as well as the terminating null.
        string baseUrl(Gdrive::GDRIVE_URL_FILES);
        baseUrl += "/";
        baseUrl += fileId;
        
        xfer.setUrl(baseUrl)
            .addQuery("fields", "title,id,mimeType,fileSize,"
                                    "createdDate,modifiedDate,"
                                    "lastViewedByMeDate,parents(id),"
                                    "userPermission");

        // Perform the request
        if (xfer.execute() != 0)
        {
            // Download error
            throw new exception();
        }

        if (xfer.getHttpResponse() >= 400)
        {
            // Server returned an error that couldn't be retried, or continued
            // returning an error after retrying
            throw new exception();
        }

        // If we're here, we have a good response.  Extract the ID from the 
        // response.

        // Convert to a JSON object.
        Json jsonObj(xfer.getData());
        if (!jsonObj.isValid())
        {
            // Couldn't convert to Json object
            throw new exception();
        }
        
        pFileinfo->readJson(jsonObj);

        // If it's a folder, get the number of children.
        if (pFileinfo->type == GDRIVE_FILETYPE_FOLDER)
        {
            FileinfoArray fileArray(*this);
            if (ListFolderContents(fileId, fileArray) >= 0)
            {

                pFileinfo->nChildren = fileArray.count();
            }
        }
        return *pFileinfo;
    }
    
    GdriveFile* Gdrive::openFile(const std::string& fileId, int flags, 
            int& error)
    {
        assert(!fileId.empty());

        // Get the cache node from the cache if it exists.  If it doesn't exist,
        // don't make a node with an empty Gdrive_Fileinfo.  Instead, use 
        // gdrive_file_info_from_id() to create the node and fill out the struct, 
        // then try again to get the node.
        CacheNode* pNode;
        while ((pNode = mCache.getNode(fileId, false)) 
                == NULL)
        {
            try
            {
                getFileinfoById(fileId);
            }
            catch (const exception& e)
            {
                // Problem getting the file info.  Return failure.
                error = ENOENT;
                throw new exception();
            }
        }

        // If the file is deleted, existing filehandles will still work, but nobody
        // new can open it.
        if (pNode->isDeleted())
        {
            error = ENOENT;
            throw new exception();
        }

        // Don't open directories, only regular files.
        if (pNode->getFileinfo().type == GDRIVE_FILETYPE_FOLDER)
        {
            // Return failure
            error = EISDIR;
            throw new exception();
        }


        if (!pNode->checkPermissions(flags))
        {
            // Access error
            error = EACCES;
            throw new exception();
        }


        // Increment the open counter
        pNode->incrementOpenCount((flags & O_WRONLY) || (flags & O_RDWR));

        // Return file handle containing the cache node
        return openFileHelper(*pNode);
    }

    string Gdrive::createFile(const string& path, bool createFolder, 
            int& error)
    {
        assert(!path.empty() && path[0] == '/');
            
        // Separate path into basename and parent folder.
        Gdrive_Path* pGpath = gdrive_path_create(path.c_str());
        if (pGpath == NULL)
        {
            // Memory error
            error = ENOMEM;
            return "";
        }
        const char* folderName = gdrive_path_get_dirname(pGpath);
        const char* filename = gdrive_path_get_basename(pGpath);

        // Check basename for validity (non-NULL, not a directory link such as "..")
        if (filename == NULL || filename[0] == '/' || 
                strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
        {
            error = EISDIR;
            gdrive_path_free(pGpath);
            return "";
        }

        // Check folder for validity (non-NULL, starts with '/', and is an existing
        // folder)
        if (folderName == NULL || folderName[0] != '/')
        {
            // Path wasn't in the form of an absolute path
            error = ENOTDIR;
            gdrive_path_free(pGpath);
            return "";
        }
        string parentId = getFileIdFromPath(folderName);
        if (parentId.empty())
        {
            // Folder doesn't exist
            error = ENOTDIR;
            gdrive_path_free(pGpath);
            return "";
        }
        CacheNode* pFolderNode = 
                mCache.getNode(parentId, true);
        if (pFolderNode == NULL)
        {
            // Couldn't get a node for the parent folder
            error = EIO;
            gdrive_path_free(pGpath);
            return "";
        }
        const Fileinfo& folderinfo = pFolderNode->getFileinfo();
        if (folderinfo.type != GDRIVE_FILETYPE_FOLDER)
        {
            // Not an actual folder
            error = ENOTDIR;
            gdrive_path_free(pGpath);
            return "";
        }

        // Make sure we have write access to the folder
        if (!pFolderNode->checkPermissions(O_WRONLY))
        {
            // Don't have the needed permission
            error = EACCES;
            gdrive_path_free(pGpath);
            return "";
        }


        string fileId = syncMetadataOrCreate(NULL, parentId, filename, 
                createFolder, error);
        gdrive_path_free(pGpath);

        // TODO: See if gdrive_cache_add_fileid() can be modified to return a 
        // pointer to the cached ID (which is a new copy of the ID that was passed
        // in). This will avoid the need to look up the ID again after adding it,
        // and it will also help with multiple files that have identical paths.
        int result = mCache.addFileid(path, fileId);
        if (result != 0)
        {
            // Probably a memory error
            error = ENOMEM;
            return "";
        }

        return getFileIdFromPath(path);
    }

    int Gdrive::removeParent(const string& fileId, const string& parentId)
    {
        assert(!(fileId.empty() || parentId.empty()));

        // Need write access
        if (!(mMode & Gdrive::GDRIVE_ACCESS_WRITE))
        {
            return -EACCES;
        }

        // URL will look like 
        // "<standard Drive Files url>/<fileId>/parents/<parentId>"
        string url = GDRIVE_URL_FILES + "/" + fileId + "/parents/" + parentId;

        HttpTransfer xfer(*this);
        
        xfer.setUrl(url) 
            .setRequestType(HttpTransfer::DELETE);

        int result = xfer.execute();

        return (result != 0 || xfer.getHttpResponse() >= 400) ? -EIO : 0;
    }


    int Gdrive::deleteFile(const string& fileId, const string& parentId)
    {
        // TODO: If support for manipulating trashed files is added, we'll need to
        // check whether the specified file is already trashed, and permanently 
        // delete if it is.
        // TODO: May want to add an option for whether to trash or really delete.

        assert(!fileId.empty());

        // Need write access
        if (!(mMode & Gdrive::GDRIVE_ACCESS_WRITE))
        {
            return -EACCES;
        }

        // URL will look like 
        // "<standard Drive Files url>/<fileId>/trash"
        string url = GDRIVE_URL_FILES + "/" + fileId + "/trash";

        HttpTransfer xfer(*this);
        
        xfer.setUrl(url)
            .setRequestType(HttpTransfer::POST);

        int result = xfer.execute();

        int returnVal = (result != 0 || xfer.getHttpResponse() >= 400) ? 
            -EIO : 0;
        
        if (returnVal == 0)
        {
            mCache.deleteId(fileId);
            if (!parentId.empty() && parentId.compare("/") != 0)
            {
                // Remove the parent from the cache because the child count will be
                // wrong.
                mCache.deleteId(parentId);
            }
        }
        return returnVal;
    }


    int Gdrive::addParent(const string& fileId, const string& parentId)
    {
        assert(!(fileId.empty() || parentId.empty()));

        // Need write access
        if (!(mMode & Gdrive::GDRIVE_ACCESS_WRITE))
        {
            return -EACCES;
        }

        // URL will look like 
        // "<standard Drive Files url>/<fileId>/parents"
        string url = GDRIVE_URL_FILES + "/" + fileId + "/parents";

        // Create the Parent resource for the request body
        Json jsonObj;
        jsonObj.addString("id", parentId);
        string body = string(jsonObj.toString(false));
        

        HttpTransfer xfer(*this);
        
        xfer.setUrl(url)
            .addHeader("Content-Type: application/json")
            .setRequestType(HttpTransfer::POST)
            .setBody(body);

        int result = xfer.execute();

        int returnVal = (result != 0 || xfer.getHttpResponse() >= 400) ? 
            -EIO : 0;

        if (returnVal == 0)
        {
            // Update the parent count in case we do anything else with the file
            // before the cache expires. (For example, if there was only one parent
            // before, and the user deletes one of the links, we don't want to
            // delete the entire file because of a bad parent count).
            Fileinfo* pFileinfo = mCache.getItem(fileId, false);
            if (pFileinfo)
            {
                pFileinfo->nParents++;
            }
        }
        return returnVal;
    }

    int Gdrive::changeBasename(const string& fileId, const string& newName)
    {
        assert(!(fileId.empty() || newName.empty()));

        // Need write access
        if (!(mMode & Gdrive::GDRIVE_ACCESS_WRITE))
        {
            return -EACCES;
        }

        // Create the request body with the new name
        Json jsonObj;
        jsonObj.addString("title", newName);
        string body(jsonObj.toString(false));

        // Create the url in the form of:
        // "<GDRIVE_URL_FILES>/<fileId>"
        string url = GDRIVE_URL_FILES + "/" + fileId;

        // Set up the network transfer
        HttpTransfer xfer(*this);
        
        xfer.setUrl(url)
            .addQuery("updateViewedDate", "false")
            .addHeader("Content-Type: application/json")
            .setBody(body)
            .setRequestType(HttpTransfer::PATCH);

        // Send the network request
        int result = xfer.execute();

        return (result != 0 || xfer.getHttpResponse() >= 400) ? -EIO : 0;
    }
    
    Sysinfo& Gdrive::sysinfo()
    {
        return mSysinfo;
    }

    Gdrive::~Gdrive() 
    {
        if (this->mNeedsCurlCleanup && mCurlHandle != NULL)
        {
            curl_easy_cleanup(mCurlHandle);
        }
    }

    
    
    
    /**************************
    * Semi-Public Constants
    **************************/
    
    const std::string Gdrive::GDRIVE_URL_FILES = 
        "https://www.googleapis.com/drive/v2/files";
    const std::string Gdrive::GDRIVE_URL_UPLOAD =
        "https://www.googleapis.com/upload/drive/v2/files";
    const std::string Gdrive::GDRIVE_URL_ABOUT =
        "https://www.googleapis.com/drive/v2/about";
    const std::string Gdrive::GDRIVE_URL_CHANGES =
        "https://www.googleapis.com/drive/v2/changes";
    
    
    /**************************
    * Private Constants
    **************************/
    
    const std::string Gdrive::GDRIVE_REDIRECT_URI = 
        "urn:ietf:wg:oauth:2.0:oob";

    const std::string Gdrive::GDRIVE_FIELDNAME_ACCESSTOKEN = 
        "access_token";
    const std::string Gdrive::GDRIVE_FIELDNAME_REFRESHTOKEN = 
        "refresh_token";
    const std::string Gdrive::GDRIVE_FIELDNAME_CODE = "code";
    const std::string Gdrive::GDRIVE_FIELDNAME_CLIENTID = "client_id";
    const std::string Gdrive::GDRIVE_FIELDNAME_CLIENTSECRET = 
        "client_secret";
    const std::string Gdrive::GDRIVE_FIELDNAME_GRANTTYPE = "grant_type";
    const std::string Gdrive::GDRIVE_FIELDNAME_REDIRECTURI = 
        "redirect_uri";

    const std::string Gdrive::GDRIVE_GRANTTYPE_CODE = "authorization_code";
    const std::string Gdrive::GDRIVE_GRANTTYPE_REFRESH = "refresh_token";

    const std::string Gdrive::GDRIVE_URL_AUTH_TOKEN = 
        "https://www.googleapis.com/oauth2/v3/token";
    const std::string Gdrive::GDRIVE_URL_AUTH_TOKENINFO = 
        "https://www.googleapis.com/oauth2/v1/tokeninfo";
    const std::string Gdrive::GDRIVE_URL_AUTH_NEWAUTH = 
        "https://accounts.google.com/o/oauth2/auth";
    // GDRIVE_URL_FILES, GDRIVE_URL_ABOUT, and GDRIVE_URL_CHANGES defined in 
    // gdrive-internal.h because they are used elsewhere

    const std::string Gdrive::GDRIVE_SCOPE_META = 
        "https://www.googleapis.com/auth/drive.readonly.metadata";
    const std::string Gdrive::GDRIVE_SCOPE_READ = 
        "https://www.googleapis.com/auth/drive.readonly";
    const std::string Gdrive::GDRIVE_SCOPE_WRITE = 
        "https://www.googleapis.com/auth/drive";
    const std::string Gdrive::GDRIVE_SCOPE_APPS = 
        "https://www.googleapis.com/auth/drive.apps.readonly";
    const unsigned int Gdrive::GDRIVE_SCOPE_MAXLENGTH = 200;

    const unsigned int Gdrive::GDRIVE_RETRY_LIMIT = 5;


    const unsigned int Gdrive::GDRIVE_ACCESS_MODE_COUNT = 4;
    const unsigned int Gdrive::GDRIVE_ACCESS_MODES[] = {
        Gdrive::GDRIVE_ACCESS_META,
        Gdrive::GDRIVE_ACCESS_READ,
        Gdrive::GDRIVE_ACCESS_WRITE,
        Gdrive::GDRIVE_ACCESS_APPS
    };
    const std::string Gdrive::GDRIVE_ACCESS_SCOPES[] = {
        GDRIVE_SCOPE_META, 
        GDRIVE_SCOPE_READ, 
        GDRIVE_SCOPE_WRITE,
        GDRIVE_SCOPE_APPS 
    };
    
    
    /**************************
    * Semi-Public Methods
    **************************/
    
    CURL* Gdrive::getCurlHandle()
    {
        if (mCurlHandle == NULL)
        {
            mCurlHandle = curl_easy_init();
            if (mCurlHandle == NULL)
            {
                // Error
                return NULL;
            }
            SetupCurlHandle(mCurlHandle);
        }
        return curl_easy_duphandle(mCurlHandle);
    }
    
    string Gdrive::syncMetadataOrCreate(Fileinfo* pFileinfo, 
            const std::string& parentId, const std::string& filename, 
            bool isFolder, int& error)
    {
        // For existing file, pFileinfo must be non-NULL. For creating new file,
        // both parentId and filename must be non-empty.
        assert(pFileinfo || !(parentId.empty() || filename.empty()));
        
        bool isReallyFolder = isFolder;
        Fileinfo myFileinfo(*this);
        Fileinfo* pMyFileinfo;
        if (pFileinfo != NULL)
        {
            pMyFileinfo = pFileinfo;
            isReallyFolder = (pMyFileinfo->type == GDRIVE_FILETYPE_FOLDER);
        }
        else
        {
            myFileinfo.filename = filename;
            myFileinfo.type = isReallyFolder ? 
                GDRIVE_FILETYPE_FOLDER : GDRIVE_FILETYPE_FILE;
            struct timespec ts;
            if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
            {
                myFileinfo.creationTime = ts;
                myFileinfo.accessTime = ts;
                myFileinfo.modificationTime = ts;
            }
            // else leave the times at 0 on failure

            pMyFileinfo = &myFileinfo;
        }


        // Set up the file resource as a JSON object
        Json uploadResourceJson;
        if (!uploadResourceJson.isValid())
        {
            error = ENOMEM;
            return NULL;
        }
        uploadResourceJson.addString("title", pMyFileinfo->filename);
        if (pFileinfo == NULL)
        {
            // Only set parents when creating a new file
            Json parentsArray = 
                    uploadResourceJson.addNewArray("parents");
            if (!parentsArray.isValid())
            {
                error = ENOMEM;
                return NULL;
            }
            Json parentIdObj;
            parentIdObj.addString("id", parentId);
            parentsArray.arrayAppendObject(parentIdObj);
        }
        if (isReallyFolder)
        {
            uploadResourceJson.addString("mimeType", 
                    "application/vnd.google-apps.folder");
        }
    //    char* timeString = (char*) malloc(Fileinfo::GDRIVE_TIMESTRING_LENGTH);
    //    if (timeString == NULL)
    //    {
    //        // Memory error
    //        gdrive_json_kill(uploadResourceJson);
    //        *pError = ENOMEM;
    //        return NULL;
    //    }
    //    // Reuse the same timeString for atime and mtime. Can't change ctime.
        string timeString = pMyFileinfo->getAtimeString();
        if (!timeString.empty())
        {
            uploadResourceJson.addString("lastViewedByMeDate", 
                    timeString);
        }

        bool hasMtime = false;
        timeString = pMyFileinfo->getMtimeString();
        if (!timeString.empty())
        {
            uploadResourceJson.addString("modifiedDate", timeString);
            hasMtime = true;
        }

        // Convert the JSON into a string
        string uploadResourceStr = uploadResourceJson.toString(false);
        if (uploadResourceStr.empty())
        {
            error = ENOMEM;
            return NULL;
        }


        // Full URL has '/' and the file ID appended for an existing file, or just
        // the base URL for a new one.
        stringstream url;
        url << GDRIVE_URL_FILES;
        if (pFileinfo)
        {
            // Existing file, need base URL + '/' + file ID
            url << '/' << pMyFileinfo->id;
        }

        // Set up the network request
        HttpTransfer xfer(*this);
        
        // URL, header, and updateViewedDate query parameter always get added. The 
        // setModifiedDate query parameter only gets set when hasMtime is true.
        xfer.setUrl(url.str())
            .addHeader("Content-Type: application/json");
        if (hasMtime)
        {
            xfer.addQuery("setModifiedDate", "true");
        }
        xfer.addQuery("updateViewedDate", "false")
            .setRequestType((pFileinfo != NULL) ? 
                HttpTransfer::PATCH : 
                HttpTransfer::POST)
            .setBody(uploadResourceStr);

        // Do the transfer
        int result = xfer.execute();

        if (result != 0 || xfer.getHttpResponse() >= 400)
        {
            // Transfer was unsuccessful
            error = EIO;
            return NULL;
        }

        // Extract the file ID from the returned resource
        Json jsonObj(xfer.getData());
        if (!jsonObj.isValid())
        {
            // Either memory error, or couldn't convert the response to JSON.
            // More likely memory.
            error = ENOMEM;
            return NULL;
        }
        string fileId = jsonObj.getString("id");
        if (fileId.empty())
        {
            // Either memory error, or couldn't extract the desired string, can't
            // tell which.
            error = EIO;
            return NULL;
        }

        pMyFileinfo->dirtyMetainfo = false;
        return fileId;
    }
    
    Cache& Gdrive::getCache()
    {
        return mCache;
    }
    
    const string& Gdrive::getAccessToken()
    {
        return mAccessToken;
    }

    int Gdrive::authenticate()
    {
        // Try to refresh existing tokens first.
        if (!mRefreshToken.empty())
        {
            int refreshSuccess = refreshAuthToken(
                    GDRIVE_GRANTTYPE_REFRESH, mRefreshToken
            );

            if (refreshSuccess == 0)
            {
                // Refresh succeeded, but we don't know what scopes were previously
                // granted.  Check to make sure we have the required scopes.  If so,
                // then we don't need to do anything else and can return success.
                int success = checkScopes();
                if (success == 0)
                {
                    // Refresh succeeded with correct scopes, return success.
                    return 0;
                }
            }
        }

        // Either didn't have a refresh token, or it didn't work.  Need to get new
        // authorization, if allowed.
        if (!mUserInteractionAllowed)
        {
            // Need to get new authorization, but not allowed to interact with the
            // user.  Return error.
            return -1;
        }

        // If we've gotten this far, then we need to interact with the user, and
        // we're allowed to do so.  Prompt for authorization, and return whatever
        // success or failure the prompt returns.
        return promptForAuth();
    }    

    
    /**************************
    * Private Methods
    **************************/
    
    void Gdrive::initWithCurl(int access, time_t cacheTTL, 
        enum Gdrive_Interaction interactionMode, size_t minFileChunkSize)
    {
        if (curl_global_init(CURL_GLOBAL_ALL) != 0)
        {
            // Curl initialization failed.  For now, throw a generic exception.  
            // Later, we may define different error conditions.
            throw new exception();
        }

        // If we've made it this far, both the Gdrive_Info and its contained
        // Gdrive_Info_Internal exist, and curl has been successfully initialized.
        // Signal that fact in the data structure, and defer the rest of the 
        // processing to gdrive_init_nocurl().
        //isCurlInitialized = true;

        initNoCurl(access, cacheTTL, interactionMode, minFileChunkSize);

    }

    void Gdrive::initNoCurl(int access, time_t cacheTTL, 
        enum Gdrive_Interaction interactionMode, size_t minFileChunkSize)
    {
        // Seed the RNG.
        srand(time(NULL));

        // Set up the Google Drive client ID and secret.
        mClientId = GDRIVE_CLIENT_ID;
        mClientSecret = GDRIVE_CLIENT_SECRET;
        mRedirectUri = GDRIVE_REDIRECT_URI;

        // Can we prompt the user for authentication during initial setup?
        if (interactionMode == GDRIVE_INTERACTION_STARTUP || 
                interactionMode == GDRIVE_INTERACTION_ALWAYS)
        {
            mUserInteractionAllowed = true;
        }

        // If a filename was given, attempt to open the file and read its contents.
        if (!mAuthFilename.empty())
        {
            readAuthFile(mAuthFilename);
        }

        // Authenticate or refresh access
        mMode = access;
        if (mMode & Gdrive::GDRIVE_ACCESS_WRITE) 
        {
            // Write access implies read access
            mMode = mMode | Gdrive::GDRIVE_ACCESS_READ;
        }
        if (mMode & Gdrive::GDRIVE_ACCESS_READ) 
        {
            // Read access implies meta-info access
            mMode = mMode | Gdrive::GDRIVE_ACCESS_META;
        }
        if (authenticate() != 0)
        {
            // Could not get the required permissions.
            throw new exception();
        }
        writeAuthCredentials();
        // Can we continue prompting for authentication if needed later?
        mUserInteractionAllowed = 
                (interactionMode == GDRIVE_INTERACTION_ALWAYS);
        
        mCache.init();
        
        // Set chunk size
        mMinChunkSize = (minFileChunkSize > 0) ? 
            Util::divideCeil(minFileChunkSize, Gdrive::GDRIVE_BASE_CHUNK_SIZE) * 
                Gdrive::GDRIVE_BASE_CHUNK_SIZE :
            Gdrive::GDRIVE_BASE_CHUNK_SIZE;

    }
    
    int Gdrive::readAuthFile(const std::string& filename)
    {
        if (filename.empty())
        {
            // Invalid argument.  For now, return -1 for all errors.
            return -1;
        }


        // Make sure the file exists and is a regular file.
        struct stat st;
        if ((stat(filename.c_str(), &st) == 0) && (st.st_mode & S_IFREG))
        {
            FILE* inFile = fopen(filename.c_str(), "r");
            if (inFile == NULL)
            {
                // Couldn't open file for reading.
                return -1;
            }

            char* buffer = (char*) malloc(st.st_size + 1);
            if (buffer == NULL)
            {
                // Memory allocation error.
                fclose(inFile);
                return -1;
            }

            int bytesRead = fread(buffer, 1, st.st_size, inFile);
            buffer[bytesRead >= 0 ? bytesRead : 0] = '\0';
            int returnVal = 0;

            Json jsonObj = Json(string(buffer));
            if (!jsonObj.isValid())
            {
                // Couldn't convert the file contents to a JSON object, prepare to
                // return failure.
                returnVal = -1;
            }
            else
            {
                mAccessToken.assign(jsonObj.getString( 
                        GDRIVE_FIELDNAME_ACCESSTOKEN));
                mRefreshToken.assign(jsonObj.getString( 
                        GDRIVE_FIELDNAME_REFRESHTOKEN));
                
                if (mAccessToken.empty() || mRefreshToken.empty())
                {
                    // Didn't get one or more auth tokens from the file.
                    returnVal = -1;
                }
            }
            free(buffer);
            fclose(inFile);
            return returnVal;

        }
        else
        {
            // File doesn't exist or isn't a regular file.
            return -1;
        }

    }
        
    int Gdrive::refreshAuthToken(const std::string& grantType, 
    const std::string& tokenString)
    {
        // Make sure we were given a valid grant_type
        if (grantType != GDRIVE_GRANTTYPE_CODE && 
                grantType != GDRIVE_GRANTTYPE_REFRESH)
        {
            // Invalid grant_type
            return -1;
        }

        // Prepare the network request
        HttpTransfer xfer(*this);
        
        xfer.setRequestType(HttpTransfer::POST)
            // We're trying to get authorization, so it doesn't make sense to retry if
            // authentication fails.
            .setRetryOnAuthError(false);

        // Set up the post data. Some of the post fields depend on whether we have
        // an auth code or a refresh token, and some do not.
        string tokenOrCodeField;
        if (grantType == GDRIVE_GRANTTYPE_CODE)
        {
            // Converting an auth code into auth and refresh tokens.  Interpret
            // tokenString as the auth code.
            xfer.addPostField(GDRIVE_FIELDNAME_REDIRECTURI, 
                    GDRIVE_REDIRECT_URI);
            tokenOrCodeField.assign(GDRIVE_FIELDNAME_CODE);
        }
        else
        {
            // Refreshing an existing refresh token.  Interpret tokenString as the
            // refresh token.
            tokenOrCodeField.assign(GDRIVE_FIELDNAME_REFRESHTOKEN);
        }
        xfer.addPostField(tokenOrCodeField, tokenString)
            .addPostField(GDRIVE_FIELDNAME_CLIENTID, 
                GDRIVE_CLIENT_ID)
            .addPostField(GDRIVE_FIELDNAME_CLIENTSECRET,
                GDRIVE_CLIENT_SECRET)
            .addPostField(GDRIVE_FIELDNAME_GRANTTYPE, grantType)
            .setUrl(GDRIVE_URL_AUTH_TOKEN);
        

        // Do the transfer. 
        if (xfer.execute() != 0)
        {
            // There was an error sending the request and getting the response.
            return -1;
        }
        if (xfer.getHttpResponse() >= 400)
        {
            // Failure, but probably not an error.  Most likely, the user has
            // revoked permission or the refresh token has otherwise been
            // invalidated.
            return 1;
        }

        // If we've gotten this far, we have a good HTTP response.  Now we just
        // need to pull the access_token string (and refresh token string if
        // present) out of it.

        Json jsonObj(xfer.getData());
        if (!jsonObj.isValid())
        {
            // Couldn't locate JSON-formatted information in the server's 
            // response.  Return error.
            return -1;
        }
        
//        int returnVal = gdrive_json_realloc_string(
//                pObj, 
//                GDRIVE_FIELDNAME_ACCESSTOKEN,
//                &(pInfo->accessToken),
//                &(pInfo->accessTokenLength)
//                );
        string tmpStr = jsonObj.getString(GDRIVE_FIELDNAME_ACCESSTOKEN);
        if (tmpStr.empty())
        {
            // Couldn't get access token
            return -1;
        }
        mAccessToken.assign(tmpStr);
        
        // Only try to get refresh token if we successfully got the access 
        // token.
        
        // We won't always have a refresh token.  Specifically, if we were
        // already sending a refresh token, we may not get one back.
        // Don't treat the lack of a refresh token as an error or a failure,
        // and don't clobber the existing refresh token if we don't get a
        // new one.

        tmpStr = jsonObj.getString(GDRIVE_FIELDNAME_REFRESHTOKEN);
        if (!tmpStr.empty())
        {
            // We were given a refresh token, so store it.
            mRefreshToken.assign(tmpStr);
        }

        return 0;
    }

    int Gdrive::promptForAuth()
    {
        char scopeStr[GDRIVE_SCOPE_MAXLENGTH] = "";
        bool scopeFound = false;

        // Check each of the possible permissions, and add the appropriate scope
        // if necessary.
        for (unsigned int i = 0; i < GDRIVE_ACCESS_MODE_COUNT; i++)
        {
            if (mMode & GDRIVE_ACCESS_MODES[i])
            {
                // If this isn't the first scope, add a space to separate scopes.
                if (scopeFound)
                {
                    strcat(scopeStr, " ");
                }
                // Add the current scope.
                strcat(scopeStr, GDRIVE_ACCESS_SCOPES[i].c_str());
                scopeFound = true;
            }
        }

        HttpQuery query(*this, "response_type", "code");
        query.add("client_id", GDRIVE_CLIENT_ID)
            .add("redirect_uri", GDRIVE_REDIRECT_URI)
            .add("scope", scopeStr)
            .add("include_granted_scopes", "true");

        string authUrl = query.assemble(GDRIVE_URL_AUTH_NEWAUTH);

        // Prompt the user.
        puts("This program needs access to a Google Drive account.\n"
                "To grant access, open the following URL in your web\n"
                "browser.  Copy the code that you receive, and paste it\n"
                "below.\n\n"
                "The URL to open is:");
        puts(authUrl.c_str());
        puts("\nPlease paste the authorization code here:");

        // The authorization code should never be this long, so it's fine to ignore
        // longer input
        char authCode[1024] = "";
        if (!fgets(authCode, 1024, stdin))
        {
            puts("Error getting user input");
            return -1;
        }

        if (authCode[0] == '\0')
        {
            // No code entered, return failure.
            return 1;
        }

        // Exchange the authorization code for access and refresh tokens.
        return refreshAuthToken(GDRIVE_GRANTTYPE_CODE, authCode);
    }

    int Gdrive::checkScopes()
    {
        // Prepare and send the network request
        HttpTransfer xfer(*this);
        int result =
            xfer.setRequestType(HttpTransfer::GET)
                .setRetryOnAuthError(false)
                .setUrl(GDRIVE_URL_AUTH_TOKENINFO)
                .addQuery(GDRIVE_FIELDNAME_ACCESSTOKEN, mAccessToken)
                .execute();
        
        if (result != 0 || xfer.getHttpResponse() >= 400)
        {
            // Download failed or gave a bad response.
            return -1;
        }

        // If we've made it this far, we have an ok response.  Extract the scopes
        // from the JSON array that should have been returned, and compare them
        // with the expected scopes.

        Json jsonObj(xfer.getData());
        if (!jsonObj.isValid())
        {
            // Couldn't interpret the response as JSON, return error.
            return -1;
        }
        string grantedScopes = jsonObj.getString("scope");
        if (grantedScopes.empty())
        {
            // Key not found, or value not a string.  Return error.
            return -1;
        }


        // Go through each of the space-separated scopes in the string, comparing
        // each one to the GDRIVE_ACCESS_SCOPES array.
        long startIndex = 0;
        long endIndex = 0;
        long scopeLength = grantedScopes.length();
        int matchedScopes = 0;
        while (startIndex <= scopeLength)
        {
            // After the loop executes, startIndex indicates the start of a scope,
            // and endIndex indicates the (null or space) terminator character.
            for (
                    endIndex = startIndex; 
                    !(endIndex >= scopeLength ||
                    grantedScopes[endIndex] == ' ' || 
                    grantedScopes[endIndex] == '\0');
                    endIndex++
                    )
            {
                // Empty body
            }

            // Compare the current scope to each of the entries in 
            // GDRIVE_ACCESS_SCOPES.  If there's a match, set the appropriate bit(s)
            // in matchedScopes.
            for (unsigned int i = 0; i < GDRIVE_ACCESS_MODE_COUNT; i++)
            {
                if (grantedScopes.compare(startIndex, endIndex - startIndex, 
                        GDRIVE_ACCESS_SCOPES[i]) == 0)
                {
                    matchedScopes = matchedScopes | GDRIVE_ACCESS_MODES[i];
                    continue;
                }
            }

            startIndex = endIndex + 1;
        }

        // Compare the access mode we encountered to the one we expected, one piece
        // at a time.  If we don't find what we need, return failure.
        for (unsigned int i = 0; i < GDRIVE_ACCESS_MODE_COUNT; i++)
        {
            if ((mMode & GDRIVE_ACCESS_MODES[i]) && 
                    !(matchedScopes & GDRIVE_ACCESS_MODES[i])
                    )
            {
                return -1;
            }
        }

        // If we made it through to here, return success.
        return 0;
    }

    string Gdrive::getChildFileId(const std::string& parentId, 
    const std::string& childName)
    {
        // Construct a filter in the form of 
        // "'<parentId>' in parents and title = '<childName>'"
        string filter = string("'") + parentId + "' in parents and title = '" +
                childName + "' and trashed = false";

        HttpTransfer xfer(*this);
        
        int result =
            xfer.setRequestType(HttpTransfer::GET)
                .setUrl(GDRIVE_URL_FILES)
                .addQuery("q", filter)
                .addQuery("fields", "items(id)")
                .execute();

        if (result != 0 || xfer.getHttpResponse() >= 400)
        {
            // Download error
            return "";
        }


        // If we're here, we have a good response.  Extract the ID from the 
        // response.

        // Convert to a JSON object.
        Json jsonObj(xfer.getData());
        if (!jsonObj.isValid())
        {
            // Couldn't convert to JSON object.
            return "";
        }

        string childId;
        Json arrayItem = jsonObj.arrayGet("items", 0);
        if (arrayItem.isValid())
        {
            childId.assign(arrayItem.getString("id"));
        }
        return childId;
    }

    int Gdrive::writeAuthCredentials()
    {
        if (mAuthFilename.empty())
        {
            // Do nothing if there's no filename
            return -1;
        }

        // Create a JSON object, fill it with the necessary details, 
        // convert to a string, and write to the file.
        FILE* outFile = Util::recursiveFopen(mAuthFilename, "w");
        if (outFile == NULL)
        {
            // Couldn't open file for writing.
            return -1;
        }

        Json jsonObj;
        jsonObj.addString(GDRIVE_FIELDNAME_ACCESSTOKEN, 
                mAccessToken);
        jsonObj.addString(GDRIVE_FIELDNAME_REFRESHTOKEN, 
                mRefreshToken);
        int success = fputs(jsonObj.toString(true).c_str(), 
                outFile);
        fclose(outFile);

        return (success >= 0) ? 0 : -1;
    }

    void Gdrive::SetupCurlHandle(CURL* curlHandle)
    {
        // Accept compressed responses and let libcurl automatically uncompress
        curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "");

        // Automatically follow redirects
        curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
    }
    
    
}
