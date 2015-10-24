/* 
 * File:   FileinfoArray.cpp
 * Author: me
 * 
 * Created on October 23, 2015, 10:09 PM
 */

#include "FileinfoArray.hpp"
#include "FileContents.hpp"

using namespace std;

namespace fusedrive
{
    FileinfoArray::FileinfoArray(Gdrive& gInfo)
    : gInfo(gInfo)
    {
        mNextIndex = 0;
    }
        
    FileinfoArray::~FileinfoArray()
    {
        for (vector<Fileinfo*>::iterator iter = mContainer.begin(); 
                iter != mContainer.end(); 
                ++iter)
        {
            delete *iter;
        }
    }

    const Fileinfo* FileinfoArray::gdrive_finfoarray_get_first()
    {
        const Fileinfo* returnVal = NULL;
        mNextIndex = 0;
        if (!mContainer.empty())
        {
            returnVal = mContainer[mNextIndex++];
        }
        return returnVal;
    }

    const Fileinfo* FileinfoArray::gdrive_finfoarray_get_next()
    {
        if (mNextIndex >= mContainer.size())
        {
            mNextIndex = (unsigned long) (-1);
            return NULL;
        }
        
        return mContainer[mNextIndex++];
    }

    unsigned long FileinfoArray::gdrive_finfoarray_get_count()
    {
        return mContainer.size();
    }

    void FileinfoArray::gdrive_finfoarray_add_from_json(Json& jsonObj)
    {
        Fileinfo* pInfo = new Fileinfo(gInfo, jsonObj);
        mContainer.push_back(pInfo);
    }
}
