



#include "gdrive-transfer.hpp"
#include "HttpQuery.hpp"

#include <string.h>

using namespace fusedrive;


#define GDRIVE_RETRY_LIMIT 5


/*************************************************************************
 * Private struct and declarations of private functions for use within 
 * this file
 *************************************************************************/


typedef struct Gdrive_Transfer 
{
    enum Gdrive_Request_Type requestType;
    bool retryOnAuthError;
    char* url;
    HttpQuery* pQuery;
    HttpQuery* pPostData;
    const char* body;
    struct curl_slist* pHeaders;
    FILE* destFile;
    gdrive_xfer_upload_callback uploadCallback;
    void* userdata;
    off_t uploadOffset;
    Gdrive* pGdrive;
} Gdrive_Transfer;


/*
 * Returns 0 on success, other on failure.
 */
static int gdrive_xfer_add_query_or_post(Gdrive& gInfo, HttpQuery** ppQuery, 
                                         const char* field, const char* value);

static size_t gdrive_xfer_upload_callback_internal(char* buffer, size_t size, 
                                                   size_t nitems, 
                                                   void* instream);

static struct curl_slist* 
gdrive_get_authbearer_header(Gdrive& gInfo, struct curl_slist* pHeaders);


/*************************************************************************
 * Implementations of public functions for internal or external use
 *************************************************************************/

/******************
 * Constructors, factory methods, destructors and similar
 ******************/

Gdrive_Transfer* gdrive_xfer_create(Gdrive& gInfo)
{
    Gdrive_Transfer* returnVal = (Gdrive_Transfer*) malloc(sizeof(Gdrive_Transfer));
    if (returnVal != NULL)
    {
        memset(returnVal, 0, sizeof(Gdrive_Transfer));
        returnVal->retryOnAuthError = true;
        returnVal->pHeaders = gdrive_get_authbearer_header(gInfo, NULL);
    }
    
    return returnVal;
}

void gdrive_xfer_free(Gdrive_Transfer* pTransfer)
{
    if (pTransfer == NULL)
    {
        // Nothing to do
        return;
    }
    
    // If these need freed, they'll be non-NULL. If they don't need freed,
    // they'll be NULL, and it's safe to free them anyway.
    free(pTransfer->url);
    pTransfer->url = NULL;
    delete (pTransfer->pQuery);
    pTransfer->pQuery = NULL;
    delete (pTransfer->pPostData);
    pTransfer->pPostData = NULL;
    if (pTransfer->pHeaders != NULL)
    {
        curl_slist_free_all(pTransfer->pHeaders);
        pTransfer->pHeaders = NULL;
    }
    
    // Free the overall struct
    free(pTransfer);
}


/******************
 * Getter and setter functions
 ******************/

void gdrive_xfer_set_gdrive(Gdrive_Transfer* pTransfer, Gdrive& gdrive)
{
    pTransfer->pGdrive = &gdrive;
}

Gdrive& gdrive_xfer_get_gdrive(Gdrive_Transfer* pTransfer)
{
    return *(pTransfer->pGdrive);
}

void gdrive_xfer_set_requesttype(Gdrive_Transfer* pTransfer, 
                                 enum Gdrive_Request_Type requestType)
{
    pTransfer->requestType = requestType;
}

void gdrive_xfer_set_retryonautherror(Gdrive_Transfer* pTransfer, bool retry)
{
    pTransfer->retryOnAuthError = retry;
}

int gdrive_xfer_set_url(Gdrive_Transfer* pTransfer, const char* url)
{
    size_t size = strlen(url) + 1;
    pTransfer->url = (char*) malloc(size);
    if (pTransfer->url == NULL)
    {
        // Memory error
        return -1;
    }
    memcpy(pTransfer->url, url, size);
    return 0;
}

void gdrive_xfer_set_destfile(Gdrive_Transfer* pTransfer, FILE* destFile)
{
    pTransfer->destFile = destFile;
}

void gdrive_xfer_set_body(Gdrive_Transfer* pTransfer, const char* body)
{
    pTransfer->body = body;
}

void gdrive_xfer_set_uploadcallback(Gdrive_Transfer* pTransfer, 
                                    gdrive_xfer_upload_callback callback, 
                                    void* userdata)
{
    pTransfer->userdata = userdata;
    pTransfer->uploadOffset = 0;
    pTransfer->uploadCallback = callback;
}


/******************
 * Other accessible functions
 ******************/

int gdrive_xfer_add_query(Gdrive& gInfo, Gdrive_Transfer* pTransfer, 
                          const char* field, 
                          const char* value)
{
    return gdrive_xfer_add_query_or_post(gInfo, &(pTransfer->pQuery), field, value);
}

int gdrive_xfer_add_postfield(Gdrive& gInfo, Gdrive_Transfer* pTransfer, const char* field, 
                              const char* value)
{
    return gdrive_xfer_add_query_or_post(gInfo, &(pTransfer->pPostData), field, value);
}

int gdrive_xfer_add_header(Gdrive_Transfer* pTransfer, const char* header)
{
    pTransfer->pHeaders = curl_slist_append(pTransfer->pHeaders, header);
    return (pTransfer->pHeaders == NULL);
}

DownloadBuffer* gdrive_xfer_execute(Gdrive& gInfo, Gdrive_Transfer* pTransfer)
{
    if (pTransfer->url == NULL)
    {
        // Invalid parameter, need at least a URL.
        return NULL;
    }
    
    CURL* curlHandle = gInfo.getCurlHandle();
    
    bool needsBody = false;
    
    // Set the request type
    switch (pTransfer->requestType)
    {
        case GDRIVE_REQUEST_GET:
            curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, 1);
            break;

        case GDRIVE_REQUEST_POST:
            curl_easy_setopt(curlHandle, CURLOPT_POST, 1);
            needsBody = true;
            break;

        case GDRIVE_REQUEST_PUT:
            curl_easy_setopt(curlHandle, CURLOPT_UPLOAD, 1);
            needsBody = true;
            break;

        case GDRIVE_REQUEST_PATCH:
            curl_easy_setopt(curlHandle, CURLOPT_POST, 1);
            curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "PATCH");
            needsBody = true;
            break;

        case GDRIVE_REQUEST_DELETE:
            curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, 1);
            curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;

        default:
            // Unsupported request type.  
            curl_easy_cleanup(curlHandle);
            return NULL;
    }
    
    // Append any query parameters to the URL, and add the full URL to the
    // curl handle.
    const char* fullUrl = NULL;
    std::string tmpStr = pTransfer->pQuery ?
        pTransfer->pQuery->assemble(pTransfer->url) :
        pTransfer->url;
    fullUrl = tmpStr.c_str();
    if (fullUrl == NULL)
    {
        // Memory error or invalid URL
        curl_easy_cleanup(curlHandle);
        return NULL;
    }
    curl_easy_setopt(curlHandle, CURLOPT_URL, fullUrl);
    //free(fullUrl);
    fullUrl = NULL;
    
    // Set simple POST fields, if applicable
    if (needsBody && pTransfer->body == NULL && pTransfer->pPostData == NULL && 
            pTransfer->uploadCallback == NULL
            )
    {
        // A request type that normally has a body, but no body given. Need to
        // explicitly set the body length to 0, according to 
        // http://curl.haxx.se/libcurl/c/CURLOPT_POSTFIELDS.html
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, 0L);
    }
    if (pTransfer->body != NULL)
    {
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, -1L);
        curl_easy_setopt(curlHandle, CURLOPT_COPYPOSTFIELDS, pTransfer->body);
    }
    else if (pTransfer->pPostData != NULL)
    {
        std::string tmpStr = 
                pTransfer->pPostData->assembleAsPostData();
        const char* postData = tmpStr.c_str();
        if (postData == NULL)
        {
            // Memory error or invalid query
            curl_easy_cleanup(curlHandle);
            return NULL;
        }
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, -1L);
        curl_easy_setopt(curlHandle, CURLOPT_COPYPOSTFIELDS, postData);
    }
    
    // Set upload data callback, if applicable
    if (pTransfer->uploadCallback != NULL)
    {
        gdrive_xfer_set_gdrive(pTransfer, gInfo);
        gdrive_xfer_add_header(pTransfer, "Transfer-Encoding: chunked");
        curl_easy_setopt(curlHandle, 
                         CURLOPT_READFUNCTION, 
                         gdrive_xfer_upload_callback_internal
                );
        curl_easy_setopt(curlHandle, CURLOPT_READDATA, pTransfer);
    }
    

    
    // Set headers
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, pTransfer->pHeaders);
    
    
    DownloadBuffer* pBuf;
    try
    {
        pBuf = new DownloadBuffer(gInfo, pTransfer->destFile);
    }
    catch (const std::exception& e)
    {
        // Memory error.
        curl_easy_cleanup(curlHandle);
        return NULL;
    }
    
    pBuf->downloadWithRetry(curlHandle, 
            pTransfer->retryOnAuthError, GDRIVE_RETRY_LIMIT);
    curl_easy_cleanup(curlHandle);
    
    if (!pBuf->wasSuccessful())
    {
        // Download failure
        delete pBuf;
        return NULL;
    }
    
    // The HTTP Response may be success (e.g., 200) or failure (400 or higher),
    // but the actual request succeeded as far as libcurl is concerned.  Return
    // the buffer.
    return pBuf;
}


/*************************************************************************
 * Implementations of private functions for use within this file
 *************************************************************************/

static int gdrive_xfer_add_query_or_post(Gdrive& gInfo, HttpQuery** ppQuery, 
                                         const char* field, const char* value)
{
    *ppQuery = *ppQuery ? 
        &(*ppQuery)->add(field, value) :
        new HttpQuery(gInfo, field, value);
    return (*ppQuery == NULL);
}

static size_t gdrive_xfer_upload_callback_internal(char* buffer, size_t size, 
                                                   size_t nitems, 
                                                   void* instream)
{
    // Get the transfer struct.
    Gdrive_Transfer* pTransfer = (Gdrive_Transfer*) instream;
    size_t bytesTransferred = 
            pTransfer->uploadCallback(gdrive_xfer_get_gdrive(pTransfer), buffer, pTransfer->uploadOffset, 
                                      size * nitems, pTransfer->userdata
            );
    if (bytesTransferred == (size_t)(-1))
    {
        // Upload error
        return CURL_READFUNC_ABORT;
    }
    // else succeeded
    
    pTransfer->uploadOffset += bytesTransferred;
    return bytesTransferred;
}

/*
 * pHeaders can be NULL, or an existing set of headers can be given.
 */
static struct curl_slist* 
gdrive_get_authbearer_header(Gdrive& gInfo, struct curl_slist* pHeaders)
{
    const char* token = gInfo.getAccessToken().c_str();
    
    // If we don't have any access token yet, do nothing
    if (!token)
    {
        return pHeaders;
    }
    
    // First form a string with the required text and the access token.
    char* header = (char*) malloc(strlen("Authorization: Bearer ") + 
                          strlen(token) + 1
    );
    if (!header)
    {
        // Memory error
        return NULL;
    }
    strcpy(header, "Authorization: Bearer ");
    strcat(header, token);
    
    // Copy the string into a curl_slist for use in headers.
    struct curl_slist* returnVal = curl_slist_append(pHeaders, header);
    free(header);
    return returnVal;
}
