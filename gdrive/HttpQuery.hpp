/* 
 * File:   HttpQuery.hpp
 * Author: me
 *
 * Created on October 24, 2015, 9:43 AM
 */

#ifndef HTTPQUERY_HPP
#define	HTTPQUERY_HPP

#include <string>

namespace fusedrive
{
    class Gdrive;

    class HttpQuery {
    public:
        HttpQuery(Gdrive& gInfo, const std::string& field, 
                const std::string& value);
        
        virtual ~HttpQuery();
        
        HttpQuery& gdrive_query_add(const std::string& field, 
            const std::string& value);

        std::string gdrive_query_assemble(const std::string& url);
        
        std::string gdrive_query_assemble_as_post_data();

    private:
        std::string field;
        std::string value;
        HttpQuery* pNext;
        Gdrive& gInfo;
        
        HttpQuery(const HttpQuery& orig);
        
        void gdrive_query_assemble_internal(const std::string* pUrl, 
            std::stringstream& outStr);
        
    };

}

#endif	/* HTTPQUERY_HPP */

