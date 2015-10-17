/* 
 * File:   FuseDrivePrivateData.hpp
 * Author: me
 *
 * Created on October 16, 2015, 11:49 PM
 */

#ifndef FUSEDRIVEPRIVATEDATA_HPP
#define	FUSEDRIVEPRIVATEDATA_HPP

#include "Options.hpp"

class FuseDrivePrivateData {
public:
    FuseDrivePrivateData(const fusedrive::Options& options);
    virtual ~FuseDrivePrivateData();
    
    unsigned long getPerms();
    fusedrive::Gdrive& getGdrive();
    
private:
    FuseDrivePrivateData(const FuseDrivePrivateData& orig);
    fusedrive::Gdrive mGdrive;
    unsigned long mPerms;
};

#endif	/* FUSEDRIVEPRIVATEDATA_HPP */

