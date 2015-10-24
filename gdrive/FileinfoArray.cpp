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
    : mGInfo(gInfo)
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

    const Fileinfo* FileinfoArray::first()
    {
        const Fileinfo* returnVal = NULL;
        mNextIndex = 0;
        if (!mContainer.empty())
        {
            returnVal = mContainer[mNextIndex++];
        }
        return returnVal;
    }

    const Fileinfo* FileinfoArray::next()
    {
        if (mNextIndex >= mContainer.size())
        {
            mNextIndex = (unsigned long) (-1);
            return NULL;
        }
        
        return mContainer[mNextIndex++];
    }

    unsigned long FileinfoArray::count() const
    {
        return mContainer.size();
    }

    void FileinfoArray::addFromJson(const Json& jsonObj)
    {
        Fileinfo* pInfo = new Fileinfo(mGInfo, jsonObj);
        mContainer.push_back(pInfo);
    }
}
