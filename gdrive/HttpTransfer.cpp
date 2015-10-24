/* 
 * File:   HttpTransfer.cpp
 * Author: me
 * 
 * Created on October 24, 2015, 12:06 PM
 */

#include "HttpTransfer.hpp"
#include "DownloadBuffer.hpp"
#include "HttpQuery.hpp"
#include "Gdrive.hpp"

using namespace std;

namespace fusedrive
{
    
    const int HttpTransfer::GDRIVE_RETRY_LIMIT = 5;

    HttpTransfer::HttpTransfer(Gdrive& gInfo)
    : mGInfo(gInfo)
    {
        requestType = (Gdrive_Request_Type) 0;
        retryOnAuthError = true;
        pQuery = NULL;
        pPostData = NULL;
        body = NULL;
        pHeaders = gdrive_get_authbearer_header(NULL);
        destFile = NULL;
        uploadCallback = NULL;
        userdata = NULL;
        uploadOffset = 0;
        mpResultBuf = NULL;
    }
        
    HttpTransfer::~HttpTransfer()
    {
        if (mpResultBuf) delete mpResultBuf;
        if (pQuery) delete pQuery;
        if (pPostData) delete pPostData;
        if (pHeaders) curl_slist_free_all(pHeaders);
    }

    Gdrive& HttpTransfer::gdrive_xfer_get_gdrive()
    {
        return mGInfo;
    }

    HttpTransfer& HttpTransfer::gdrive_xfer_set_requesttype(enum Gdrive_Request_Type requestType)
    {
        this->requestType = requestType;
        return *this;
    }

    HttpTransfer& HttpTransfer::gdrive_xfer_set_retryonautherror(bool retry)
    {
        retryOnAuthError = retry;
        return *this;
    }

    HttpTransfer& HttpTransfer::gdrive_xfer_set_url(const std::string& url)
    {
        this->url = url;
        return *this;
    }

    HttpTransfer& HttpTransfer::gdrive_xfer_set_destfile(FILE* destFile)
    {
        this->destFile = destFile;
        return *this;
    }

    HttpTransfer& HttpTransfer::gdrive_xfer_set_body(const std::string& body)
    {
        this->body = &body;
        return *this;
    }

    HttpTransfer& HttpTransfer::gdrive_xfer_set_uploadcallback(gdrive_xfer_upload_callback callback, 
                                        void* userdata)
    {
        this->uploadCallback = callback;
        this->uploadOffset = 0;
        this->userdata = userdata;
        return *this;
    }

    HttpTransfer& HttpTransfer::gdrive_xfer_add_query(const std::string& field, 
        const std::string& value)
    {
        //return gdrive_xfer_add_query_or_post(&pQuery, field, value);
        
        pQuery = pQuery ? 
            &pQuery->add(field, value) : 
            new HttpQuery(mGInfo, field, value);
        if (!pQuery)
        {
            throw new exception();
        }
        
        return *this;
    }

    HttpTransfer& HttpTransfer::gdrive_xfer_add_postfield(const std::string& field, 
        const std::string& value)
    {
        //return gdrive_xfer_add_query_or_post(&pPostData, field, value);
        
        pPostData = pPostData ? 
            &pPostData->add(field, value) : 
            new HttpQuery(mGInfo, field, value);
        if (!pPostData)
        {
            throw new exception();
        }
        
        return *this;
    }

    HttpTransfer& HttpTransfer::gdrive_xfer_add_header(const std::string& header)
    {
        pHeaders = curl_slist_append(pHeaders, header.c_str());
        if (!pHeaders)
        {
            throw new exception();
        }
        
        return *this;
    }

    int HttpTransfer::gdrive_xfer_execute()
    {
        if (this->url.empty())
        {
            // Invalid parameter, need at least a URL.
            return -1;
        }

        CURL* curlHandle = mGInfo.getCurlHandle();

        bool needsBody = false;

        // Set the request type
        switch (this->requestType)
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
                return -1;
        }

        // Append any query parameters to the URL, and add the full URL to the
        // curl handle.
        std::string fullUrl = this->pQuery ?
            this->pQuery->assemble(this->url) :
            this->url;
        if (fullUrl.empty())
        {
            // Memory error or invalid URL
            curl_easy_cleanup(curlHandle);
            return -1;
        }
        curl_easy_setopt(curlHandle, CURLOPT_URL, fullUrl.c_str());
        
        // Set simple POST fields, if applicable
        if (needsBody && this->body == NULL && this->pPostData == NULL && 
                this->uploadCallback == NULL
                )
        {
            // A request type that normally has a body, but no body given. Need to
            // explicitly set the body length to 0, according to 
            // http://curl.haxx.se/libcurl/c/CURLOPT_POSTFIELDS.html
            curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, 0L);
        }
        if (this->body != NULL)
        {
            curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, -1L);
            curl_easy_setopt(curlHandle, CURLOPT_COPYPOSTFIELDS, this->body->c_str());
        }
        else if (this->pPostData != NULL)
        {
            std::string postData = this->pPostData->assembleAsPostData();
            if (postData.empty())
            {
                // Memory error or invalid query
                curl_easy_cleanup(curlHandle);
                return -1;
            }
            curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, -1L);
            curl_easy_setopt(curlHandle, CURLOPT_COPYPOSTFIELDS, postData.c_str());
        }

        // Set upload data callback, if applicable
        if (this->uploadCallback != NULL)
        {
            gdrive_xfer_add_header("Transfer-Encoding: chunked");
            curl_easy_setopt(curlHandle, CURLOPT_READFUNCTION, 
                    gdrive_xfer_upload_callback_internal);
            curl_easy_setopt(curlHandle, CURLOPT_READDATA, this);
        }



        // Set headers
        curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, this->pHeaders);

        try
        {
            mpResultBuf = new DownloadBuffer(mGInfo, this->destFile);
        }
        catch (const std::exception& e)
        {
            // Memory error.
            curl_easy_cleanup(curlHandle);
            return -1;
        }

        mpResultBuf->downloadWithRetry(curlHandle, this->retryOnAuthError, 
                GDRIVE_RETRY_LIMIT);
        curl_easy_cleanup(curlHandle);
        
        if (!mpResultBuf->wasSuccessful())
        {
            // Download failure
            delete mpResultBuf;
            mpResultBuf = NULL;
            return -1;
        }

        // The HTTP Response may be success (e.g., 200) or failure (400 or higher),
        // but the actual request succeeded as far as libcurl is concerned.  Return
        // success.
        return 0;
    }

    long HttpTransfer::getHttpResponse() const
    {
        // Throws if the transfer hasn't executed or has failed
        // (that is, if mpResultBuf is NULL)
        return mpResultBuf->getHttpResponse();
    }

    std::string HttpTransfer::getData() const
    {
        // Throws if the transfer hasn't executed or has failed
        // (that is, if mpResultBuf is NULL)
        return mpResultBuf->getData();
    }
    
    std::string HttpTransfer::getReturnedHeaders() const
    {
        // Throws if the transfer hasn't executed or has failed
        // (that is, if mpResultBuf is NULL)
        return mpResultBuf->getHeaders();
    }

    bool HttpTransfer::wasSuccessful() const
    {
        // Throws if the transfer hasn't executed or has failed
        // (that is, if mpResultBuf is NULL)
        return mpResultBuf->wasSuccessful();
    }
    
//    const DownloadBuffer& HttpTransfer::result()
//    {
//        // This throws if the transfer hasn't executed or has failed
//        // (if mpResultBuf is NULL).
//        return *mpResultBuf;
//    }

//    int HttpTransfer::gdrive_xfer_add_query_or_post(HttpQuery** ppQuery, 
//            const std::string& field, const std::string& value)
//    {
//        *ppQuery = *ppQuery ? 
//            &(*ppQuery)->add(field, value) :
//            new HttpQuery(mGInfo, field, value);
//        return (*ppQuery == NULL);
//    }

    size_t HttpTransfer::gdrive_xfer_upload_callback_internal(char* buffer, size_t size, 
            size_t nitems, void* instream)
    {
        // Get the transfer struct.
        HttpTransfer* pTransfer = (HttpTransfer*) instream;
        size_t bytesTransferred = pTransfer->uploadCallback(pTransfer->mGInfo, 
                buffer, pTransfer->uploadOffset, size * nitems, 
                pTransfer->userdata);
        if (bytesTransferred == (size_t)(-1))
        {
            // Upload error
            return CURL_READFUNC_ABORT;
        }
        // else succeeded

        pTransfer->uploadOffset += bytesTransferred;
        return bytesTransferred;
    }

    struct curl_slist* 
    HttpTransfer::gdrive_get_authbearer_header(struct curl_slist* pHeaders)
    {
        const string& token = mGInfo.getAccessToken();

        // If we don't have any access token yet, do nothing
        if (token.empty())
        {
            return pHeaders;
        }

        // First form a string with the required text and the access token.
        string header("Authorization: Bearer ");
        header += token;

        // Copy the string into a curl_slist for use in headers.
        struct curl_slist* returnVal = 
            curl_slist_append(pHeaders, header.c_str());
        return returnVal;
    }

}
