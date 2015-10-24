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
        
        void gdrive_sysinfo_cleanup();

        int64_t gdrive_sysinfo_get_size();

        int64_t gdrive_sysinfo_get_used();

        const std::string& gdrive_sysinfo_get_rootid();
        
    private:
        int64_t nextChangeId;
        int64_t quotaBytesTotal;
        int64_t quotaBytesUsed;
        std::string rootId;
        Gdrive& gInfo;
        
        //const Sysinfo* gdrive_sysinfo_get_or_clear(bool cleanup);

        //void gdrive_sysinfo_cleanup_internal();

        int gdrive_sysinfo_fill_from_json(const Json& jsonObj);
        
        int gdrive_sysinfo_update_if_stale();

        int gdrive_sysinfo_update();
        
        Sysinfo(const Sysinfo& orig);
        
    };

}

#endif	/* SYSINFO_HPP */

