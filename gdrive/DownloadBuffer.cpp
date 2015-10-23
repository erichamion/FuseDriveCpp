/* 
 * File:   DownloadBuffer.cpp
 * Author: me
 * 
 * Created on October 22, 2015, 8:16 PM
 */

#include "DownloadBuffer.hpp"
#include "Gdrive.hpp"

#include <string.h>
#include <sstream>

using namespace std;
namespace fusedrive
{
    std::string GDRIVE_403_RATELIMIT = "rateLimitExceeded";
    std::string GDRIVE_403_USERRATELIMIT = "userRateLimitExceeded";

    DownloadBuffer::DownloadBuffer(Gdrive& gInfo, FILE* fh)
    : mFh(fh), mGInfo(gInfo)
    {
        //usedSize = 0;
        //allocatedSize = initialSize;
        mHttpResp = 0;
        mResultCode = CURLE_OK;
    }

    DownloadBuffer::~DownloadBuffer()
    {
        // Empty
    }

    long DownloadBuffer::getHttpResponse()
    {
        return mHttpResp;
    }

    string DownloadBuffer::getData()
    {
        return mData.str();
    }

    bool DownloadBuffer::wasSuccessful()
    {
        return (mResultCode == CURLE_OK);
    }

    CURLcode DownloadBuffer::download(CURL* curlHandle)
    {
        // Set the destination - either our own callback function to fill the
        // in-memory buffer, or the default libcurl function to write to a FILE*.
        if (mFh == NULL)
        {
            curl_easy_setopt(curlHandle, 
                             CURLOPT_WRITEFUNCTION, 
                             dataCallback
                    );
            curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, this);
        }
        else
        {
            curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, mFh);
        }

        // Capture the returned headers with a callback
        curl_easy_setopt(curlHandle, 
                         CURLOPT_HEADERFUNCTION, 
                         headerCallback
                );
        curl_easy_setopt(curlHandle, CURLOPT_HEADERDATA, this);

        // Do the transfer.
        mResultCode = curl_easy_perform(curlHandle);

        // Get the HTTP response
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &mHttpResp);

        return mResultCode;
    }

    int DownloadBuffer::downloadWithRetry(CURL* curlHandle, 
            bool retryOnAuthError, int maxTries)
    {
        return downloadWithRetry(curlHandle, retryOnAuthError, 
                0, maxTries);
    }
    
    int DownloadBuffer::downloadWithRetry(CURL* curlHandle, bool retryOnAuthError, 
                                             int tryNum, int maxTries)
    {
        CURLcode curlResult = download(curlHandle);


        if (curlResult != CURLE_OK)
        {
            // Download error
            return -1;
        }
        if (mHttpResp >= 400)
        {
            // Handle HTTP error responses.  Normal error handling - 5xx gets 
            // retried, 403 gets retried if it's due to rate limits, 401 gets
            // retried after refreshing auth.  If retryOnAuthError is false, 
            // suppress the normal behavior for 401 and don't retry.

            // See whether we've already used our maximum attempts.
            if (tryNum == maxTries)
            {
                return -1;
            }

            bool retry = false;
            switch (shouldRetry(mHttpResp))
            {
                case RetryMethod::RETRY:
                    // Normal retry, use exponential backoff.
                    doExponentialWait(tryNum);
                    retry = true;
                    break;

                case RetryMethod::RENEWAUTH:
                    // Authentication error, probably expired access token.
                    // If retryOnAuthError is true, refresh auth and retry (unless 
                    // auth fails).
                    if (retryOnAuthError)
                    {
                        retry = (mGInfo.authenticate() == 0);
                        break;
                    }
                    // else fall through to NORETRY (and again to default)

                case RetryMethod::NORETRY:
                    // Fall through to default
                    default:
                        retry = false;
                        break;
            }

            if (retry)
            {
                return downloadWithRetry(curlHandle, 
                        retryOnAuthError, tryNum + 1, maxTries);
            }
            else
            {
                return -1;
            }
        }

        // If we're here, we have a good response.  Return success.
        return 0;
    }


    size_t 
    DownloadBuffer::dataCallback(char *newData, size_t size, size_t nmemb, void *userdata)
    {
        if (size == 0 || nmemb == 0)
        {
            // No data
            return 0;
        }

        DownloadBuffer* pBuffer = (DownloadBuffer*) userdata;

        size_t dataSize = size * nmemb;
        pBuffer->mData.write(newData, dataSize);
        
        return pBuffer->mData.good() ? dataSize : 0;
    }

    size_t
    DownloadBuffer::headerCallback(char* buffer, size_t size, size_t nitems, 
                                 void* userdata)
    {
        DownloadBuffer* pDlBuf = (DownloadBuffer*) userdata;

        size_t newHeaderLength = size * nitems;
        pDlBuf->mReturnedHeaders.write(buffer, newHeaderLength);
        
        return pDlBuf->mReturnedHeaders.good() ? newHeaderLength : 0;
    }


    enum RetryMethod 
    DownloadBuffer::shouldRetry(long httpResp)
    {
        /* TODO:    Currently only handles 403 errors correctly when pBuf->fh is 
         *          NULL (when the downloaded data is stored in-memory, not in a 
         *          file).
         */


        /* Most transfers should retry:
         * A. After HTTP 5xx errors, using exponential backoff
         * B. After HTTP 403 errors with a reason of "rateLimitExceeded" or 
         *    "userRateLimitExceeded", using exponential backoff
         * C. After HTTP 401, after refreshing credentials
         * If not one of the above cases, should not retry.
         */

        if (httpResp >= 500)
        {
            // Always retry these
            return RetryMethod::RETRY;
        }
        else if (httpResp == 401)
        {
            // Always refresh credentials for 401.
            return RetryMethod::RENEWAUTH;
        }
        else if (httpResp == 403)
        {
            // Retry ONLY if the reason for the 403 was an exceeded rate limit
            bool retry = false;
    
            Json jsonRoot(mData.str());
            if (jsonRoot.isValid())
            {
                Json jsonErrors = 
                        jsonRoot.getNestedObject("error/errors");
                string reasonStr = 
                        jsonErrors.getString("reason");
                if ((reasonStr == ERROR_403_RATELIMIT) || 
                        (reasonStr == ERROR_403_USERRATELIMIT))
                {
                    // Rate limit exceeded, retry.
                    retry = true;
                }
                // else do nothing (retry remains false for all other 403 
                // errors)

            }
    //        free(reason);
            if (retry)
            {
                return RetryMethod::RENEWAUTH;
            }
        }

        // For all other errors, don't retry.
        return RetryMethod::NORETRY;
    }

    void DownloadBuffer::doExponentialWait(int tryNum)
    {
        // Number of milliseconds to wait before retrying
        long waitTime;
        int i;
        // Start with 2^tryNum seconds.
        for (i = 0, waitTime = 1000; i < tryNum; i++, waitTime *= 2)
        {
            // Empty loop
        }
        // Randomly add up to 1 second more.
        waitTime += (rand() % 1000) + 1;
        // Convert waitTime to a timespec for use with nanosleep.
        struct timespec waitTimeNano;
        // Intentional integer division:
        waitTimeNano.tv_sec = waitTime / 1000;
        waitTimeNano.tv_nsec = (waitTime % 1000) * 1000000L;
        nanosleep(&waitTimeNano, NULL);
    }


}

