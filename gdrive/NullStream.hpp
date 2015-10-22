/* 
 * File:   NullStream.hpp
 * Author: me
 *
 * Created on October 20, 2015, 10:22 PM
 */

#ifndef NULLSTREAM_HPP
#define	NULLSTREAM_HPP

#include <iostream>

namespace fusedrive
{
    class NullBuffer : public std::streambuf
    {
    protected:
        virtual std::streamsize xsputn(const char* s, std::streamsize n);
        virtual int overflow(int c);
    };

    class NullStream : public std::ostream {
    public:
        NullStream();
        
    protected:
        NullBuffer mNullBuffer;
    };

}

#endif	/* NULLSTREAM_HPP */

