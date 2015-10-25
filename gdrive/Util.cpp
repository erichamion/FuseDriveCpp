/* 
 * File:   Util.cpp
 * Author: me
 * 
 * Created on October 18, 2015, 3:32 PM
 */

#include "Util.hpp"
#include "Path.hpp"

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include <math.h>
#include <assert.h>
#include <sstream>
#include <iomanip>
#include <cstdarg>

using namespace std;
namespace fusedrive
{
    long Util::divideCeil(long dividend, long divisor)
    {
        // Could use ceill() or a similar function for this, but I don't  know 
        // whether there might be some values that don't convert exactly between
        // long int and long double and back.

        // Integer division rounds down.  If there's a remainder, add 1.
        return (dividend % divisor == 0) ? 
            (dividend / divisor) : 
            (dividend / divisor + 1);
    }

    FILE* Util::recursiveFopen(const string& path, const string& mode)
    {
        // Any files we create would be authentication files and possibly (not 
        // currently implemented) configuration files. These should be visible 
        // only to the user.
        mode_t oldUmask = umask(S_IRGRP | S_IWGRP | S_IXGRP | 
                                S_IROTH | S_IWOTH | S_IXOTH);

        Path gpath(path);
        const string& dirname = gpath.getDirname();

        FILE* returnVal = NULL;

        // Does the parent directory exist?
        if (access(dirname.c_str(), F_OK))
        {
            // Directory doesn't exist, need to create it.
            if (!recursiveMkdir(dirname))
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
        return returnVal;
    }

    int Util::recursiveMkdir(const string& path)
    {
        Path gpath(path);
        const string& parentDir = gpath.getDirname();

        int returnVal;
        // Does the parent directory exist?
        if (access(parentDir.c_str(), F_OK))
        {
            // Directory doesn't exist, need to create it.
            returnVal = recursiveMkdir(parentDir);
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

        return returnVal;
    }
    
    int Util::sprintf(std::string& str, const string& format, ...)
    {
        str.clear();
        
        // Find the required length
        va_list args;
        va_start(args, format);
        int resultSize = ::vsnprintf(NULL, 0, format.c_str(), args) + 1;
        va_end(args);
        
        if (resultSize <= 1)
        {
            // Result is empty, no need to continue
            return 0;
        }
        
        va_start(args, format);
        char* cStr = new char[resultSize];
        int returnVal = ::vsnprintf(cStr, resultSize, format.c_str(), args);
        str.assign(cStr);
        delete cStr;
        va_end(args);
        
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
    
    int Util::rfc3339ToEpochTimeNS(const string& rfcTime, 
            struct timespec* pResultTime)
    {
        // Get the time down to seconds. Don't do anything with it yet, because
        // we still need to confirm the timezone.
        struct tm epochTime = {0};
        size_t remainderStart;
        try
        {
            remainderStart = Util::strptime(rfcTime, "%Y-%m-%dT%H:%M:%S", &epochTime);
        }
        catch (const exception& e)
        {
            // Conversion failure.  
            return -1;
        }

        // Get the fraction of a second.  The remainder variable points to the next 
        // character after seconds.  If and only if there are fractional seconds 
        // (which Google Drive does use but which are optional per the RFC 3339 
        // specification),  this will be the '.' character.
        if (remainderStart != string::npos && rfcTime[remainderStart] == '.')
        {
            // Rather than getting the integer after the decimal and needing to 
            // count digits or count leading "0" characters, it's easier just to
            // get a floating point (or double) fraction between 0 and 1, then
            // multiply by 1000000000 to get nanoseconds.
            //char* start = remainder;
            pResultTime->tv_nsec = 
                    lround(1000000000L * Util::strtod(rfcTime, remainderStart));
        }
        else
        {
            // No fractional part.
            pResultTime->tv_nsec = 0;
        }

        // Get the timezone offset from UTC. Google Drive appears to use UTC (offset
        // is "Z"), but I don't know whether that's guaranteed. If not using UTC,
        // the offset will start with either '+' or '-'.
        if (remainderStart >= rfcTime.length())
        {
            // Invalid/no timezone offset
            return -1;
        }
        const char tzSep = rfcTime[remainderStart];
        if (tzSep != '+' && tzSep != '-' && toupper(tzSep) != 'Z')
        {
            // Invalid offset.
            return -1;
        }
        if (toupper(tzSep) != 'Z')
        {
            // Get the hour portion of the offset.
            size_t start = remainderStart;
            long offHour = Util::strtol(rfcTime, remainderStart, 10);
            if (remainderStart != start + 2 || rfcTime[remainderStart] != ':')
            {
                // Invalid offset, not in the form of "+HH:MM" / "-HH:MM"
                return -1;
            }

            // Get the minute portion of the offset
            start = remainderStart + 1;
            long offMinute = Util::strtol(rfcTime, remainderStart, 10);
            if (remainderStart != start + 2)
            {
                // Invalid offset, minute isn't a 2-digit number.
                return -1;
            }

            // Subtract the offset from the hour/minute parts of the tm struct.
            // This may give out-of-range values (e.g., tm_hour could be -2 or 26),
            // but mktime() is supposed to handle those.
            epochTime.tm_hour -= offHour;
            epochTime.tm_min -= offMinute;
        }

        // Convert the broken-down time into seconds.
        pResultTime->tv_sec = mktime(&epochTime);

        // Failure if mktime returned -1, success otherwise.
        if (pResultTime->tv_sec == (time_t) (-1))
        {
            return -1;
        }

        // Correct for local timezone, converting back to UTC
        // (Probably unnecessary to call tzset(), but it doesn't hurt)
        tzset();
        pResultTime->tv_sec -= timezone;
        return 0;
    }

    string Util::epochTimeNSToRfc3339(const struct timespec* ts)
    {
        // A max of 31 (or GDRIVE_TIMESTRING_LENGTH) should be the minimum that will
        // be successful.

        // If nanoseconds were greater than this number, they would be seconds.
        assert(ts->tv_nsec < 1000000000L);

        // Get everything down to whole seconds
        struct tm* pTime = gmtime(&(ts->tv_sec));
        stringstream ss;
        ss << Util::strftime("%Y-%m-%dT%H:%M:%S", pTime);
        
        // strftime() doesn't do fractional seconds. Add the '.', the fractional
        // part, and the 'Z' for timezone.
        ss << "." << setfill('0') << setw(9) << ts->tv_nsec << "Z";
//        int bytesWritten = baseLength;
//        bytesWritten += snprintf(dest + baseLength, max - baseLength, 
//                                 ".%09luZ", ts->tv_nsec);

        return ss.str();

    }

}