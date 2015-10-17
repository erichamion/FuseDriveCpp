/* 
 * File:   FuseDriveOptions.cpp
 * Author: me
 * 
 * Created on October 13, 2015, 11:26 PM
 */

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <assert.h>

#include "Options.hpp"

using namespace std;

namespace fusedrive
{
    /**************************
    * Private Constants
    **************************/
    const char Options::OPTION_ACCESS = 'a';
    const char Options::OPTION_CONFIG = 'c';
    const char Options::OPTION_INTERACTION = 'i';
    const char Options::OPTION_FILEPERM = 'p';
    const char Options::OPTION_DIRPERM = 'd';
    const unsigned int Options::OPTION_CACHETTL = 500;
    const unsigned int Options::OPTION_CHUNKSIZE = 501;
    const unsigned int Options::OPTION_MAXCHUNKS = 502;
    const char* const Options::OPTION_STRING = "+a:c:i:p:d:";

    const unsigned int Options::DEFAULT_GDRIVE_ACCESS = Gdrive::GDRIVE_ACCESS_WRITE;
    const char* const Options::DEFAULT_AUTH_BASENAME = ".auth";
    const char* const Options::DEFAULT_AUTH_RELPATH = "fuse-drive";
    const unsigned int Options::DEFAULT_CACHETTL = 30;
    const Gdrive_Interaction Options::DEFAULT_INTERACTION = GDRIVE_INTERACTION_STARTUP;
    const unsigned int Options::DEFAULT_CHUNKSIZE = Gdrive::GDRIVE_BASE_CHUNK_SIZE * 4;
    const unsigned int Options::DEFAULT_MAXCHUNKS = 15;
    const unsigned int Options::DEFAULT_FILEPERMS = 0644;
    const unsigned int Options::DEFAULT_DIRPERMS = 07777;
    
    
    /**************************
    * Public Methods
    **************************/
    
    Options::Options(int argc, char** argv) 
    : gdrive_auth_file()
    {
        // Initialize all options to their default values
        fudr_options_default();
        
        // Set up the long options
        struct option longopts[] = 
        {
            {
                .name = "access",
                .has_arg = required_argument,
                .flag = NULL,
                .val = OPTION_ACCESS
            },
            {
                .name = "config",
                .has_arg = required_argument,
                .flag = NULL,
                .val = OPTION_CONFIG
            },
            {
                .name = "cache-time",
                .has_arg = required_argument,
                .flag = NULL,
                .val = OPTION_CACHETTL
            },
            {
                .name = "interaction",
                .has_arg = required_argument,
                .flag = NULL,
                .val = OPTION_INTERACTION
            },
            {
                .name = "chunk-size",
                .has_arg = required_argument,
                .flag = NULL,
                .val = OPTION_CHUNKSIZE
            },
            {
                .name = "max-chunks",
                .has_arg = required_argument,
                .flag = NULL,
                .val = OPTION_MAXCHUNKS
            },
            {
                .name = "file-perm",
                .has_arg = required_argument,
                .flag = NULL,
                .val = OPTION_FILEPERM
            },
            {
                .name = "dir-perm",
                .has_arg = required_argument,
                .flag = NULL,
                .val = OPTION_DIRPERM
            },
            {
                // End the array with an 
                // all-zero element
                0
            }
        };

        opterr = 0;
        int currentArg;
        int longoptIndex = -1;
        while ((currentArg = getopt_long(argc, argv, OPTION_STRING, 
                                         longopts, &longoptIndex)) != -1)
        {
            switch (currentArg)
            {
                case OPTION_ACCESS:
                    // Set Google Drive access level
                    fudr_options_set_access(optarg);
                    break;
                case OPTION_CONFIG:
                    // Set config file (auth file)
                    fudr_options_set_config(optarg);
                    break;
                case OPTION_INTERACTION:
                    // Set the interaction type
                    fudr_options_set_interaction(optarg);
                    break;
                case OPTION_FILEPERM:
                    // Set the file permissions
                    fudr_options_set_fileperm(optarg);
                    break;
                case OPTION_DIRPERM:
                    // Set directory permissions
                    fudr_options_set_dirperm(optarg);
                    break;
                case OPTION_CACHETTL:
                    // Set cache TTL
                    fudr_options_set_cachettl(optarg);
                    break;
                case OPTION_CHUNKSIZE:
                    // Set chunk size
                    fudr_options_set_chunksize(optarg);
                    break;
                case OPTION_MAXCHUNKS:
                    // Set max chunks
                    fudr_options_set_maxchunks(optarg);
                    break;
                case '?': 
                    // Fall through to default
                    default:
                        fudr_options_set_failed(optopt, longopts, 
                                longoptIndex);
            }
            
        }

        if (dir_perms == DEFAULT_DIRPERMS)
        {
            // Default directory permissions start by coping the file 
            // permissions, but anybody who has read permission also gets
            // execute permission.
            dir_perms = file_perms;
            unsigned int read_perms = dir_perms & 0444;
            dir_perms |= (read_perms >> 2);
        }



        // Pass on non-option argument and any following arguments to FUSE, but
        // make a copy first. We need to add an argv[0], and we'll be adding 
        // some extra options to the end, so we can't just use the passed-in 
        // array.
        if (optind < argc && !strcmp(argv[optind], "--"))
        {
            // If a "--" end-of-arguments is found, skip past it
            optind++;
        }
        fuse_argc = argc - optind + 1;
        try
        {
            fuse_argv = new char*[fuse_argc + 2];
        }
        catch (const bad_alloc& e)
        {
            // Memory error
            cleanup();
            throw e;
        }
        fuse_argv[0] = argv[0];
        for (int i = 1; i < fuse_argc; i++)
        {
            fuse_argv[i] = argv[i + optind - 1];
        }



        // If we might need to interact with the user, need to add the
        // foreground option. The foreground option also changes other behavior
        // (specifically, the working directory is different). Since we need the
        // option sometimes, always add it to be consistent.
        fuse_argv[fuse_argc++] = (char*) "-f";

        // Enforce single-threaded mode
        fuse_argv[fuse_argc++] = (char*) "-s";
        
    }

    Options::Options(const Options& orig) {
    }

    Options::~Options() 
    {
        cleanup();
    }
    
    
    /**
     * Gets the default path for the config/auth file.
     * @return (char*): A string containing "<HOME-DIRECTORY>/"
     *                  "<FUDR_DEFAULT_AUTH_RELPATH>/<FUDR_DEFAULT_AUTH_BASENAME>"
     *                  on success, or NULL on error. The returned memory must be 
     *                  freed by the caller.
     */
    void Options::fudr_options_get_default_auth_file()
    {
        // Get the user's home directory
        const char* homedir = getenv("HOME");
        if (!homedir)
        {
            homedir = getpwuid(getuid())->pw_dir;
        }

        // Append the relative path and basename to the home directory
        
        gdrive_auth_file.append(homedir);
        gdrive_auth_file.append("/");
        gdrive_auth_file.append(DEFAULT_AUTH_RELPATH);
        gdrive_auth_file.append("/");
        gdrive_auth_file.append(DEFAULT_AUTH_BASENAME);
        
    }

    
    /**************************
    * Private Instance Methods
    **************************/
    
    /**
     * Allocates and fills in an error message with one string parameter.
     * @param fmtStr (const char*): Format string as used for printf(). Must contain
     *                              exactly one "%s"
     * @param arg (const char*):    String that will be substituted into fmtStr.
     */
    void Options::fudr_options_make_errormsg(char** pDest, const char* fmtStr, 
            const char* arg)
    {
        // Nothing should be NULL, and fmtStr should not be "".
        assert(pDest && fmtStr && fmtStr[0] && arg);

        int length = snprintf(NULL, 0, fmtStr, arg);
        
        *pDest = new char[length + 1];
        
        snprintf(*pDest, length + 1, fmtStr, arg);
        
    }
    
    void Options::fudr_options_default()
    {
        gdrive_access = DEFAULT_GDRIVE_ACCESS;
        fudr_options_get_default_auth_file();
        gdrive_cachettl = DEFAULT_CACHETTL;
        gdrive_interaction_type = DEFAULT_INTERACTION;
        gdrive_chunk_size = DEFAULT_CHUNKSIZE;
        gdrive_max_chunks = DEFAULT_MAXCHUNKS;
        file_perms = DEFAULT_FILEPERMS;
        dir_perms = DEFAULT_DIRPERMS;
        fuse_argv = NULL;
        fuse_argc = 0;
        error = false;
        // errorMsg = NULL;
    }

    /**
     * Set the Google Drive access level
     * @param pOptions
     * @param arg
     * @return false on success, true on error
     */
    void Options::fudr_options_set_access(const char* arg)
    {
        // Nothing should be NULL
        assert(arg);

        if (!strcmp(arg, "meta"))
        {
            gdrive_access = Gdrive::GDRIVE_ACCESS_META;
        }
        else if (!strcmp(arg, "read"))
        {
            gdrive_access = Gdrive::GDRIVE_ACCESS_READ;
        }
        else if (!strcmp(arg, "write"))
        {
            gdrive_access = Gdrive::GDRIVE_ACCESS_WRITE;
        }
        else if (!strcmp(arg, "apps"))
        {
            gdrive_access = Gdrive::GDRIVE_ACCESS_APPS;
        }
        else if (!strcmp(arg, "all"))
        {
            gdrive_access = Gdrive::GDRIVE_ACCESS_ALL;
        }
        else
        {
            const char* fmtStr = "Unrecognized access level '%s'. Valid values are "
                                 "meta, read, write, apps, or all.\n";
            throw new BadOptionException(fmtStr, arg);
        }
    }

    /**
     * Set config file (auth file)
     * @param pOptions
     * @param arg
     * @return false on success, true on error
     */
    void Options::fudr_options_set_config(const char* arg)
    {
        // Nothing should be NULL
        assert(arg);
        try
        {
            gdrive_auth_file.assign(arg);
        }
        catch (const exception& e)
        {
            // This may fail, but at least try
            throw new BadOptionException("Could not allocate memory for options");
        }
    }

    /**
     * Set the interaction type
     * @param pOptions
     * @param arg
     * @return false on success, true on error
     */
    void Options::fudr_options_set_interaction(const char* arg)
    {
        // Nothing should be NULL
        assert(arg);

        if (!strcmp(arg, "never"))
        {
            gdrive_interaction_type = GDRIVE_INTERACTION_NEVER;
        }
        else if (!strcmp(arg, "startup"))
        {
            gdrive_interaction_type = GDRIVE_INTERACTION_STARTUP;
        }
        else if (!strcmp(arg, "always"))
        {
            gdrive_interaction_type = GDRIVE_INTERACTION_ALWAYS;
        }
        else
        {
            error = true;
            const char* fmtStr = "Unrecognized interaction type '%s'. Valid values "
                                 "are always, never, and startup\n";
            throw new BadOptionException(fmtStr, arg);
        }
    }

    /**
     * Set the file permissions
     * @param pOptions
     * @param arg
     * @return false on success, true on error
     */
    void Options::fudr_options_set_fileperm(const char* arg)
    {
        // Nothing should be NULL
        assert(arg);

        char* end = NULL;
        long filePerm = strtol(arg, &end, 8);
        BadOptionException* pBoe = NULL;
        if (end == arg)
        {
            const char* fmtStr = "Invalid file permission '%s', not an octal "
                                 "integer\n";
            pBoe = new BadOptionException(fmtStr, arg);
        }
        else if (filePerm > 0777)
        {
            const char* fmtStr = "Invalid file permission '%s', should be three "
                                 "octal digits\n";
            pBoe = new BadOptionException(fmtStr, arg);
        }
        else
        {
            file_perms = filePerm;
        }
        
        if (pBoe)
        {
            throw *pBoe;
        }
    }

    /**
     * Set directory permissions
     * @param pOptions
     * @param arg
     * @return false on success, true on error
     */
    void Options::fudr_options_set_dirperm(const char* arg)
    {
        // Nothing should be NULL
        assert(arg);

        char* end = NULL;
        long long dirPerm = strtol(arg, &end, 8);
        BadOptionException* pBoe = NULL;
        if (end == arg)
        {
            const char* fmtStr = "Invalid directory permission '%s', not an octal "
                                 "integer\n";
            pBoe = new BadOptionException(fmtStr, arg);
        }
        else if (dirPerm > 0777)
        {
            const char* fmtStr = "Invalid directory permission '%s', should be "
                                 "three octal digits\n";
            pBoe = new BadOptionException(fmtStr, arg);
        }
        else
        {
            dir_perms = dirPerm;
        }
        
        if (pBoe)
        {
            throw *pBoe;
        }
    }

    /**
     * Set cache TTL
     * @param pOptions
     * @param arg
     * @return false on success, true on error
     */
    void Options::fudr_options_set_cachettl(const char* arg)
    {
        // Nothing should be NULL
        assert(arg);

        char* end = NULL;
        long cacheTime = strtol(arg, &end, 10);
        if (end == arg)
        {
            error = true;
            const char* fmtStr = "Invalid cache-time '%s', not an integer\n";
            throw new BadOptionException(fmtStr, arg);
        }
        gdrive_cachettl = cacheTime;
    }

    /**
     * Set chunk size
     * @param pOptions
     * @param arg
     * @return false on success, true on error
     */
    void Options::fudr_options_set_chunksize(const char* arg)
    {
        // Nothing should be NULL
        assert(arg);

        char* end = NULL;
        long long chunkSize = strtoll(arg, &end, 10);
        if (end == arg)
        {
            error = true;
            const char* fmtStr = "Invalid chunk size '%s', not an integer\n";
            throw new BadOptionException(fmtStr, arg);
        }
        gdrive_chunk_size = chunkSize;
    }

    /**
     * Set max chunks
     * @param pOptions
     * @param arg
     * @return false on success, true on error
     */
    void Options::fudr_options_set_maxchunks(const char* arg)
    {
        // Nothing should be NULL
        assert(arg);

        char* end = NULL;
        long maxChunks = strtol(arg, &end, 10);
        if (end == arg)
        {
            error = true;
            const char* fmtStr = "Invalid max chunks '%s', not an integer\n";
            throw new BadOptionException(fmtStr, arg);
        }
        
        gdrive_max_chunks = maxChunks;
    }

    /**
     * Set unrecognized option or option with required value but no value provided
     * @param pOptions
     * @param argVal:   The value of the actual argument found (as from optopt)
     * @param longIndex
     * @return   true (to be consistent with other fudr_options_set* functions that
     *          return true on error)
     */
    void Options::fudr_options_set_failed(int argVal, 
                                        const struct option* longopts, 
                                        int longIndex)
    {
        error = true;

        // Assume short option for initialization
        char shortArgStr[] = {(char) argVal, '\0'};
        const char* argStr;
        const char* fmtStr;
        if (argVal > 32 && argVal < 256)
        {
            // Short option
            argStr = shortArgStr;
            fmtStr = "Unrecognized option, or no value given "
                             "for option: '-%s'\n";
        }
        else
        {
            // Long option
            argStr = longopts[longIndex].name;
            fmtStr = "Unrecognized option, or no value given "
                             "for option: '%s'\n";
        }
        
        // Throw exception to be consistent with other functions
        throw new BadOptionException(fmtStr, argStr);
    }

    void Options::cleanup()
    {
        if (fuse_argv)
        {
//            for (int i = 0; i < fuse_argc; i++)
//            {
//                //free(fuse_argv[i]);
//                printf("%d - %s\n", i, fuse_argv[i]);
//            }
            delete[] fuse_argv;
        }
        
    }
    
    
    
    BadOptionException::BadOptionException(const char* message)
    {
        msg = message ? strdup(message) : NULL;
    }
    
    BadOptionException::BadOptionException(const char* fmtStr, const char* arg)
    {
        
        if (!(fmtStr && fmtStr[0] && arg))
        {
            msg = NULL;
            return;
        }

        try
        {
            int length = snprintf(NULL, 0, fmtStr, arg);

            this->msg = new char[length + 1];

            snprintf(msg, length + 1, fmtStr, arg);
        }
        catch (const exception& e)
        {
            // Don't throw another exception while creating this one.
            // Just do nothing and let the message be blank.
            this->msg = NULL;
        }
    }
    
    const char* BadOptionException::what() const noexcept
    {
        return msg;
    }
    
    BadOptionException::~BadOptionException()
    {
        if (msg)
        {
            delete msg;
        }
    }
}
