/* 
 * File:   Path.hpp
 * Author: me
 *
 * Created on October 24, 2015, 3:47 PM
 */

#ifndef PATH_HPP
#define	PATH_HPP

#include <string>

namespace fusedrive
{

    class Path {
    public:
        Path(const std::string& path);
        
        virtual ~Path();
        
        const std::string& getDirname() const;

        const std::string& getBasename() const;
        
    private:
        std::string mFullPath;
        mutable bool mHasDirString;
        mutable std::string mDirname;
        mutable bool mHasBaseString;
        mutable std::string mBasename;
        
        Path(const Path& orig);
        
    };

}

#endif	/* PATH_HPP */

