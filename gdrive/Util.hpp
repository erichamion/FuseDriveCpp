/* 
 * File:   Util.hpp
 * Author: me
 *
 * Created on October 18, 2015, 3:32 PM
 */

#ifndef UTIL_HPP
#define	UTIL_HPP

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <string>


namespace fusedrive
{
    class Util {
    public:
        /*
         * gdrive_divide_round_up():    Divide a dividend by a divisor. If there is a
         *                              remainder, round UP to the next integer.
         * Parameters:
         *      dividend (long):    
         *              The dividend.
         *      divisor (long):     
         *              The divisor.
         * Return value (long):
         *      The smallest integer greater than or equal to the result of 
         *      dividend / divisor.
         */
        static long gdrive_divide_round_up(long dividend, long divisor);
        
        /*
         * gdrive_power_fopen():    Opens a file in a way similar to the fopen() system
         *                          call, but creates the parent directory (and the 
         *                          grandparent directory, and so on) if it doesn't
         *                          exist.
         * Parameters:
         *      path (const char*): 
         *              The path of the file to open.
         *      mode (const char*): 
         *              The mode parameter of the fopen() system call.
         * Return value:
         *      A valid FILE* handle on success, or NULL on failure.
         */
        static FILE* gdrive_power_fopen(const std::string& path, const std::string& mode);

        /*
         * gdrive_recursive_mkdir():    Creates a directory in a way similar to the 
         *                              mkdir() system call, but creates the parent 
         *                              directory (and the grandparent directory, and so
         *                              on) if it doesn't exist.
         * Parameters:
         *      path (const char*): 
         *              The path of the directory to create.
         * Return value:
         *      Return value (int):
         *      0 on success. On error, returns a negative value whose absolute value
         *      is defined in <errors.h>
         */
        static int gdrive_recursive_mkdir(const std::string& path);
        
        // Like strptime from <time.h>, but uses C++ string references. Returns the
        // index of the first unprocessed character, or string::npos if the entire
        // string was processed.
        static size_t strptime(const std::string& s, const std::string& format, struct tm* tm, bool initTm=false);
        
        // Like strtod, but uses C++ string reference.
        static double strtod(const std::string& nstr);
        
        // The supplied value of index is the index into the string at which to
        // start processing (to process from the beginning, set index to 0). On
        // exit, index will be the index into the entire original string of the 
        // first unprocessed character, or string::npos if the entire string 
        // was processed.
        static double strtod(const std::string& nstr, size_t& index);
        
        // Like strtol, but uses C++ string reference.
        static long strtol(const std::string& nstr, int base=0);
        
        // The supplied value of index is the index into the string at which to
        // start processing (to process from the beginning, set index to 0). On
        // exit, index will be the index into the entire original string of the 
        // first unprocessed character, or string::npos if the entire string 
        // was processed.
        static long strtol(const std::string& nstr, size_t& index, int base=0);
        
        static std::string strftime(const std::string& format, 
            const struct tm* tm);
        
        static int rfc3339ToEpochTimeNS(const std::string& rfcTime, 
            struct timespec* pResultTime);

        static std::string 
        epochTimeNSToRfc3339(const struct timespec* ts);
        
    private:
        Util();
        Util(const Util& orig);
        virtual ~Util();
    };
}
#endif	/* UTIL_HPP */

