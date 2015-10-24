/* 
 * File:   HttpTransfer.hpp
 * Author: me
 *
 * Created on October 24, 2015, 12:06 PM
 */

#ifndef HTTPTRANSFER_HPP
#define	HTTPTRANSFER_HPP

#include <stddef.h>
#include <string>
#include <curl/curl.h>



namespace fusedrive
{
    class Gdrive;
    class DownloadBuffer;
    class HttpQuery;
    
    enum Gdrive_Request_Type
    {
        GDRIVE_REQUEST_GET,
        GDRIVE_REQUEST_POST,
        GDRIVE_REQUEST_PUT,
        GDRIVE_REQUEST_PATCH,
        GDRIVE_REQUEST_DELETE
    };
    
    typedef size_t(*gdrive_xfer_upload_callback)
    (fusedrive::Gdrive& gInfo, char* buffer, off_t offset, size_t size, void* userdata);

    class HttpTransfer {
    public:
        HttpTransfer(Gdrive& gInfo);
        
        virtual ~HttpTransfer();
        
        Gdrive& gdrive_xfer_get_gdrive();

        HttpTransfer& gdrive_xfer_set_requesttype(enum Gdrive_Request_Type requestType);

        HttpTransfer& gdrive_xfer_set_retryonautherror(bool retry);

        HttpTransfer& gdrive_xfer_set_url(const std::string& url);

        HttpTransfer& gdrive_xfer_set_destfile(FILE* destFile);

        HttpTransfer& gdrive_xfer_set_body(const std::string& body);

        HttpTransfer& gdrive_xfer_set_uploadcallback(gdrive_xfer_upload_callback callback, 
                                            void* userdata);

        HttpTransfer& gdrive_xfer_add_query(const std::string& field, 
            const std::string& value);

        HttpTransfer& gdrive_xfer_add_postfield(const std::string& field, 
            const std::string& value);

        HttpTransfer& gdrive_xfer_add_header(const std::string& header);

        int gdrive_xfer_execute();
        
        long getHttpResponse() const;

        std::string getData() const;
        
        std::string getReturnedHeaders() const;

        bool wasSuccessful() const;

    private:
        static const int GDRIVE_RETRY_LIMIT;
        
        enum Gdrive_Request_Type requestType;
        bool retryOnAuthError;
        std::string url;
        HttpQuery* pQuery;
        HttpQuery* pPostData;
        const std::string* body;
        struct curl_slist* pHeaders;
        FILE* destFile;
        gdrive_xfer_upload_callback uploadCallback;
        void* userdata;
        off_t uploadOffset;
        DownloadBuffer* mpResultBuf;
        Gdrive& mGInfo;
        
//        int gdrive_xfer_add_query_or_post(HttpQuery** ppQuery, 
//            const std::string& field, const std::string& value);

        static size_t gdrive_xfer_upload_callback_internal(char* buffer, 
            size_t size, size_t nitems, void* instream);

        struct curl_slist* 
        gdrive_get_authbearer_header(struct curl_slist* pHeaders);
        
        HttpTransfer(const HttpTransfer& orig);
        
    };

}

#endif	/* HTTPTRANSFER_HPP */

