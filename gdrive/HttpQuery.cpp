/* 
 * File:   HttpQuery.cpp
 * Author: me
 * 
 * Created on October 24, 2015, 9:43 AM
 */

#include "HttpQuery.hpp"
#include "Gdrive.hpp"

#include <curl/curl.h>

using namespace std;

namespace fusedrive
{
    HttpQuery::HttpQuery(Gdrive& gInfo, const string& field, const string& value)
    : mGInfo(gInfo)
    {
        mpNext = NULL;
        
        // Need to URL-escape the field and value
        CURL* curlHandle = gInfo.getCurlHandle();
        if (!curlHandle)
        {
            // Error
            throw new exception();
        }
        // If curl_easy_escape fails, it returns NULL. Assigning NULL
        // pointer to a string will throw an exception.
        char* curlFieldStr = curl_easy_escape(curlHandle, field.c_str(), 0);
        char* curlValueStr = curl_easy_escape(curlHandle, value.c_str(), 0);
        
        this->mField.assign(curlFieldStr);
        this->mValue.assign(curlValueStr);
        
        curl_free(curlFieldStr);
        curl_free(curlValueStr);
        curl_easy_cleanup(curlHandle);
    }
    
    HttpQuery::~HttpQuery()
    {
        // Free the rest of the list
        if (mpNext)
        {
            delete mpNext;
        }
        
        // Nothing else to do here
    }

    HttpQuery& HttpQuery::add(const string& field, const string& value)
    {
        // Add a new empty Gdrive_Query to the end of the list (unless the first
        // one is already empty, as it will be if we just created it).
        HttpQuery* pLast = this;
        
        while (pLast->mpNext != NULL)
        {
            pLast = pLast->mpNext;
        }
        pLast->mpNext = new HttpQuery(mGInfo, field, value);
        
        return *this;
    }

    string HttpQuery::assemble(const string& url)
    {
        stringstream returnStream;
        assembleInternal(&url, returnStream);
        return returnStream.str();
    }
    
    std::string HttpQuery::assembleAsPostData()
    {
        stringstream returnStream;
        assembleInternal(NULL, returnStream);
        return returnStream.str();
    }
    
    void HttpQuery::assembleInternal(const std::string* pUrl, 
            std::stringstream& outStream)
    {
//        // If there is a url, allow for its length plus the '?' character (or the
//        // url length plus terminating null if there is no query string).
////        int totalLength = (url == NULL) ? 0 : (strlen(url) + 1);
//
//        // If there is a query string (or POST data, which is handled the same way),
//        // each field adds its length plus 1 for the '=' character. Each value adds
//        // its length plus 1, for either the '&' character (all but the last item)
//        // or the terminating null (on the last item).
//        const Gdrive_Query* pCurrentQuery = pQuery;
//        while (pCurrentQuery != NULL)
//        {
//            if (pCurrentQuery->field != NULL && pCurrentQuery->value != NULL)
//            {
////                totalLength += strlen(pCurrentQuery->field) + 1;
////                totalLength += strlen(pCurrentQuery->value) + 1;
//            }
//            else
//            {
//                // The query is empty.  Add 1 to the length for the null terminator
////                totalLength++;
//            }
//            pCurrentQuery = pCurrentQuery->pNext;
//        }
//
//        if (totalLength < 1)
//        {
//            // Invalid arguments
//            return NULL;
//        }

        // Allocate a string long enough to hold everything.
//        char* result = (char*) malloc(totalLength);
//        if (result == NULL)
//        {
//            // Memory error
//            return NULL;
//        }

        // Copy the url into the result string.  If there is no url, start with an
        // empty string.
        if (pUrl != NULL)
        {
            outStream << *pUrl;
//            strcpy(result, url);
//            if (pQuery == NULL)
//            {
//                // We had a URL string but no query string, so we're done.
//                return result;
//            }
//            else
//            {
                // We have both a URL and a query, so they need to be separated
                // with a '?'.
            outStream << '?';
//                strcat(result, "?");
//            }
        }
//        else
//        {
//            result[0] = '\0';
//        }

//        if (pQuery == NULL)
//        {
//            // There was no query string, so we're done.
//            return result;
//        }

        // Copy each of the query field/value pairs into the result.
        HttpQuery* pCurrentQuery = this;
        do
        {
//            if (pCurrentQuery->field != NULL && pCurrentQuery->value != NULL)
//            {
//                strcat(result, pCurrentQuery->field);
//                strcat(result, "=");
//                strcat(result, pCurrentQuery->value);
//            }
            outStream << pCurrentQuery->mField << '=' << pCurrentQuery->mValue;
            if (pCurrentQuery->mpNext)
            {
//                strcat(result, "&");
                outStream << '&';
            }
            pCurrentQuery = pCurrentQuery->mpNext;
        } while (pCurrentQuery);

    }
}