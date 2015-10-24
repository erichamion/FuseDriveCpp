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
    
    
    
    

    class HttpTransfer {
    public:
        typedef size_t(*uploadCallback)
        (Gdrive& gInfo, char* buffer, off_t offset, size_t size, void* userdata);
        
        enum Request_Type
        {
            GET,
            POST,
            PUT,
            PATCH,
            DELETE
        };
        
        HttpTransfer(Gdrive& gInfo);
        
        virtual ~HttpTransfer();

        HttpTransfer& setRequestType(enum Request_Type requestType);

        HttpTransfer& setRetryOnAuthError(bool retry);

        HttpTransfer& setUrl(const std::string& url);

        HttpTransfer& setDestfile(FILE* destFile);

        HttpTransfer& setBody(const std::string& body);

        HttpTransfer& setUploadCallback(uploadCallback callback, 
                                            void* userdata);

        HttpTransfer& addQuery(const std::string& field, 
            const std::string& value);

        HttpTransfer& addPostField(const std::string& field, 
            const std::string& value);

        HttpTransfer& addHeader(const std::string& header);

        int execute();
        
        long getHttpResponse() const;

        std::string getData() const;
        
        std::string getReturnedHeaders() const;

        bool wasSuccessful() const;

    private:
        static const int RETRY_LIMIT;
        
        enum Request_Type mRequestType;
        bool mRetryOnAuthError;
        std::string mUrl;
        HttpQuery* mpQuery;
        HttpQuery* mpPostData;
        const std::string* mpBody;
        struct curl_slist* mpHeaders;
        FILE* mDestFile;
        uploadCallback mUploadCallback;
        void* mUserdata;
        off_t mUploadOffset;
        DownloadBuffer* mpResultBuf;
        Gdrive& mGInfo;
        
        static size_t uploadCallbackInternal(char* buffer, size_t size, 
            size_t nitems, void* instream);

        void getAuthbearerHeader();
        
        HttpTransfer(const HttpTransfer& orig);
        
    };

}

#endif	/* HTTPTRANSFER_HPP */

