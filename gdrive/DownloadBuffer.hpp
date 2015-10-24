/* 
 * File:   DownloadBuffer.hpp
 * Author: me
 *
 * Created on October 22, 2015, 8:16 PM
 */

#ifndef DOWNLOADBUFFER_HPP
#define	DOWNLOADBUFFER_HPP

#include <curl/curl.h>
#include <string>
#include <sstream>

namespace fusedrive
{
    class Gdrive;
    
    enum RetryMethod
    {
        NORETRY,
        RETRY,
        RENEWAUTH
    };

    class DownloadBuffer {
    public:
        DownloadBuffer(Gdrive& gInfo, FILE* fh);
        
        virtual ~DownloadBuffer();
        
        long getHttpResponse() const;

        std::string getData() const;
        
        std::string getHeaders() const;

        bool wasSuccessful() const;

        int downloadWithRetry(CURL* curlHandle, bool retryOnAuthError,
            int maxTries);
        
    private:
        const std::string ERROR_403_RATELIMIT;
        const std::string ERROR_403_USERRATELIMIT;
        
        long mHttpResp;
        CURLcode mResultCode;
        std::stringstream mData;
        std::stringstream mReturnedHeaders;
        FILE* mFh;
        Gdrive& mGInfo;
        
        CURLcode download(CURL* curlHandle);

        int downloadWithRetry(CURL* curlHandle, bool retryOnAuthError, 
            int tryNum, int maxTries);

        static size_t dataCallback(char *newData, size_t size, 
            size_t nmemb, void *userdata);

        static size_t headerCallback(char* buffer, size_t size, size_t nitems,
            void* userdata);

        enum RetryMethod shouldRetry(long httpResp);

        static void doExponentialWait(int tryNum);
        
        DownloadBuffer(const DownloadBuffer& orig);
        
    };

}

#endif	/* DOWNLOADBUFFER_HPP */

