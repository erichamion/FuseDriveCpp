
#include "gdrive-sysinfo.hpp"

#include "Gdrive.hpp"
#include "Cache.hpp"

#include <string.h>
#include <stdbool.h>
    
using namespace fusedrive;

typedef struct Gdrive_Sysinfo
{
    // nextChangeId: For internal use
    int64_t nextChangeId;
    // quotaBytesTotal: Total space on Google Drive, in bytes
    int64_t quotaBytesTotal;
    // quotaBytesUsed: Space already used, in bytes
    int64_t quotaBytesUsed;
    // rootId: Google Drive file ID for the root folder
    char* rootId;
    
} Gdrive_Sysinfo;    


/*************************************************************************
 * Private struct and declarations of private functions for use within 
 * this file
 *************************************************************************/

static const Gdrive_Sysinfo* gdrive_sysinfo_get_or_clear(Gdrive& gInfo, bool cleanup);

static void gdrive_sysinfo_cleanup_internal(Gdrive_Sysinfo* pSysinfo);

static int gdrive_sysinfo_fill_from_json(Gdrive_Sysinfo* pDest, 
                                         Json& jsonObj);

static int gdrive_sysinfo_update(Gdrive& gInfo, Gdrive_Sysinfo* pDest);


/*************************************************************************
 * Implementations of public functions for internal or external use
 *************************************************************************/

/******************
 * Constructors, factory methods, destructors and similar
 ******************/

// No constructors. This is a single struct instance that lives
// in static memory for the lifetime of the application. Members are retrieved
// using the gdrive_sysinfo_get_*() functions below.

void gdrive_sysinfo_cleanup(Gdrive& gInfo)
{
    gdrive_sysinfo_get_or_clear(gInfo, true);
}


/******************
 * Getter and setter functions
 ******************/

int64_t gdrive_sysinfo_get_size(Gdrive& gInfo)
{
    return gdrive_sysinfo_get_or_clear(gInfo, false)->quotaBytesTotal;
}

int64_t gdrive_sysinfo_get_used(Gdrive& gInfo)
{
    return gdrive_sysinfo_get_or_clear(gInfo, false)->quotaBytesUsed;
}

const char* gdrive_sysinfo_get_rootid(Gdrive& gInfo)
{
    return gdrive_sysinfo_get_or_clear(gInfo, false)->rootId;
}


/******************
 * Other accessible functions
 ******************/

// No other public functions


/*************************************************************************
 * Implementations of private functions for use within this file
 *************************************************************************/

static const Gdrive_Sysinfo* gdrive_sysinfo_get_or_clear(Gdrive& gInfo, bool cleanup)
{
    // Set the initial nextChangeId to the lowest possible value, guaranteeing
    // that the info will be updated the first time this function is called.
    static Gdrive_Sysinfo sysinfo = {.nextChangeId = INT64_MIN};
    
    if (cleanup)
    {
        // Clear out the struct and return NULL
        gdrive_sysinfo_cleanup_internal(&sysinfo);
        return NULL;
    }
    
    

    // Is the info current?
    // First, make sure the cache is up to date.
    Cache& cache = gInfo.gdrive_get_cache();
    cache.UpdateIfStale();

    // If the Sysinfo's next change ID is at least as high as the cache's
    // next change ID, then our info is current.  No need to do anything
    // else. Otherwise, it needs updated.
    int64_t cacheChangeId = cache.getNextChangeId();
    if (sysinfo.nextChangeId < cacheChangeId)
    {
        // Either we don't have any sysinfo, or it needs updated.
        gdrive_sysinfo_update(gInfo, &sysinfo);
    }
    
    
    return &sysinfo;
}

static void gdrive_sysinfo_cleanup_internal(Gdrive_Sysinfo* pSysinfo)
{
    free(pSysinfo->rootId);
    memset(pSysinfo, 0, sizeof(Gdrive_Sysinfo));
    pSysinfo->nextChangeId = INT64_MIN;
}

static int gdrive_sysinfo_fill_from_json(Gdrive_Sysinfo* pDest, 
                                         Json& jsonObj)
{
    bool currentSuccess = true;
    bool totalSuccess = true;
    pDest->nextChangeId = jsonObj.gdrive_json_get_int64("largestChangeId", 
            true, currentSuccess) + 1;
    totalSuccess = totalSuccess && currentSuccess;
    
    pDest->quotaBytesTotal = jsonObj.gdrive_json_get_int64("quotaBytesTotal", 
            true,currentSuccess);
    totalSuccess = totalSuccess && currentSuccess;
    
    pDest->quotaBytesUsed = jsonObj.gdrive_json_get_int64("quotaBytesUsed", 
            true, currentSuccess);
    totalSuccess = totalSuccess && currentSuccess;
    
    pDest->rootId = 
            strdup(jsonObj.gdrive_json_get_string("rootFolderId").c_str());
    currentSuccess = totalSuccess && (pDest->rootId != NULL);
    
    // For now, we'll ignore the importFormats and exportFormats.
    
    return totalSuccess ? 0 : -1;
}

static int gdrive_sysinfo_update(Gdrive& gInfo, Gdrive_Sysinfo* pDest)
{
    if (pDest != NULL)
    {
        // Clean up the existing info.
        gdrive_sysinfo_cleanup_internal(pDest);
    }
        
    const char* const fieldString = "quotaBytesTotal,quotaBytesUsed,"
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
            gdrive_xfer_add_query(gInfo, pTransfer, "fields", fieldString)
        )
    {
        // Error
        gdrive_xfer_free(pTransfer);
        return -1;
    }
    
    // Do the transfer.
    Gdrive_Download_Buffer* pBuf = gdrive_xfer_execute(gInfo, pTransfer);
    gdrive_xfer_free(pTransfer);
    
    int returnVal = -1;
    if (pBuf != NULL && gdrive_dlbuf_get_httpresp(pBuf) < 400)
    {
        // Response was good, try extracting the data.
        Json jsonObj(gdrive_dlbuf_get_data(pBuf));
        if (jsonObj.gdrive_json_is_valid())
        {
            returnVal = gdrive_sysinfo_fill_from_json(pDest, jsonObj);
        }
    }
    
    gdrive_dlbuf_free(pBuf);
    
    return returnVal;
}



