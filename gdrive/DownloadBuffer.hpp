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
    
    enum Gdrive_Retry_Method
    {
        GDRIVE_RETRY_NORETRY,
        GDRIVE_RETRY_RETRY,
        GDRIVE_RETRY_RENEWAUTH
    };

    class DownloadBuffer {
    public:
        DownloadBuffer(Gdrive& gInfo, FILE* fh);
        
        virtual ~DownloadBuffer();
        
        long gdrive_dlbuf_get_httpresp();

        std::string gdrive_dlbuf_get_data();

        bool gdrive_dlbuf_get_success();

        CURLcode gdrive_dlbuf_download(CURL* curlHandle);

        int gdrive_dlbuf_download_with_retry(CURL* curlHandle, bool retryOnAuthError,
            int maxTries);
        
    private:
        const std::string GDRIVE_403_RATELIMIT;
        const std::string GDRIVE_403_USERRATELIMIT;
        
        //size_t allocatedSize;
        //size_t usedSize;
        long httpResp;
        CURLcode resultCode;
        std::stringstream data;
        std::stringstream pReturnedHeaders;
        //size_t returnedHeaderSize;
        FILE* mFh;
        Gdrive& gInfo;
        
        int gdrive_dlbuf_download_with_retry(CURL* curlHandle, bool retryOnAuthError, 
                                             int tryNum, int maxTries);

        static size_t 
        gdrive_dlbuf_callback(char *newData, size_t size, size_t nmemb, void *userdata);

        static size_t
        gdrive_dlbuf_header_callback(char* buffer, size_t size, size_t nitems, 
                                     void* userdata);

        enum Gdrive_Retry_Method 
        gdrive_dlbuf_retry_on_error(long httpResp);

        static void gdrive_exponential_wait(int tryNum);
        
        DownloadBuffer(const DownloadBuffer& orig);
        
    };

}

#endif	/* DOWNLOADBUFFER_HPP */

