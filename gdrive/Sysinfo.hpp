/* 
 * File:   Sysinfo.hpp
 * Author: me
 *
 * Created on October 24, 2015, 11:21 AM
 */

#ifndef SYSINFO_HPP
#define	SYSINFO_HPP

#include <cstdint>
#include <string>

namespace fusedrive
{
    class Gdrive;
    class Json;

    class Sysinfo {
    public:
        Sysinfo(Gdrive& gInfo);
        
        virtual ~Sysinfo();
        
        void clear();

        int64_t size();

        int64_t used();

        const std::string& rootId();
        
    private:
        int64_t mNextChangeId;
        int64_t mQuotaBytesTotal;
        int64_t mQuotaBytesUsed;
        std::string mRootId;
        Gdrive& mGInfo;
        
        int fillFromJson(const Json& jsonObj);
        
        int updateIfStale();

        int update();
        
        Sysinfo(const Sysinfo& orig);
        
    };

}

#endif	/* SYSINFO_HPP */

