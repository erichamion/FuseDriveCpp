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

        const Fileinfo* first();

        const Fileinfo* next();

        unsigned long count() const;

        void addFromJson(const Json& jsonObj);

    private:
        std::vector<Fileinfo*> mContainer;
        unsigned long mNextIndex;
        Gdrive& mGInfo;
        
        FileinfoArray(const FileinfoArray& orig);
        
    };

}

#endif	/* FILEINFOARRAY_HPP */

