/* 
 * File:   CacheNode.cpp
 * Author: me
 * 
 * Created on October 19, 2015, 11:11 AM
 */

#include "CacheNode.hpp"
#include "Cache.hpp"
#include "Gdrive.hpp"
#include "gdrive-file.hpp"
#include "Util.hpp"

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>


using namespace std;
namespace fusedrive
{

    CacheNode::CacheNode() {
    }

    CacheNode::CacheNode(const CacheNode& orig) {
    }

    CacheNode::~CacheNode() {
    }

}
