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
        
        HttpQuery& add(const std::string& field, 
            const std::string& value);

        std::string assemble(const std::string& url);
        
        std::string assembleAsPostData();

    private:
        std::string mField;
        std::string mValue;
        HttpQuery* mpNext;
        Gdrive& mGInfo;
        
        HttpQuery(const HttpQuery& orig);
        
        void assembleInternal(const std::string* pUrl, 
            std::stringstream& outStr);
        
    };

}

#endif	/* HTTPQUERY_HPP */

