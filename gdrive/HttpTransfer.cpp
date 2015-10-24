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
    
    const int HttpTransfer::RETRY_LIMIT = 5;

    HttpTransfer::HttpTransfer(Gdrive& gInfo)
    : mGInfo(gInfo)
    {
        mRequestType = (Request_Type) 0;
        mRetryOnAuthError = true;
        mpQuery = NULL;
        mpPostData = NULL;
        mpBody = NULL;
        mpHeaders = NULL;
        mDestFile = NULL;
        mUploadCallback = NULL;
        mUserdata = NULL;
        mUploadOffset = 0;
        mpResultBuf = NULL;
        
        getAuthbearerHeader();
    }
        
    HttpTransfer::~HttpTransfer()
    {
        if (mpResultBuf) delete mpResultBuf;
        if (mpQuery) delete mpQuery;
        if (mpPostData) delete mpPostData;
        if (mpHeaders) curl_slist_free_all(mpHeaders);
    }

    HttpTransfer& HttpTransfer::setRequestType(enum Request_Type requestType)
    {
        this->mRequestType = requestType;
        return *this;
    }

    HttpTransfer& HttpTransfer::setRetryOnAuthError(bool retry)
    {
        mRetryOnAuthError = retry;
        return *this;
    }

    HttpTransfer& HttpTransfer::setUrl(const std::string& url)
    {
        this->mUrl = url;
        return *this;
    }

    HttpTransfer& HttpTransfer::setDestfile(FILE* destFile)
    {
        this->mDestFile = destFile;
        return *this;
    }

    HttpTransfer& HttpTransfer::setBody(const std::string& body)
    {
        this->mpBody = &body;
        return *this;
    }

    HttpTransfer& HttpTransfer::setUploadCallback(uploadCallback callback, 
                                        void* userdata)
    {
        this->mUploadCallback = callback;
        this->mUploadOffset = 0;
        this->mUserdata = userdata;
        return *this;
    }

    HttpTransfer& HttpTransfer::addQuery(const std::string& field, 
        const std::string& value)
    {
        //return gdrive_xfer_add_query_or_post(&pQuery, field, value);
        
        mpQuery = mpQuery ? 
            &mpQuery->add(field, value) : 
            new HttpQuery(mGInfo, field, value);
        if (!mpQuery)
        {
            throw new exception();
        }
        
        return *this;
    }

    HttpTransfer& HttpTransfer::addPostField(const std::string& field, 
        const std::string& value)
    {
        //return gdrive_xfer_add_query_or_post(&pPostData, field, value);
        
        mpPostData = mpPostData ? 
            &mpPostData->add(field, value) : 
            new HttpQuery(mGInfo, field, value);
        if (!mpPostData)
        {
            throw new exception();
        }
        
        return *this;
    }

    HttpTransfer& HttpTransfer::addHeader(const std::string& header)
    {
        mpHeaders = curl_slist_append(mpHeaders, header.c_str());
        if (!mpHeaders)
        {
            throw new exception();
        }
        
        return *this;
    }

    int HttpTransfer::execute()
    {
        if (this->mUrl.empty())
        {
            // Invalid parameter, need at least a URL.
            return -1;
        }

        CURL* curlHandle = mGInfo.getCurlHandle();

        bool needsBody = false;

        // Set the request type
        switch (this->mRequestType)
        {
            case GET:
                curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, 1);
                break;

            case POST:
                curl_easy_setopt(curlHandle, CURLOPT_POST, 1);
                needsBody = true;
                break;

            case PUT:
                curl_easy_setopt(curlHandle, CURLOPT_UPLOAD, 1);
                needsBody = true;
                break;

            case PATCH:
                curl_easy_setopt(curlHandle, CURLOPT_POST, 1);
                curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "PATCH");
                needsBody = true;
                break;

            case DELETE:
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
        std::string fullUrl = this->mpQuery ?
            this->mpQuery->assemble(this->mUrl) :
            this->mUrl;
        if (fullUrl.empty())
        {
            // Memory error or invalid URL
            curl_easy_cleanup(curlHandle);
            return -1;
        }
        curl_easy_setopt(curlHandle, CURLOPT_URL, fullUrl.c_str());
        
        // Set simple POST fields, if applicable
        if (needsBody && this->mpBody == NULL && this->mpPostData == NULL && 
                this->mUploadCallback == NULL
                )
        {
            // A request type that normally has a body, but no body given. Need to
            // explicitly set the body length to 0, according to 
            // http://curl.haxx.se/libcurl/c/CURLOPT_POSTFIELDS.html
            curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, 0L);
        }
        if (this->mpBody != NULL)
        {
            curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, -1L);
            curl_easy_setopt(curlHandle, CURLOPT_COPYPOSTFIELDS, this->mpBody->c_str());
        }
        else if (this->mpPostData != NULL)
        {
            std::string postData = this->mpPostData->assembleAsPostData();
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
        if (this->mUploadCallback != NULL)
        {
            addHeader("Transfer-Encoding: chunked");
            curl_easy_setopt(curlHandle, CURLOPT_READFUNCTION, 
                    uploadCallbackInternal);
            curl_easy_setopt(curlHandle, CURLOPT_READDATA, this);
        }



        // Set headers
        curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, this->mpHeaders);

        try
        {
            mpResultBuf = new DownloadBuffer(mGInfo, this->mDestFile);
        }
        catch (const std::exception& e)
        {
            // Memory error.
            curl_easy_cleanup(curlHandle);
            return -1;
        }

        mpResultBuf->downloadWithRetry(curlHandle, this->mRetryOnAuthError, 
                RETRY_LIMIT);
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

    size_t HttpTransfer::uploadCallbackInternal(char* buffer, size_t size, 
            size_t nitems, void* instream)
    {
        // Get the transfer struct.
        HttpTransfer* pTransfer = (HttpTransfer*) instream;
        size_t bytesTransferred = pTransfer->mUploadCallback(pTransfer->mGInfo, 
                buffer, pTransfer->mUploadOffset, size * nitems, 
                pTransfer->mUserdata);
        if (bytesTransferred == (size_t)(-1))
        {
            // Upload error
            return CURL_READFUNC_ABORT;
        }
        // else succeeded

        pTransfer->mUploadOffset += bytesTransferred;
        return bytesTransferred;
    }

    void HttpTransfer::getAuthbearerHeader()
    {
        const string& token = mGInfo.getAccessToken();

        // If we don't have any access token yet, do nothing
        if (token.empty())
        {
            return;
        }

        // First form a string with the required text and the access token.
        string header("Authorization: Bearer ");
        header += token;

        // Copy the string into a curl_slist for use in headers.
        mpHeaders = curl_slist_append(mpHeaders, header.c_str());
    }

}
