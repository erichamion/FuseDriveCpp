/* 
 * File:   Path.cpp
 * Author: me
 * 
 * Created on October 24, 2015, 3:47 PM
 */

#include "Path.hpp"

#include <cstring>
#include <libgen.h>

using namespace std;

namespace fusedrive
{

    Path::Path(const string& path) 
    : mFullPath(path)
    {
        mHasDirString = false;
        mHasBaseString = false;
    }

    Path::~Path() 
    {
        // Empty
    }
    
    const string& Path::getDirname() const
    {
        if (mHasDirString)
        {
            // The dirname has already been extracted. Just return it.
            return mDirname;
        }
        
        char* pathCopy = new char[mFullPath.length() + 1];
        strcpy(pathCopy, mFullPath.c_str());
        char* dirName = dirname(pathCopy);
        mDirname = (string) dirName;
        delete[] pathCopy;
        
        return mDirname;
    }

    const string& Path::getBasename() const
    {
        if (mHasBaseString)
        {
            // The dirname has already been extracted. Just return it.
            return mBasename;
        }
        
        char* pathCopy = new char[mFullPath.length() + 1];
        strcpy(pathCopy, mFullPath.c_str());
        char* baseName = basename(pathCopy);
        mBasename = (string) baseName;
        delete[] pathCopy;
        
        return mBasename;
    }

}