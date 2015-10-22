/* 
 * File:   GdriveEnums.hpp
 * Author: me
 *
 * Created on October 19, 2015, 9:15 PM
 */

#ifndef GDRIVEENUMS_HPP
#define	GDRIVEENUMS_HPP

namespace fusedrive
{
    enum Gdrive_Interaction
    {
        GDRIVE_INTERACTION_NEVER,
        GDRIVE_INTERACTION_STARTUP,
        GDRIVE_INTERACTION_ALWAYS
    };

    enum Gdrive_Filetype
    {
        // May add a Google Docs file type or others
        GDRIVE_FILETYPE_FILE,
        GDRIVE_FILETYPE_FOLDER
    };
}


#endif	/* GDRIVEENUMS_HPP */

