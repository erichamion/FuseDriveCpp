/* 
 * File:   Sysinfo.cpp
 * Author: me
 * 
 * Created on October 24, 2015, 11:21 AM
 */

#include "Sysinfo.hpp"
#include "Json.hpp"
#include "Cache.hpp"
#include "Gdrive.hpp"


using namespace std;

namespace fusedrive
{
    Sysinfo::Sysinfo(Gdrive& gInfo)
    : gInfo(gInfo)
    {
        gdrive_sysinfo_cleanup();
    }

    Sysinfo::~Sysinfo()
    {
        // Empty
    }

    void Sysinfo::gdrive_sysinfo_cleanup()
    {
        // Ensure an update happens on the first access
        nextChangeId = INT64_MIN;
        
        quotaBytesTotal = 0;
        quotaBytesUsed = 0;
        rootId = "";
    }

    int64_t Sysinfo::gdrive_sysinfo_get_size()
    {
        gdrive_sysinfo_update_if_stale();
        return quotaBytesTotal;
    }

    int64_t Sysinfo::gdrive_sysinfo_get_used()
    {
        gdrive_sysinfo_update_if_stale();
        return quotaBytesUsed;
    }

    const string& Sysinfo::gdrive_sysinfo_get_rootid()
    {
        gdrive_sysinfo_update_if_stale();
        return rootId;
    }

//    const Sysinfo* Sysinfo::gdrive_sysinfo_get_or_clear(bool cleanup)
//    {
//        
//    }

//    void Sysinfo::gdrive_sysinfo_cleanup_internal()
//    {
//        
//    }

    int Sysinfo::gdrive_sysinfo_fill_from_json(const Json& jsonObj)
    {
        bool currentSuccess = true;
        bool totalSuccess = true;
        nextChangeId = 
                jsonObj.getInt64("largestChangeId", true, currentSuccess)
                + 1;
        totalSuccess = totalSuccess && currentSuccess;

        quotaBytesTotal = 
                jsonObj.getInt64("quotaBytesTotal", true, currentSuccess);
        totalSuccess = totalSuccess && currentSuccess;

        quotaBytesUsed = 
                jsonObj.getInt64("quotaBytesUsed", true, currentSuccess);
        totalSuccess = totalSuccess && currentSuccess;

        rootId = jsonObj.getString("rootFolderId");
        currentSuccess = totalSuccess && (!rootId.empty());

        // For now, we'll ignore the importFormats and exportFormats.

        return totalSuccess ? 0 : -1;
    }


    int Sysinfo::gdrive_sysinfo_update_if_stale()
    {
        // First, make sure the cache is up to date.
        Cache& cache = gInfo.getCache();
        cache.UpdateIfStale();

        // If the Sysinfo's next change ID is at least as high as the cache's
        // next change ID, then our info is current.  No need to do anything
        // else. Otherwise, it needs updated.
        int64_t cacheChangeId = cache.getNextChangeId();
        return (nextChangeId < cacheChangeId) ? gdrive_sysinfo_update() : 0;
    }

    int Sysinfo::gdrive_sysinfo_update()
    {
        gdrive_sysinfo_cleanup();

        const string fieldString = "quotaBytesTotal,quotaBytesUsed,"
                "largestChangeId,rootFolderId,importFormats,exportFormats";

        // Prepare the transfer
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(gInfo);
        if (pTransfer == NULL)
        {
            // Memory error
            return -1;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);
        if (
                gdrive_xfer_set_url(pTransfer, Gdrive::GDRIVE_URL_ABOUT.c_str()) || 
                gdrive_xfer_add_query(gInfo, pTransfer, "includeSubscribed", "false") || 
                gdrive_xfer_add_query(gInfo, pTransfer, "fields", fieldString.c_str())
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
            return -1;
        }

        // Do the transfer.
        DownloadBuffer* pBuf = gdrive_xfer_execute(gInfo, pTransfer);
        gdrive_xfer_free(pTransfer);

        int returnVal = -1;
        if (pBuf != NULL && pBuf->getHttpResponse() < 400)
        {
            // Response was good, try extracting the data.
            Json jsonObj(pBuf->getData());
            if (jsonObj.isValid())
            {
                returnVal = gdrive_sysinfo_fill_from_json(jsonObj);
            }
        }

        delete pBuf;

        return returnVal;
    }

}