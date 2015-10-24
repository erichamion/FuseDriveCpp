/* 
 * File:   FileinfoArray.hpp
 * Author: me
 *
 * Created on October 23, 2015, 10:09 PM
 */

#ifndef FILEINFOARRAY_HPP
#define	FILEINFOARRAY_HPP

#include "Fileinfo.hpp"

#include <vector>

namespace fusedrive
{
    class Gdrive;

    class FileinfoArray {
    public:
        FileinfoArray(Gdrive& gInfo);
        
        virtual ~FileinfoArray();

        const Fileinfo* 
        gdrive_finfoarray_get_first();

        const Fileinfo* gdrive_finfoarray_get_next();

        unsigned long gdrive_finfoarray_get_count();

        void gdrive_finfoarray_add_from_json(Json& jsonObj);

    private:
        std::vector<Fileinfo*> mContainer;
        unsigned long mNextIndex;
        Gdrive& gInfo;
        
        FileinfoArray(const FileinfoArray& orig);
        
    };

}

#endif	/* FILEINFOARRAY_HPP */

