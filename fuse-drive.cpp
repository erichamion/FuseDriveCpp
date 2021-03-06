/* 
 * File:   FuseDrive.cpp
 * Author: me
 *
 * Created on October 13, 2015, 11:10 PM
 */



// Library and standard headers
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <assert.h>


// Project header(s)
#include "fuse-drive.hpp"
#include "gdrive/Gdrive.hpp"
#include "Options.hpp"
#include "FuseDrivePrivateData.hpp"
#include "gdrive/GdriveFile.hpp"

using namespace std;
using namespace fusedrive;

/* 
 * File:   fuse-drive.c
 * Author: me
 *
 * 
 */

#ifndef __GDRIVE_TEST__






//namespace fusedrive
//{

    static int fudr_stat_from_fileinfo(Gdrive& gInfo, 
            const Fileinfo* pFileinfo, bool isRoot, struct stat* stbuf);

    static int fudr_rm_file_or_dir_by_id(Gdrive& gInfo, const string& fileId, 
            const string& parentId);

    static unsigned int fudr_get_max_perms(bool isDir);

    static bool fudr_group_match(gid_t gidToMatch, gid_t gid, uid_t uid);
    
    static int fudr_access(const string& path, int mask);
    
    static int fudr_access(const char* path, int mask);

    // static int fudr_bmap(const char* path, size_t blocksize, uint64_t* blockno);

    // static int fudr_chmod(const char* path, mode_t mode);

    // static int fudr_chown(const char* path, uid_t uid, gid_t gid);

    static int fudr_create(const char* path, mode_t mode, 
                           struct fuse_file_info* fi);

    static void fudr_destroy(void* private_data);

    /* static int fudr_fallocate(const char* path, int mode, off_t offset, 
     *                           off_t len, struct fuse_file_info* fi);
     */

    static int 
    fudr_fgetattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi);

    // static int fudr_flock(const char* path, struct fuse_file_info* fi, int op);

    // static int fudr_flush(const char* path, struct fuse_file_info* fi);

    static int fudr_fsync(const char* path, int isdatasync, 
                          struct fuse_file_info* fi);

    /* static int fudr_fsyncdir(const char* path, int isdatasync, 
     *                          struct fuse_file_info* fi);
     */

    static int fudr_ftruncate(const char* path, off_t size, 
                              struct fuse_file_info* fi);

    static int fudr_getattr(const char *path, struct stat *stbuf);

    /* static int fudr_getxattr(const char* path, const char* name, char* value, 
     *                          size_t size);
     */

    static void* fudr_init(struct fuse_conn_info *conn);

    /* static int fudr_ioctl(const char* path, int cmd, void* arg, 
     *                       struct fuse_file_info* fi, unsigned int flags, 
     *                       void* data);
     */

    static int fudr_link(const char* from, const char* to);

    // static int fudr_listxattr(const char* path, char* list, size_t size);

    /* static int fudr_lock(const char* path, struct fuse_file_info* fi, int cmd, 
     *                      struct flock* locks);
     */

    static int fudr_mkdir(const char* path, mode_t mode);

    // static int fudr_mknod(const char* path, mode_t mode, dev_t rdev);

    static int fudr_open(const char *path, struct fuse_file_info *fi);

    // static int fudr_opendir(const char* path, struct fuse_file_info* fi);

    /* static int fudr_poll(const char* path, struct fuse_file_info* fi, 
     *                      struct fuse_pollhandle* ph, unsigned* reventsp);
     */

    static int fudr_read(const char *path, char *buf, size_t size, off_t offset, 
                         struct fuse_file_info *fi);

    /* static int fudr_read_buf(const char* path, struct fuse_bufvec **bufp, 
     *                          size_t size, off_t off, struct fuse_file_info* fi);
     */

    static int fudr_readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
                            off_t offset, struct fuse_file_info *fi);

    // static int fudr_readlink(const char* path, char* buf, size_t size);

    static int fudr_release(const char* path, struct fuse_file_info *fi);

    // static int fudr_releasedir(const char* path, struct fuse_file_info *fi);

    // static int fudr_removexattr(const char* path, const char* value);

    static int fudr_rename(const char* from, const char* to);

    static int fudr_rmdir(const char* path);

    /* static int fudr_setxattr(const char* path, const char* name, 
     *                          const char* value, size_t size, int flags);
     */

    static int fudr_statfs(const char* path, struct statvfs* stbuf);

    // static int fudr_symlink(const char* to, const char* from);

    static int fudr_truncate(const char* path, off_t size);

    static int fudr_unlink(const char* path);

    // static int fudr_utime();

    static int fudr_utimens(const char* path, const struct timespec ts[2]);

    static int fudr_write(const char* path, const char *buf, size_t size, 
                          off_t offset, struct fuse_file_info* fi);

    /* static int fudr_write_buf(const char* path, struct fuse_bufvec* buf, 
     *                           off_t off, struct fuse_file_info* fi);
     */




    static int fudr_stat_from_fileinfo(Gdrive& gInfo, 
            const Fileinfo* pFileinfo, bool isRoot, struct stat* stbuf)
    {
        switch (pFileinfo->type)
        {
            case GDRIVE_FILETYPE_FOLDER:
                stbuf->st_mode = S_IFDIR;
                stbuf->st_nlink = pFileinfo->nParents + pFileinfo->nChildren;
                // Account for ".".  Also, if the root of the filesystem, account 
                // for  "..", which is outside of the Google Drive filesystem and 
                // thus not included in nParents.
                stbuf->st_nlink += isRoot ? 2 : 1;
                break;

            case GDRIVE_FILETYPE_FILE:
                // Fall through to default
                default:
                {
                    stbuf->st_mode = S_IFREG;
                    stbuf->st_nlink = pFileinfo->nParents;
                }
        }

        unsigned int perms = pFileinfo->getRealPermissions();
        unsigned int maxPerms = 
            fudr_get_max_perms(pFileinfo->type == GDRIVE_FILETYPE_FOLDER);
        // Owner permissions.
        stbuf->st_mode = stbuf->st_mode | ((perms << 6) & maxPerms);
        // Group permissions
        stbuf->st_mode = stbuf->st_mode | ((perms << 3) & maxPerms);
        // User permissions
        stbuf->st_mode = stbuf->st_mode | ((perms) & maxPerms);

        stbuf->st_uid = geteuid();
        stbuf->st_gid = getegid();
        stbuf->st_size = pFileinfo->size;
        stbuf->st_atime = pFileinfo->accessTime.tv_sec;
        stbuf->st_atim.tv_nsec = pFileinfo->accessTime.tv_nsec;
        stbuf->st_mtime = pFileinfo->modificationTime.tv_sec;
        stbuf->st_mtim.tv_nsec = pFileinfo->modificationTime.tv_nsec;
        stbuf->st_ctime = pFileinfo->creationTime.tv_sec;
        stbuf->st_ctim.tv_nsec = pFileinfo->creationTime.tv_nsec;

        return 0;
    }

    static int fudr_rm_file_or_dir_by_id(Gdrive& gInfo, const string& fileId, 
            const string& parentId)
    {
        // The fileId should never be empty. An empty parentId is a runtime 
        // error, but it shouldn't stop execution. Just check the fileId here.
        assert(!fileId.empty());
        

        // Find the number of parents, which is the number of "hard" links.
        const Fileinfo* pFileinfo;
        try
        {
            pFileinfo = &(gInfo.getFileinfoById(fileId));
        }
        catch (const exception& e)
        {
            // Error
            return -ENOENT;
        }
        
        if (pFileinfo->nParents > 1)
        {
            // Multiple "hard" links, just remove the parent
            if (parentId.empty())
            {
                // Invalid ID for parent folder
                return -ENOENT;
            }
            return gInfo.removeParent(fileId, parentId);
        }
        // else this is the only hard link. Delete or trash the file.

        return gInfo.deleteFile(fileId, parentId);
    }

    static unsigned int fudr_get_max_perms(bool isDir)
    {
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        unsigned long perms = pPrivateData->getPerms();
        if (isDir)
        {
            perms >>= 9;
        }
        else
        {
            perms &= 0777;
        }

        return perms & ~context->umask;
    }

    static bool fudr_group_match(gid_t gidToMatch, gid_t gid, uid_t uid)
    {
        // Simplest case - primary group matches
        if (gid == gidToMatch)
        {
            return true;
        }

        // Get a list of all the users in the group, and see whether the desired
        // user is in the list. It seems like there MUST be a cleaner way to do
        // this!
        const struct passwd* userInfo = getpwuid(uid);
        const struct group* grpInfo = getgrgid(gidToMatch);
        for (char** pName = grpInfo->gr_mem; *pName; pName++)
        {
            if (!strcmp(*pName, userInfo->pw_name))
            {
                return true;
            }
        }

        // No match found
        return false;
    }


    static int fudr_access(const string& path, int mask)
    {
        // If fudr_chmod() or fudr_chown() is ever added, this function will likely 
        // need changes.
        
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();
        
        string fileId = gInfo.getFileIdFromPath(path);
        if (fileId.empty())
        {
            // File doesn't exist
            return -ENOENT;
        }
        const Fileinfo* pFileinfo;
        try
        {
            pFileinfo = &(gInfo.getFileinfoById(fileId));
        }
        catch (const exception& e)
        {
            // Unknown error
            return -EIO;
        }

        if (mask == F_OK)
        {
            // Only checking whether the file exists
            return 0;
        }

        unsigned int filePerms = pFileinfo->getRealPermissions();
        unsigned int maxPerms = 
            fudr_get_max_perms(pFileinfo->type == GDRIVE_FILETYPE_FOLDER);


        if (context->uid == geteuid())
        {
            // User permission
            maxPerms >>= 6;
        }
        else if (fudr_group_match(getegid(), context->gid, context->uid))
        {
            // Group permission
            maxPerms >>= 3;
        }
        // else other permission, don't change maxPerms

        unsigned int finalPerms = filePerms & maxPerms;

        if (((mask & R_OK) && !(finalPerms & S_IROTH)) || 
                ((mask & W_OK) && !(finalPerms & S_IWOTH)) || 
                ((mask & X_OK) && !(finalPerms & S_IXOTH))
                )
        {
            return -EACCES;
        }

        return 0;
    }

    static int fudr_access(const char* path, int mask)
    {
        return fudr_access((const string) path, mask);
    }
    
    /* static int fudr_bmap(const char* path, size_t blocksize, uint64_t* blockno)
     * {
     *     
     * }
     */

    /* static int fudr_chmod(const char* path, mode_t mode)
     * {
     *     // If this is implemented at all, will need to store new permissions as
     *     // private file metadata.  Should check the owners[] list in file 
     *     // metadata, return -EPERM unless IsAuthenticatedUser is true for one of 
     *     // the owners.
     *     return -ENOSYS;
     * }
     */

    /* static int fudr_chown(const char* path, uid_t uid, gid_t gid)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_create(const char* path, mode_t mode, 
            struct fuse_file_info* fi)
    {
        // Silence compiler warning for unused parameter. If fudr_chmod is 
        // implemented, this line should be removed.
        (void) mode;
        
        const string pathStr(path);
        
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();

        // Determine whether the file already exists
        if (!gInfo.getFileIdFromPath(path).empty())
        {
            return -EEXIST;
        }

        // Need write access to the parent directory
        Path gpath(path);
        int accessResult = fudr_access(gpath.getDirname(), W_OK);
        if (accessResult)
        {
            // Access check failed
            return accessResult;
        }

        // Create the file
        int error = 0;
        const string fileId = gInfo.createFile(pathStr, false, error);
        if (fileId.empty())
        {
            // Some error occurred
            return -error;
        }

        // TODO: If fudr_chmod is ever implemented, change the file permissions 
        // using the (currently unused) mode parameter we were given.

        // File was successfully created. Open it.
        fi->fh = (uint64_t) gInfo.openFile(fileId, O_RDWR, error);

        return -error;
    }

    static void fudr_destroy(void* private_data)
    {
        // Silence compiler warning about unused parameter
        (void) private_data;

        //gdrive_cleanup();
    }

    /* static int fudr_fallocate(const char* path, int mode, off_t offset, 
     *                           off_t len, struct fuse_file_info* fi)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_fgetattr(const char* path, struct stat* stbuf, 
                             struct fuse_file_info* fi)
    {
        GdriveFile* fh = (GdriveFile*) fi->fh;
        const Fileinfo* pFileinfo = (fi->fh == (uint64_t) NULL) ? 
            NULL : &fh->getFileinfo();

        if (pFileinfo == NULL)
        {
            // Invalid file handle
            return -EBADF;
        }
        
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();
        
        return fudr_stat_from_fileinfo(gInfo, pFileinfo, strcmp(path, "/") == 0, stbuf);
    }

    /* static int fudr_flock(const char* path, struct fuse_file_info* fi, int op)
     * {
     *     
     * }
     */

    /* static int fudr_flush(const char* path, struct fuse_file_info* fi)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_fsync(const char* path, int isdatasync, 
                          struct fuse_file_info* fi)
    {
        // Distinguishing between data-only and data-and-metadata syncs doesn't
        // really help us, so ignore isdatasync. Ignore path since we should have
        // a Gdrive file handle.
        (void) isdatasync;
        (void) path;

        if (fi->fh == (uint64_t) NULL)
        {
            // Bad file handle
            return -EBADF;
        }
        GdriveFile* fh = (GdriveFile*) fi->fh;
        return fh->sync();
    }

    /* static int fudr_fsyncdir(const char* path, int isdatasync, 
     *                          struct fuse_file_info* fi)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_ftruncate(const char* path, off_t size, 
                              struct fuse_file_info* fi)
    {
        // Suppress unused parameter compiler warnings
        (void) path;

        GdriveFile* fh = (GdriveFile*) fi->fh;
        if (fh == NULL)
        {
            // Invalid file handle
            return -EBADF;
        }

        // Need write access to the file
        int accessResult = fudr_access(path, W_OK);
        if (accessResult)
        {
            return accessResult;
        }

        return fh->truncate(size);
    }

    static int fudr_getattr(const char *path, struct stat *stbuf)
    {
        memset(stbuf, 0, sizeof(struct stat));
        
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();
        
        string fileId = gInfo.getFileIdFromPath(path);
        if (fileId.empty())
        {
            // File not found
            return -ENOENT;
        }

        const Fileinfo* pFileinfo;
        try
        {
            pFileinfo = &(gInfo.getFileinfoById(fileId));
        }
        catch (const exception& e)
        {
            // An error occurred.
            return -ENOENT;
        }    

        return fudr_stat_from_fileinfo(gInfo, pFileinfo, strcmp(path, "/") == 0, stbuf);
    }

    /* static int fudr_getxattr(const char* path, const char* name, char* value, 
     *                          size_t size)
     * {
     *     return -ENOSYS;
     * }
     */

    static void* fudr_init(struct fuse_conn_info *conn)
    {
        // Add any desired capabilities.
        conn->want = conn->want | 
                FUSE_CAP_ATOMIC_O_TRUNC | FUSE_CAP_BIG_WRITES | 
                FUSE_CAP_EXPORT_SUPPORT;
        // Remove undesired capabilities.
        conn->want = conn->want & !(FUSE_CAP_ASYNC_READ);

        // Need to turn off async read here, too.
        conn->async_read = 0;

        return fuse_get_context()->private_data;
    }

    /* static int fudr_ioctl(const char* path, int cmd, void* arg, 
     *                       struct fuse_file_info* fi, unsigned int flags, 
     *                       void* data)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_link(const char* from, const char* to)
    {
        Path oldPath(from);
        Path newPath(to);
        
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();

        // Determine whether the file already exists
        if (!gInfo.getFileIdFromPath(to).empty())
        {
            return -EEXIST;
        }

        // Google Drive supports a file with multiple parents - that is, a file with
        // multiple hard links that all have the same base name.
        if (oldPath.getBasename() != newPath.getBasename())
        {
            // Basenames differ, not supported
            return -ENOENT;
        }

        // Need write access in the target directory
        int accessResult = fudr_access(newPath.getDirname(), W_OK);
        if (accessResult)
        {
            return accessResult;
        }

        const string fileId = gInfo.getFileIdFromPath(from);
        if (fileId.empty())
        {
            // Original file does not exist
            return -ENOENT;
        }
        const string newParentId = 
        gInfo.getFileIdFromPath(newPath.getDirname());
        if (newParentId.empty())
        {
            // New directory doesn't exist
            return -ENOENT;
        }

        int returnVal = gInfo.addParent(fileId, newParentId);

        return returnVal;
    }

    /* static int fudr_listxattr(const char* path, char* list, size_t size)
     * {
     *     return -ENOSYS;
     * }
     */

    /* static int fudr_lock(const char* path, struct fuse_file_info* fi, int cmd, 
     *                      struct flock* locks)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_mkdir(const char* path, mode_t mode)
    {
        // Silence compiler warning for unused variable. If and when chmod is 
        // implemented, this should be removed.
        (void) mode;
        
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();

        // Determine whether the folder already exists, 
        string dummyFileId = gInfo.getFileIdFromPath(path);
        if (!dummyFileId.empty())
        {
            return -EEXIST;
        }

        // Need write access to the parent directory
        Path gpath(path);
        int accessResult = fudr_access(gpath.getDirname(), W_OK);
        if (accessResult)
        {
            return accessResult;
        }

        // Create the folder
        int error = 0;
        gInfo.createFile(path, true, error);

        // TODO: If fudr_chmod is ever implemented, change the folder permissions 
        // using the (currently unused) mode parameter we were given.

        return -error;
    }

    /* static int fudr_mknod(const char* path, mode_t mode, dev_t rdev)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_open(const char *path, struct fuse_file_info *fi)
    {
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();
        
        // Get the file ID
        string fileId = gInfo.getFileIdFromPath(path);
        if (fileId.empty())
        {
            // File not found
            return -ENOENT;
        }

        // Confirm permissions
        unsigned int modeNeeded = 0;
        if (fi->flags & (O_RDONLY | O_RDWR))
        {
            modeNeeded |= R_OK;
        }
        if (fi->flags & (O_WRONLY | O_RDWR))
        {
            modeNeeded |= W_OK;
        }
        if (!modeNeeded)
        {
            modeNeeded = F_OK;
        }
        int accessResult = fudr_access(path, modeNeeded);
        if (accessResult)
        {
            return accessResult;
        }

        // Open the file
        int error = 0;
        GdriveFile* pFile = gInfo.openFile(fileId, fi->flags, error);

        if (pFile == NULL)
        {
            // An error occurred.
            return -error;
        }

        // Store the file handle
        fi->fh = (uint64_t) pFile;
        return 0;
    }

    /* static int fudr_opendir(const char* path, struct fuse_file_info* fi)
     * {
     *     return -ENOSYS;
     * }
     */

    /* static int fudr_poll(const char* path, struct fuse_file_info* fi, 
     *                      struct fuse_pollhandle* ph, unsigned* reventsp)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_read(const char *path, char *buf, size_t size, off_t offset, 
                         struct fuse_file_info *fi)
    {
        // Silence compiler warning about unused parameter
        (void) path;

        // Check for read access
        int accessResult = fudr_access(path, R_OK);
        if (accessResult)
        {
            return accessResult;
        }

        GdriveFile* pFile = (GdriveFile*) fi->fh;
        if (!pFile)
        {
            return -EBADF;
        }

        return pFile->read(buf, size, offset);
    }

    /* static int 
     * fudr_read_buf(const char* path, struct fuse_bufvec **bufp, 
     *               size_t size, off_t off, struct fuse_file_info* fi)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
                            off_t offset, struct fuse_file_info *fi)
    {
        // Suppress warnings for unused function parameters
        (void) offset;
        (void) fi;
        
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();
        
        string folderId = gInfo.getFileIdFromPath(path);
        if (folderId.empty())
        {
            return -ENOENT;
        }

        // Check for read access
        int accessResult = fudr_access(path, R_OK);
        if (accessResult)
        {
            return accessResult;
        }

        FileinfoArray fileArray(gInfo);
        if (gInfo.ListFolderContents(folderId, fileArray) < 0)
        {
            // An error occurred.
            return -ENOENT;
        }

        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        const Fileinfo* pCurrentFile;
        for (pCurrentFile = fileArray.first(); 
                pCurrentFile != NULL; 
                pCurrentFile = fileArray.next()
                )
        {
            struct stat st = {0};
            switch (pCurrentFile->type)
            {
                case GDRIVE_FILETYPE_FILE:
                    st.st_mode = S_IFREG;
                    break;

                case GDRIVE_FILETYPE_FOLDER:
                    st.st_mode = S_IFDIR;
                    break;
            }
            filler(buf, pCurrentFile->filename.c_str(), &st, 0);
        }


        return 0;
    }

    /* static int fudr_readlink(const char* path, char* buf, size_t size)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_release(const char* path, struct fuse_file_info* fi)
    {
        // Suppress unused parameter warning
        (void) path;

        GdriveFile* fh = (GdriveFile*) fi->fh;
        if (!fh)
        {
            // Bad file handle
            return -EBADF;
        }

        fh->close(fi->flags);
        return 0;
    }

    /* static int fudr_releasedir(const char* path, struct fuse_file_info *fi)
     * {
     *     return -ENOSYS;
     * }
     */

    /* static int fudr_removexattr(const char* path, const char* value)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_rename(const char* from, const char* to)
    {
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();
        
        // Neither from nor to should be the root directory
        const string& rootId = gInfo.sysinfo().rootId();
        if (rootId.empty())
        {
            // Error
            return -EIO;
        }
        
        string fromFileId = gInfo.getFileIdFromPath(from);
        if (fromFileId.empty())
        {
            // from doesn't exist
            return -ENOENT;
        }
        if (fromFileId == rootId)
        {
            // from is root
            return -EBUSY;
        }
        string toFileId = gInfo.getFileIdFromPath(to);
        // Empty toFileId is not an error.

        // Special handling if the destination exists
        if (!toFileId.empty())
        {
            if (toFileId == rootId)
            {
                // to is root
                return -EBUSY;
            }

            // If from and to are hard links to the same file, do nothing and 
            // return success.
            if (fromFileId == toFileId)
            {
                return 0;
            }

            // If the source is a directory, destination must be an empty directory
            const Fileinfo* pFromInfo;
            try
            {
                pFromInfo = &(gInfo.getFileinfoById(fromFileId));
            }
            catch (const exception& e)
            {
                // Unknown error
                return -EIO;
            }
            if (pFromInfo->type == GDRIVE_FILETYPE_FOLDER)
            {
                bool toExists = true;
                const Fileinfo* pToInfo = NULL;
                try
                {
                    const Fileinfo& toInfo = gInfo.getFileinfoById(toFileId);
                    pToInfo = &toInfo;
                }
                catch (...)
                {
                    toExists = false;
                }
                if (toExists && pToInfo->type != GDRIVE_FILETYPE_FOLDER)
                {
                    // Destination is not a directory
                    return -ENOTDIR;
                }
                if (toExists && pToInfo->nChildren > 0)
                {
                    // Destination is not empty
                    return -ENOTEMPTY;
                }
            }

            // Need write access for the destination
            int accessResult = fudr_access(to, W_OK);
            if (accessResult)
            {
                return -EACCES;
            }
        }
        

        Path fromPath(from);
        Path toPath(to);
        
        
        string fromParentId = 
                gInfo.getFileIdFromPath(fromPath.getDirname());
        if (fromParentId.empty())
        {
            // from path doesn't exist
            return -ENOENT;
        }
        string toParentId = 
                gInfo.getFileIdFromPath(toPath.getDirname());
        if (toParentId.empty())
        {
            // from path doesn't exist
            return -ENOENT;
        }

        // Need write access in the destination parent directory
        int accessResult = fudr_access(toPath.getDirname(), W_OK);
        if (accessResult)
        {
            return accessResult;
        }

        // If the directories are different, create a new hard link and delete
        // the original. Compare the actual file IDs of the parents, not the paths,
        // because different paths could refer to the same directory.
        if (fromParentId != toParentId)
        {
            int result = gInfo.addParent(fromFileId, toParentId);
            if (result != 0)
            {
                // An error occurred
                return result;
            }
            result = fudr_unlink(from);
            if (result != 0)
            {
                // An error occurred
                return result;
            }
        }

        int returnVal = 0;

        // If the basenames are different, change the basename. NOTE: If there are
        // any other hard links to the file, this will also change their names.
        const string& fromBasename = fromPath.getBasename();
        const string& toBasename = toPath.getBasename();
        if (fromBasename != toBasename)
        {
            returnVal = gInfo.changeBasename(fromFileId, toBasename);
        }

        // If successful, and if to already existed, delete it
        if (!toFileId.empty() && !returnVal)
        {
            returnVal = fudr_rm_file_or_dir_by_id(gInfo, toFileId, toParentId);
        }


        return returnVal;
    }

    static int fudr_rmdir(const char* path)
    {
        // Can't delete the root directory
        if (strcmp(path, "/") == 0)
        {
            return -EBUSY;
        }
        
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();

        const string fileId = gInfo.getFileIdFromPath(path);
        if (fileId.empty())
        {
            // No such file
            return -ENOENT;
        }

        // Make sure path refers to an empty directory
        const Fileinfo* pFileinfo;
        try
        {
            pFileinfo = &(gInfo.getFileinfoById(fileId));
        }
        catch (const exception& e)
        {
            // Couldn't retrieve file info
            return -ENOENT;
        }
        if (pFileinfo->type != GDRIVE_FILETYPE_FOLDER)
        {
            // Not a directory
            return -ENOTDIR;
        }
        if (pFileinfo->nChildren > 0)
        {
            // Not empty
            return -ENOTEMPTY;
        }

        // Need write access
        int accessResult = fudr_access(path, W_OK);
        if (accessResult)
        {
            return accessResult;
        }

        // Get the parent ID
        Path gpath(path);
        string parentId = gInfo.getFileIdFromPath(gpath.getDirname());
        
        int returnVal = fudr_rm_file_or_dir_by_id(gInfo, fileId, parentId);
        return returnVal;
    }

    /* static int fudr_setxattr(const char* path, const char* name, 
     *                          const char* value, size_t size, int flags)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_statfs(const char* path, struct statvfs* stbuf)
    {
        // Suppress compiler warning about unused parameter
        (void) path;
        
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();

        unsigned long blockSize = gInfo.getMinChunkSize();
        unsigned long bytesTotal = gInfo.sysinfo().size();
        unsigned long bytesFree = bytesTotal - gInfo.sysinfo().used();

        memset(stbuf, 0, sizeof(struct statvfs));
        stbuf->f_bsize = blockSize;
        stbuf->f_blocks = bytesTotal / blockSize;
        stbuf->f_bfree = bytesFree / blockSize;
        stbuf->f_bavail = stbuf->f_bfree;

        return 0;
    }

    /* static int fudr_symlink(const char* to, const char* from)
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_truncate(const char* path, off_t size)
    {
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();
        
        const string fileId = gInfo.getFileIdFromPath(path);
        if (fileId.empty())
        {
            // File not found
            return -ENOENT;
        }

        // Need write access
        int accessResult = fudr_access(path, W_OK);
        if (accessResult)
        {
            return accessResult;
        }

        // Open the file
        int error = 0;
        GdriveFile* fh = gInfo.openFile(fileId, O_RDWR, error);
        if (fh == NULL)
        {
            // Error
            return error;
        }

        // Truncate
        int result = fh->truncate(size);

        // Close
        fh->close(O_RDWR);

        return result;
    }

    static int fudr_unlink(const char* path)
    {
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();
        
        string fileId = gInfo.getFileIdFromPath(path);
        if (fileId.empty())
        {
            // No such file
            return -ENOENT;
        }

        // Need write access
        int accessResult = fudr_access(path, W_OK);
        if (accessResult)
        {
            return accessResult;
        }

        Path gpath(path);
        string parentId = gInfo.getFileIdFromPath(gpath.getDirname());
        
        int returnVal = fudr_rm_file_or_dir_by_id(gInfo, fileId, parentId);
        return returnVal;
    }

    /* static int fudr_utime()
     * {
     *     return -ENOSYS;
     * }
     */

    static int fudr_utimens(const char* path, const struct timespec ts[2])
    {
        struct fuse_context* context = fuse_get_context();
        FuseDrivePrivateData* pPrivateData = 
                (FuseDrivePrivateData*) context->private_data;
        Gdrive& gInfo = pPrivateData->getGdrive();
        
        const string fileId = gInfo.getFileIdFromPath(path);
        if (fileId.empty())
        {
            return -ENOENT;
        }

        int error = 0;
        GdriveFile* fh = gInfo.openFile(fileId, O_RDWR, error);
        if (fh == NULL)
        {
            return -error;
        }
        
        if (ts[0].tv_nsec == UTIME_NOW)
        {
            error = fh->setAtime(NULL);
        }
        else if (ts[0].tv_nsec != UTIME_OMIT)
        {
            error = fh->setAtime(&(ts[0]));
        }

        if (error != 0)
        {
            fh->close(O_RDWR);
            return error;
        }

        if (ts[1].tv_nsec == UTIME_NOW)
        {
            fh->setMtime(NULL);
        }
        else if (ts[1].tv_nsec != UTIME_OMIT)
        {
            fh->setMtime(&(ts[1]));
        }

        fh->close(O_RDWR);
        return error;
    }

    static int fudr_write(const char* path, const char *buf, size_t size, 
                          off_t offset, struct fuse_file_info* fi)
    {
        // Avoid compiler warning for unused variable
        (void) path;

        // Check for write access
        int accessResult = fudr_access(path, W_OK);
        if (accessResult)
        {
            return accessResult;
        }

        GdriveFile* fh = (GdriveFile*) fi->fh;
        if (fh == NULL)
        {
            // Bad file handle
            return -EBADFD;
        }
        
        return fh->write(buf, size, offset);
    }

    /* static int fudr_write_buf(const char* path, struct fuse_bufvec* buf, 
     *                           off_t off, struct fuse_file_info* fi)
     * {
     *     return -ENOSYS;
     * }
     */


    static struct fuse_operations fo = {};

    static void init_fo() {
        fo.access         = fudr_access;
        // bmap is not needed
        fo.bmap           = NULL;
        // Might consider chmod later (somewhat unlikely)
        fo.chmod          = NULL;
        // Might consider chown later (unlikely)
        fo.chown          = NULL;
        fo.create         = fudr_create;
        fo.destroy        = fudr_destroy;
        // fallocate is not needed
        fo.fallocate      = NULL;
        fo.fgetattr       = fudr_fgetattr;
        // flock is not needed
        fo.flock          = NULL;
        // flush is not needed
        fo.flush          = NULL;
        fo.fsync          = fudr_fsync;
        // fsyncdir is not needed
        fo.fsyncdir       = NULL;
        fo.ftruncate      = fudr_ftruncate;
        fo.getattr        = fudr_getattr;
        // getxattr is not needed
        fo.getxattr       = NULL;
        fo.init           = fudr_init;
        // ioctl is not needed
        fo.ioctl          = NULL;
        fo.link           = fudr_link;
        // listxattr is not needed
        fo.listxattr      = NULL;
        // lock is not needed
        fo.lock           = NULL;
        fo.mkdir          = fudr_mkdir;
        // mknod is not needed
        fo.mknod          = NULL;
        fo.open           = fudr_open;
        // opendir is not needed
        fo.opendir        = NULL;
        // poll is not needed
        fo.poll           = NULL;
        fo.read           = fudr_read;
        // Not sure how to use read_buf or whether it would help us
        fo.read_buf       = NULL;
        fo.readdir        = fudr_readdir;
        // Might consider later whether readlink and symlink can/should be added
        fo.readlink       = NULL;
        fo.release        = fudr_release;
        // releasedir is not needed
        fo.releasedir     = NULL;
        // removexattr is not needed
        fo.removexattr    = NULL;
        fo.rename         = fudr_rename;
        fo.rmdir          = fudr_rmdir;
        // setxattr is not needed
        fo.setxattr       = NULL;
        fo.statfs         = fudr_statfs;
        // Might consider later whether symlink and readlink can/should be added
        fo.symlink        = NULL;
        fo.truncate       = fudr_truncate;
        fo.unlink         = fudr_unlink;
        // utime isn't needed
        fo.utime          = NULL;
        fo.utimens        = fudr_utimens;
        fo.write          = fudr_write;
        // Not sure how to use write_buf or whether it would help us
        fo.write_buf      = NULL;
    }


    /*
     * 
     */
    int main(int argc, char* argv[])
    {
        init_fo();

        // Parse command line options
        Options* pOptions;
        try
        {
            pOptions = new Options(argc, argv);
        }
        catch (const exception& e)
        {
            fputs("Error interpreting command line options:\n\t", stderr);
            if (e.what())
            {
                fputs(e.what(), stderr);
            }
            else
            {
                fputs("Unknown error", stderr);
            }
            return 1;
        }
        
        FuseDrivePrivateData* pPrivateData;
        try
        {
            pPrivateData = new FuseDrivePrivateData(*pOptions);
        }
        catch (const exception& e)
        {
            delete pOptions;
            fputs("Could not set up a Google Drive connection.\n", stderr);
            return 1;
        }
        
        
        int returnVal;
        returnVal = fuse_main(pOptions->fuse_argc, pOptions->fuse_argv, &fo, 
                              (void*) pPrivateData);
        
        delete pOptions;
        delete pPrivateData;
        return returnVal;
    }

//}

#endif	/*__GDRIVE_TEST__*/

