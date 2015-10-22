/* 
 * File:   NullStream.cpp
 * Author: me
 * 
 * Created on October 20, 2015, 10:22 PM
 */

#include "NullStream.hpp"

namespace fusedrive
{

    std::streamsize NullBuffer::xsputn(const char* s, std::streamsize n)
    {
        return n;
    }
    
    int NullBuffer::overflow(int c=EOF)
    {
        return c;
    }
    
    
    NullStream::NullStream()
    : std::ostream(&mNullBuffer)
    {
        // No body needed
    }


}