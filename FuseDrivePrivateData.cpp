/* 
 * File:   FuseDrivePrivateData.cpp
 * Author: me
 * 
 * Created on October 16, 2015, 11:49 PM
 */

#include "FuseDrivePrivateData.hpp"

FuseDrivePrivateData::FuseDrivePrivateData(const fusedrive::Options& options)
: mGdrive(options.gdrive_access, options.gdrive_auth_file, 
        options.gdrive_cachettl, options.gdrive_interaction_type, 
        options.gdrive_chunk_size, options.gdrive_max_chunks)
{
    mPerms = (options.dir_perms << 9) + options.file_perms;
}

FuseDrivePrivateData::~FuseDrivePrivateData() 
{
}

unsigned long FuseDrivePrivateData::getPerms()
{
    return mPerms;
}

fusedrive::Gdrive& FuseDrivePrivateData::getGdrive()
{
    return mGdrive;
}

