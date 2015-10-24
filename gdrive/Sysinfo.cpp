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
    : mGInfo(gInfo)
    {
        clear();
    }

    Sysinfo::~Sysinfo()
    {
        // Empty
    }

    void Sysinfo::clear()
    {
        // Ensure an update happens on the first access
        mNextChangeId = INT64_MIN;
        
        mQuotaBytesTotal = 0;
        mQuotaBytesUsed = 0;
        mRootId = "";
    }

    int64_t Sysinfo::size()
    {
        updateIfStale();
        return mQuotaBytesTotal;
    }

    int64_t Sysinfo::used()
    {
        updateIfStale();
        return mQuotaBytesUsed;
    }

    const string& Sysinfo::rootId()
    {
        updateIfStale();
        return mRootId;
    }

//    const Sysinfo* Sysinfo::gdrive_sysinfo_get_or_clear(bool cleanup)
//    {
//        
//    }

//    void Sysinfo::gdrive_sysinfo_cleanup_internal()
//    {
//        
//    }

    int Sysinfo::fillFromJson(const Json& jsonObj)
    {
        bool currentSuccess = true;
        bool totalSuccess = true;
        mNextChangeId = 
                jsonObj.getInt64("largestChangeId", true, currentSuccess)
                + 1;
        totalSuccess = totalSuccess && currentSuccess;

        mQuotaBytesTotal = 
                jsonObj.getInt64("quotaBytesTotal", true, currentSuccess);
        totalSuccess = totalSuccess && currentSuccess;

        mQuotaBytesUsed = 
                jsonObj.getInt64("quotaBytesUsed", true, currentSuccess);
        totalSuccess = totalSuccess && currentSuccess;

        mRootId = jsonObj.getString("rootFolderId");
        currentSuccess = totalSuccess && (!mRootId.empty());

        // For now, we'll ignore the importFormats and exportFormats.

        return totalSuccess ? 0 : -1;
    }


    int Sysinfo::updateIfStale()
    {
        // First, make sure the cache is up to date.
        Cache& cache = mGInfo.getCache();
        cache.UpdateIfStale();

        // If the Sysinfo's next change ID is at least as high as the cache's
        // next change ID, then our info is current.  No need to do anything
        // else. Otherwise, it needs updated.
        int64_t cacheChangeId = cache.getNextChangeId();
        return (mNextChangeId < cacheChangeId) ? update() : 0;
    }

    int Sysinfo::update()
    {
        clear();

        const string fieldString = "quotaBytesTotal,quotaBytesUsed,"
                "largestChangeId,rootFolderId,importFormats,exportFormats";

        // Prepare the transfer
        Gdrive_Transfer* pTransfer = gdrive_xfer_create(mGInfo);
        if (pTransfer == NULL)
        {
            // Memory error
            return -1;
        }
        gdrive_xfer_set_requesttype(pTransfer, GDRIVE_REQUEST_GET);
        if (
                gdrive_xfer_set_url(pTransfer, Gdrive::GDRIVE_URL_ABOUT.c_str()) || 
                gdrive_xfer_add_query(mGInfo, pTransfer, "includeSubscribed", "false") || 
                gdrive_xfer_add_query(mGInfo, pTransfer, "fields", fieldString.c_str())
            )
        {
            // Error
            gdrive_xfer_free(pTransfer);
            return -1;
        }

        // Do the transfer.
        DownloadBuffer* pBuf = gdrive_xfer_execute(mGInfo, pTransfer);
        gdrive_xfer_free(pTransfer);

        int returnVal = -1;
        if (pBuf != NULL && pBuf->getHttpResponse() < 400)
        {
            // Response was good, try extracting the data.
            Json jsonObj(pBuf->getData());
            if (jsonObj.isValid())
            {
                returnVal = fillFromJson(jsonObj);
            }
        }

        delete pBuf;

        return returnVal;
    }

}