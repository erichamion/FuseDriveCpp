



#include "Gdrive.hpp"
#include "Cache.hpp"
#include "gdrive-query.hpp"
#include "Util.hpp"
#include "Json.hpp"


#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <string>
#include <exception>

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
    : maxChunks(maxChunksPerFile), authFilename(authFilename), 
            needsCurlCleanup(initCurl), curlHandle(NULL), 
            cache(*this, cacheTTL)
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
    
    size_t Gdrive::gdrive_get_minchunksize()
    {
        return minChunkSize;
    }
    
    int Gdrive::gdrive_get_maxchunks()
    {
        return maxChunks;
    }

    int Gdrive::gdrive_get_filesystem_perms(enum Gdrive_Filetype type)
    {
        // Get the permissions for regular files.
        int perms = 0;
        if (mode & Gdrive::GDRIVE_ACCESS_READ)
        {
            // Read access
            perms = perms | S_IROTH;
        }
        if (mode & Gdrive::GDRIVE_ACCESS_WRITE)
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


    Gdrive_Fileinfo_Array*  Gdrive::gdrive_folder_list(const string& folderId)
    {
        // Allow for an initial quote character in addition to the terminating null
        string filter = string("'") + folderId + 
                string("' in parents and trashed=false");

        // Prepare the network request
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(*this);
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);

        if (
                gdrive_xfer_set_url(pTransfer, GDRIVE_URL_FILES.c_str()) || 
                gdrive_xfer_add_query(*this, pTransfer, "q", filter.c_str()) || 
                gdrive_xfer_add_query(*this, pTransfer, "fields", 
                                      "items(title,id,mimeType)")
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
            throw new exception();
        }

        // Send the network request
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(*this, pTransfer);
        gdrive_xfer_free(pTransfer);


        // TODO: Somehow unify this process with other ways to fill Gdrive_Fileinfo,
        // reducing code duplication and taking advantage of the cache.
        int fileCount = -1;
        Gdrive_Fileinfo_Array* pArray = NULL;
        if (pBuf != NULL && gdrive_dlbuf_get_httpresp(pBuf) < 400)
        {
            // Transfer was successful.  Convert result to a JSON object and extract
            // the file meta-info.
            Json jsonObj(gdrive_dlbuf_get_data(pBuf));
            if (jsonObj.gdrive_json_is_valid())
            {
                bool dummy;
                fileCount = jsonObj.gdrive_json_array_length("items", dummy);
                if (fileCount > 0)
                {
                    // Create an array of Gdrive_Fileinfo structs large enough to
                    // hold all the items.
                    pArray = gdrive_finfoarray_create(*this, fileCount);
                    if (pArray != NULL)
                    {
                        // Extract the file info from each member of the array.
                        for (int index = 0; index < fileCount; index++)
                        {
                            Json jsonFile = 
                                    jsonObj.gdrive_json_array_get("items",
                                    index);
                            if (jsonFile.gdrive_json_is_valid())
                            {
                                gdrive_finfoarray_add_from_json(pArray, jsonFile);
                            }
                        }
                    }
                    else
                    {
                        // Memory error.
                        fileCount = -1;
                    }
                }
                // else either failure (return -1) or 0-length array (return 0),
                // nothing special needs to be done.

            }
            // else do nothing.  Already prepared to return error.
        }

        gdrive_dlbuf_free(pBuf);

        return pArray;
    }


    string Gdrive::gdrive_filepath_to_id(const string& path)
    {
        if (path.empty())
        {
            // Invalid path
            return string("");
        }

        // Try to get the ID from the cache.
        string cachedId(cache.gdrive_cache_get_fileid(path.c_str()));
        if (!cachedId.empty())
        {
            return cachedId;
        }
        // else ID isn't in the cache yet

        // Is this the root folder?
        string result = string();
        if (path.compare("/") == 0)
        {
            result = gdrive_get_root_folder_id();
            if (!result.empty())
            {
                // Add to the fileId cache.
                cache.gdrive_cache_add_fileid(path.c_str(), result.c_str());
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
        
        string parentId = gdrive_filepath_to_id(parentPath);
        if (parentId.empty())
        {
            // An error occurred.
            return string("");
        }
        // Use the parent's ID to find the child's ID.
        string childId = gdrive_get_child_id_by_name(parentId, childName);

        // Add the ID to the fileId cache.
        if (!childId.empty())
        {
            cache.gdrive_cache_add_fileid(path.c_str(), childId.c_str());
        }
        
        return childId;
    }


    int Gdrive::gdrive_remove_parent(const string& fileId, const string& parentId)
    {
        assert(!(fileId.empty() || parentId.empty()));

        // Need write access
        if (!(mode & Gdrive::GDRIVE_ACCESS_WRITE))
        {
            return -EACCES;
        }

        // URL will look like 
        // "<standard Drive Files url>/<fileId>/parents/<parentId>"
        string url = GDRIVE_URL_FILES + "/" + fileId + "/parents/" + parentId;

        Gdrive_Transfer* pTransfer = gdrive_xfer_create(*this);
        if (pTransfer == NULL)
        {
            return -ENOMEM;
        }
        if (gdrive_xfer_set_url(pTransfer, url.c_str()) != 0)
        {
            // Error, probably memory
            gdrive_xfer_free(pTransfer);
            return -ENOMEM;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_DELETE);

        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(*this, pTransfer);
        gdrive_xfer_free(pTransfer);

        int returnVal = (pBuf == NULL || gdrive_dlbuf_get_httpresp(pBuf) >= 400) ? 
            -EIO : 0;
        gdrive_dlbuf_free(pBuf);
        return returnVal;
    }


    int Gdrive::gdrive_delete(const string& fileId, const string& parentId)
    {
        // TODO: If support for manipulating trashed files is added, we'll need to
        // check whether the specified file is already trashed, and permanently 
        // delete if it is.
        // TODO: May want to add an option for whether to trash or really delete.

        assert(!fileId.empty());

        // Need write access
        if (!(mode & Gdrive::GDRIVE_ACCESS_WRITE))
        {
            return -EACCES;
        }

        // URL will look like 
        // "<standard Drive Files url>/<fileId>/trash"
        string url = GDRIVE_URL_FILES + "/" + fileId + "/trash";

        Gdrive_Transfer* pTransfer = gdrive_xfer_create(*this);
        if (pTransfer == NULL)
        {
            return -ENOMEM;
        }
        if (gdrive_xfer_set_url(pTransfer, url.c_str()) != 0)
        {
            // Error, probably memory
            gdrive_xfer_free(pTransfer);
            return -ENOMEM;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_POST);

        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(*this, pTransfer);
        gdrive_xfer_free(pTransfer);

        int returnVal = (pBuf == NULL || gdrive_dlbuf_get_httpresp(pBuf) >= 400) ? 
            -EIO : 0;
        gdrive_dlbuf_free(pBuf);
        if (returnVal == 0)
        {
            cache.gdrive_cache_delete_id(fileId.c_str());
            if (!parentId.empty() && parentId.compare("/") != 0)
            {
                // Remove the parent from the cache because the child count will be
                // wrong.
                cache.gdrive_cache_delete_id(parentId.c_str());
            }
        }
        return returnVal;
    }


    int Gdrive::gdrive_add_parent(const string& fileId, const string& parentId)
    {
        assert(!(fileId.empty() || parentId.empty()));

        // Need write access
        if (!(mode & Gdrive::GDRIVE_ACCESS_WRITE))
        {
            return -EACCES;
        }

        // URL will look like 
        // "<standard Drive Files url>/<fileId>/parents"
        string url = GDRIVE_URL_FILES + "/" + fileId + "/parents";

        // Create the Parent resource for the request body
        Json jsonObj;
        jsonObj.gdrive_json_add_string("id", parentId);
        string body = string(jsonObj.gdrive_json_to_string(false));
        

        Gdrive_Transfer* pTransfer = gdrive_xfer_create(*this);
        if (pTransfer == NULL)
        {
            return -ENOMEM;
        }
        if (gdrive_xfer_set_url(pTransfer, url.c_str()) || 
                gdrive_xfer_add_header(pTransfer, "Content-Type: application/json")
                )
        {
            // Error, probably memory
            gdrive_xfer_free(pTransfer);
            return -ENOMEM;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_POST);
        gdrive_xfer_set_body(pTransfer, body.c_str());

        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(*this, pTransfer);
        gdrive_xfer_free(pTransfer);

        int returnVal = (pBuf == NULL || gdrive_dlbuf_get_httpresp(pBuf) >= 400) ? 
            -EIO : 0;
        gdrive_dlbuf_free(pBuf);

        if (returnVal == 0)
        {
            // Update the parent count in case we do anything else with the file
            // before the cache expires. (For example, if there was only one parent
            // before, and the user deletes one of the links, we don't want to
            // delete the entire file because of a bad parent count).
            Fileinfo* pFileinfo = 
                    cache.gdrive_cache_get_item(fileId.c_str(), false, NULL);
            if (pFileinfo)
            {
                pFileinfo->nParents++;
            }
        }
        return returnVal;
    }

    int Gdrive::gdrive_change_basename(const string& fileId, const string& newName)
    {
        assert(!(fileId.empty() || newName.empty()));

        // Need write access
        if (!(mode & Gdrive::GDRIVE_ACCESS_WRITE))
        {
            return -EACCES;
        }

        // Create the request body with the new name
        Json jsonObj;
        jsonObj.gdrive_json_add_string("title", newName);
        string body(jsonObj.gdrive_json_to_string(false));

        // Create the url in the form of:
        // "<GDRIVE_URL_FILES>/<fileId>"
        string url = GDRIVE_URL_FILES + "/" + fileId;

        // Set up the network transfer
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(*this);
        if (!pTransfer)
        {
            // Memory error
            return -ENOMEM;
        }
        if (gdrive_xfer_set_url(pTransfer, url.c_str()) || 
                gdrive_xfer_add_query(*this, pTransfer, "updateViewedDate", "false") || 
                gdrive_xfer_add_header(pTransfer, "Content-Type: application/json")
                )
        {
            // Error, probably memory
            gdrive_xfer_free(pTransfer);
            return -ENOMEM;
        }
        gdrive_xfer_set_body(pTransfer, body.c_str());
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_PATCH);

        // Send the network request
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(*this, pTransfer);
        gdrive_xfer_free(pTransfer);

        int returnVal = (pBuf == NULL || gdrive_dlbuf_get_httpresp(pBuf) >= 400) ? 
            -EIO : 0;
        gdrive_dlbuf_free(pBuf);
        return returnVal;
    }

    Gdrive::~Gdrive() 
    {
        if (this->needsCurlCleanup && curlHandle != NULL)
        {
            curl_easy_cleanup(curlHandle);
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
    
    CURL* Gdrive::gdrive_get_curlhandle()
    {
        if (curlHandle == NULL)
        {
            curlHandle = curl_easy_init();
            if (curlHandle == NULL)
            {
                // Error
                return NULL;
            }
            gdrive_curlhandle_setup(curlHandle);
        }
        return curl_easy_duphandle(curlHandle);
    }

    Cache& Gdrive::gdrive_get_cache()
    {
        return cache;
    }
    
    const string& Gdrive::gdrive_get_access_token()
    {
        return accessToken;
    }

    int Gdrive::gdrive_auth()
    {
        // Try to refresh existing tokens first.
        if (!refreshToken.empty())
        {
            int refreshSuccess = gdrive_refresh_auth_token(
                    GDRIVE_GRANTTYPE_REFRESH, refreshToken
            );

            if (refreshSuccess == 0)
            {
                // Refresh succeeded, but we don't know what scopes were previously
                // granted.  Check to make sure we have the required scopes.  If so,
                // then we don't need to do anything else and can return success.
                int success = gdrive_check_scopes();
                if (success == 0)
                {
                    // Refresh succeeded with correct scopes, return success.
                    return 0;
                }
            }
        }

        // Either didn't have a refresh token, or it didn't work.  Need to get new
        // authorization, if allowed.
        if (!userInteractionAllowed)
        {
            // Need to get new authorization, but not allowed to interact with the
            // user.  Return error.
            return -1;
        }

        // If we've gotten this far, then we need to interact with the user, and
        // we're allowed to do so.  Prompt for authorization, and return whatever
        // success or failure the prompt returns.
        return gdrive_prompt_for_auth();
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
        clientId = GDRIVE_CLIENT_ID;
        clientSecret = GDRIVE_CLIENT_SECRET;
        redirectUri = GDRIVE_REDIRECT_URI;

        // Can we prompt the user for authentication during initial setup?
        if (interactionMode == GDRIVE_INTERACTION_STARTUP || 
                interactionMode == GDRIVE_INTERACTION_ALWAYS)
        {
            userInteractionAllowed = true;
        }

        // If a filename was given, attempt to open the file and read its contents.
        if (!authFilename.empty())
        {
            gdrive_read_auth_file(authFilename);
        }

        // Authenticate or refresh access
        mode = access;
        if (mode & Gdrive::GDRIVE_ACCESS_WRITE) 
        {
            // Write access implies read access
            mode = mode | Gdrive::GDRIVE_ACCESS_READ;
        }
        if (mode & Gdrive::GDRIVE_ACCESS_READ) 
        {
            // Read access implies meta-info access
            mode = mode | Gdrive::GDRIVE_ACCESS_META;
        }
        if (gdrive_auth() != 0)
        {
            // Could not get the required permissions.
            throw new exception();
        }
        gdrive_save_auth();
        // Can we continue prompting for authentication if needed later?
        userInteractionAllowed = 
                (interactionMode == GDRIVE_INTERACTION_ALWAYS);
        
        cache.gdrive_cache_init();
        
        // Set chunk size
        minChunkSize = (minFileChunkSize > 0) ? 
            Util::gdrive_divide_round_up(minFileChunkSize, Gdrive::GDRIVE_BASE_CHUNK_SIZE) * 
                Gdrive::GDRIVE_BASE_CHUNK_SIZE :
            Gdrive::GDRIVE_BASE_CHUNK_SIZE;

    }
    
    int Gdrive::gdrive_read_auth_file(const std::string& filename)
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
            if (!jsonObj.gdrive_json_is_valid())
            {
                // Couldn't convert the file contents to a JSON object, prepare to
                // return failure.
                returnVal = -1;
            }
            else
            {
                accessToken.assign(jsonObj.gdrive_json_get_string( 
                        GDRIVE_FIELDNAME_ACCESSTOKEN));
                refreshToken.assign(jsonObj.gdrive_json_get_string( 
                        GDRIVE_FIELDNAME_REFRESHTOKEN));
                
                if (accessToken.empty() || refreshToken.empty())
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
        
    int Gdrive::gdrive_refresh_auth_token(const std::string& grantType, 
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
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(*this);
        if (pTransfer == NULL)
        {
            // Memory error
            return -1;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_POST);

        // We're trying to get authorization, so it doesn't make sense to retry if
        // authentication fails.
        gdrive_xfer_set_retryonautherror(pTransfer, false);

        // Set up the post data. Some of the post fields depend on whether we have
        // an auth code or a refresh token, and some do not.
        string tokenOrCodeField;
        if (grantType == GDRIVE_GRANTTYPE_CODE)
        {
            // Converting an auth code into auth and refresh tokens.  Interpret
            // tokenString as the auth code.
            if (gdrive_xfer_add_postfield(*this, pTransfer,
                                      GDRIVE_FIELDNAME_REDIRECTURI.c_str(), 
                                      GDRIVE_REDIRECT_URI.c_str()
                    ) != 0)
            {
                // Error
                gdrive_xfer_free(pTransfer);
                return -1;
            }
            tokenOrCodeField.assign(GDRIVE_FIELDNAME_CODE);
        }
        else
        {
            // Refreshing an existing refresh token.  Interpret tokenString as the
            // refresh token.
            tokenOrCodeField.assign(GDRIVE_FIELDNAME_REFRESHTOKEN);
        }
        if (
                gdrive_xfer_add_postfield(*this, pTransfer, tokenOrCodeField.c_str(), 
                                          tokenString.c_str()) || 
                gdrive_xfer_add_postfield(*this, pTransfer, GDRIVE_FIELDNAME_CLIENTID.c_str(), 
                                          GDRIVE_CLIENT_ID) || 
                gdrive_xfer_add_postfield(*this, pTransfer, GDRIVE_FIELDNAME_CLIENTSECRET.c_str(),
                                          GDRIVE_CLIENT_SECRET) || 
                gdrive_xfer_add_postfield(*this, pTransfer, GDRIVE_FIELDNAME_GRANTTYPE.c_str(), 
                                          grantType.c_str()) || 
                gdrive_xfer_set_url(pTransfer, GDRIVE_URL_AUTH_TOKEN.c_str())
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
            return -1;
        }

        // Do the transfer. 
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(*this, pTransfer);
        gdrive_xfer_free(pTransfer);

        if (pBuf == NULL)
        {
            // There was an error sending the request and getting the response.
            return -1;
        }
        if (gdrive_dlbuf_get_httpresp(pBuf) >= 400)
        {
            // Failure, but probably not an error.  Most likely, the user has
            // revoked permission or the refresh token has otherwise been
            // invalidated.
            gdrive_dlbuf_free(pBuf);
            return 1;
        }

        // If we've gotten this far, we have a good HTTP response.  Now we just
        // need to pull the access_token string (and refresh token string if
        // present) out of it.

        Json jsonObj(gdrive_dlbuf_get_data(pBuf));
        gdrive_dlbuf_free(pBuf);
        if (!jsonObj.gdrive_json_is_valid())
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
        string tmpStr = 
                jsonObj.gdrive_json_get_string(GDRIVE_FIELDNAME_ACCESSTOKEN);
        if (tmpStr.empty())
        {
            // Couldn't get access token
            return -1;
        }
        accessToken.assign(tmpStr);
        
        // Only try to get refresh token if we successfully got the access 
        // token.
        
        // We won't always have a refresh token.  Specifically, if we were
        // already sending a refresh token, we may not get one back.
        // Don't treat the lack of a refresh token as an error or a failure,
        // and don't clobber the existing refresh token if we don't get a
        // new one.

        tmpStr = jsonObj.gdrive_json_get_string(GDRIVE_FIELDNAME_REFRESHTOKEN);
        if (!tmpStr.empty())
        {
            // We were given a refresh token, so store it.
            refreshToken.assign(tmpStr);
        }

        return 0;
    }

    int Gdrive::gdrive_prompt_for_auth()
    {
        char scopeStr[GDRIVE_SCOPE_MAXLENGTH] = "";
        bool scopeFound = false;

        // Check each of the possible permissions, and add the appropriate scope
        // if necessary.
        for (unsigned int i = 0; i < GDRIVE_ACCESS_MODE_COUNT; i++)
        {
            if (mode & GDRIVE_ACCESS_MODES[i])
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

        Gdrive_Query* pQuery = NULL;
        pQuery = gdrive_query_add(*this, pQuery, "response_type", "code");
        pQuery = gdrive_query_add(*this, pQuery, "client_id", GDRIVE_CLIENT_ID);
        pQuery = gdrive_query_add(*this, pQuery, "redirect_uri", GDRIVE_REDIRECT_URI.c_str());
        pQuery = gdrive_query_add(*this, pQuery, "scope", scopeStr);
        pQuery = gdrive_query_add(*this, pQuery, "include_granted_scopes", "true");
        if (pQuery == NULL)
        {
            // Memory error
            return -1;
        }

        char* authUrl = gdrive_query_assemble(pQuery, GDRIVE_URL_AUTH_NEWAUTH.c_str());
        gdrive_query_free(pQuery);

        if (authUrl == NULL)
        {
            // Return error
            return -1;
        }

        // Prompt the user.
        puts("This program needs access to a Google Drive account.\n"
                "To grant access, open the following URL in your web\n"
                "browser.  Copy the code that you receive, and paste it\n"
                "below.\n\n"
                "The URL to open is:");
        puts(authUrl);
        puts("\nPlease paste the authorization code here:");
        free(authUrl);

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
        return gdrive_refresh_auth_token(GDRIVE_GRANTTYPE_CODE, authCode);
    }

    int Gdrive::gdrive_check_scopes()
    {
        // Prepare the network request
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(*this);
        if (pTransfer == NULL)
        {
            // Memory error
            return -1;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);
        gdrive_xfer_set_retryonautherror(pTransfer, false);
        if (
                gdrive_xfer_set_url(pTransfer, GDRIVE_URL_AUTH_TOKENINFO.c_str()) || 
                gdrive_xfer_add_query(*this, pTransfer, GDRIVE_FIELDNAME_ACCESSTOKEN.c_str(), 
                                      accessToken.c_str())
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
            return -1;
        }

        // Send the network request
        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(*this, pTransfer);
        gdrive_xfer_free(pTransfer);

        if (pBuf == NULL || gdrive_dlbuf_get_httpresp(pBuf) >= 400)
        {
            // Download failed or gave a bad response.
            gdrive_dlbuf_free(pBuf);
            return -1;
        }

        // If we've made it this far, we have an ok response.  Extract the scopes
        // from the JSON array that should have been returned, and compare them
        // with the expected scopes.

        Json jsonObj(gdrive_dlbuf_get_data(pBuf));
        gdrive_dlbuf_free(pBuf);
        if (!jsonObj.gdrive_json_is_valid())
        {
            // Couldn't interpret the response as JSON, return error.
            return -1;
        }
        string grantedScopes = jsonObj.gdrive_json_get_string("scope");
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
            if ((mode & GDRIVE_ACCESS_MODES[i]) && 
                    !(matchedScopes & GDRIVE_ACCESS_MODES[i])
                    )
            {
                return -1;
            }
        }

        // If we made it through to here, return success.
        return 0;
    }

    string Gdrive::gdrive_get_root_folder_id()
    {
        return string(gdrive_sysinfo_get_rootid(*this));
    }

    string Gdrive::gdrive_get_child_id_by_name(const std::string& parentId, 
    const std::string& childName)
    {
        // Construct a filter in the form of 
        // "'<parentId>' in parents and title = '<childName>'"
        string filter = string("'") + parentId + "' in parents and title = '" +
                childName + "' and trashed = false";

        Gdrive_Transfer* pTransfer = gdrive_xfer_create(*this);
        if (pTransfer == NULL)
        {
            // Memory error
            return string("");
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);
        if (
                gdrive_xfer_set_url(pTransfer, GDRIVE_URL_FILES.c_str()) || 
                gdrive_xfer_add_query(*this, pTransfer, "q", filter.c_str()) || 
                gdrive_xfer_add_query(*this, pTransfer, "fields", "items(id)")
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
            return string("");
        }

        Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(*this, pTransfer);
        gdrive_xfer_free(pTransfer);

        if (pBuf == NULL || gdrive_dlbuf_get_httpresp(pBuf) >= 400)
        {
            // Download error
            gdrive_dlbuf_free(pBuf);
            return string("");
        }


        // If we're here, we have a good response.  Extract the ID from the 
        // response.

        // Convert to a JSON object.
        Json jsonObj(gdrive_dlbuf_get_data(pBuf));
        gdrive_dlbuf_free(pBuf);
        if (!jsonObj.gdrive_json_is_valid())
        {
            // Couldn't convert to JSON object.
            return string("");
        }

        string childId;
        Json arrayItem = jsonObj.gdrive_json_array_get("items", 0);
        if (arrayItem.gdrive_json_is_valid())
        {
            childId.assign(arrayItem.gdrive_json_get_string("id"));
        }
        return childId;
    }

    int Gdrive::gdrive_save_auth()
    {
        if (authFilename.empty())
        {
            // Do nothing if there's no filename
            return -1;
        }

        // Create a JSON object, fill it with the necessary details, 
        // convert to a string, and write to the file.
        FILE* outFile = Util::gdrive_power_fopen(authFilename, "w");
        if (outFile == NULL)
        {
            // Couldn't open file for writing.
            return -1;
        }

        Json jsonObj;
        jsonObj.gdrive_json_add_string(GDRIVE_FIELDNAME_ACCESSTOKEN, 
                accessToken);
        jsonObj.gdrive_json_add_string(GDRIVE_FIELDNAME_REFRESHTOKEN, 
                refreshToken);
        int success = fputs(jsonObj.gdrive_json_to_string(true).c_str(), 
                outFile);
        fclose(outFile);

        return (success >= 0) ? 0 : -1;
    }

    void Gdrive::gdrive_curlhandle_setup(CURL* curlHandle)
    {
        // Accept compressed responses and let libcurl automatically uncompress
        curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "");

        // Automatically follow redirects
        curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
    }
    
    
}
