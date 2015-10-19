/* 
 * File:   Util.cpp
 * Author: me
 * 
 * Created on October 18, 2015, 3:32 PM
 */

#include "Util.hpp"
#include "gdrive-util.h"

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>

using namespace std;
namespace fusedrive
{
    long Util::gdrive_divide_round_up(long dividend, long divisor)
    {
        // Could use ceill() or a similar function for this, but I don't  know 
        // whether there might be some values that don't convert exactly between
        // long int and long double and back.

        // Integer division rounds down.  If there's a remainder, add 1.
        return (dividend % divisor == 0) ? 
            (dividend / divisor) : 
            (dividend / divisor + 1);
    }

    FILE* Util::gdrive_power_fopen(const string& path, const string& mode)
    {
        // Any files we create would be authentication files and possibly (not 
        // currently implemented) configuration files. These should be visible 
        // only to the user.
        mode_t oldUmask = umask(S_IRGRP | S_IWGRP | S_IXGRP | 
                                S_IROTH | S_IWOTH | S_IXOTH);

        Gdrive_Path* pGpath = gdrive_path_create(path.c_str());
        const string dirname(gdrive_path_get_dirname(pGpath));

        FILE* returnVal = NULL;

        // Does the parent directory exist?
        if (access(dirname.c_str(), F_OK))
        {
            // Directory doesn't exist, need to create it.
            if (!gdrive_recursive_mkdir(dirname))
            {
                // Successfully created directory
                returnVal = fopen(path.c_str(), mode.c_str());
            }
            // else do nothing, cleanup and return failure
        }
        else
        {
            // Directory exists, just need to open the file
            returnVal = fopen(path.c_str(), mode.c_str());
        }

        umask(oldUmask);
        gdrive_path_free(pGpath);
        return returnVal;
    }

    int Util::gdrive_recursive_mkdir(const string& path)
    {
        Gdrive_Path* pGpath = gdrive_path_create(path.c_str());
        const string parentDir(gdrive_path_get_dirname(pGpath));

        int returnVal;
        // Does the parent directory exist?
        if (access(parentDir.c_str(), F_OK))
        {
            // Directory doesn't exist, need to create it.
            returnVal = gdrive_recursive_mkdir(parentDir);
            if (!returnVal)
            {
                // Successfully created directory
                returnVal = mkdir(path.c_str(), 0755);
            }
            // else do nothing, cleanup and return failure
        }
        else
        {
            // Directory exists, just need to open the file
            returnVal = mkdir(path.c_str(), 0755);
        }

        gdrive_path_free(pGpath);
        return returnVal;
    }

    size_t Util::strptime(const std::string& s, const string& format, struct tm* tm, bool initTm)
    {
        if (initTm)
        {
            memset(tm, 0, sizeof(struct tm));
        }
        
        const char* sc = s.c_str();
        const char* end = ::strptime(sc, format.c_str(), tm);
        if (!end)
        {
            // Conversion failure
            throw new exception();
        }
        size_t returnVal = end - sc;
        if (returnVal >= s.length())
        {
            returnVal = string::npos;
        }
        
        return returnVal;
    }
    
    double Util::strtod(const std::string& nstr)
    {
        size_t dummy = 0;
        return strtod(nstr, dummy);
    }

    double Util::strtod(const std::string& nstr, size_t& index)
    {
        const char* nptr = nstr.c_str();
        const char* startptr = nptr + index;
        char* endptr = NULL;
        double returnVal = ::strtod(startptr, &endptr);
        size_t newIndex = endptr - nptr;
        index = (newIndex >= nstr.length()) ? string::npos : newIndex;
        return returnVal;
    }
    
    long Util::strtol(const std::string& nstr, int base)
    {
        size_t dummy = 0;
        return strtol(nstr, dummy, base);
    }

    long Util::strtol(const std::string& nstr, size_t& index, int base)
    {
        const char* nptr = nstr.c_str();
        const char* startptr = nptr + index;
        char* endptr = NULL;
        long returnVal = ::strtol(startptr, &endptr, base);
        size_t newIndex = endptr - nptr;
        index = (newIndex >= nstr.length()) ? string::npos : newIndex;
        return returnVal;
    }
    
    std::string Util::strftime(const std::string& format, const struct tm* tm)
    {
        if (!tm->tm_year && !tm->tm_mon && !tm->tm_mday &&
                !tm->tm_hour && !tm->tm_min && !tm->tm_sec)
        {
            // Nothing here.
            return string("");
        }
        
        // Copied/adapted from 
        // http://stackoverflow.com/questions/7935483/c-function-to-format-time
        // -t-as-stdstring-buffer-length
        
        string formatString = format + '\a'; //force at least one character in the result
        std::string buffer;
        buffer.resize(formatString.size());
        int len = ::strftime(&buffer[0], buffer.size(), 
                formatString.c_str(), tm);
        while (len == 0) 
        {
            buffer.resize(buffer.size()*2);
            len = ::strftime(&buffer[0], buffer.size(), 
                    formatString.c_str(), tm);
        } 
        buffer.resize(len-1); //remove that trailing '\a'
        return buffer;
    }
}