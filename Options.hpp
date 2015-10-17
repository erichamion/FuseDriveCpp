/* 
 * File:   Options.hpp
 * Author: me
 *
 * Created on October 14, 2015, 8:39 AM
 */

#ifndef OPTIONS_HPP
#define	OPTIONS_HPP

#include <sys/types.h>
#include <string>
    
#include "gdrive/Gdrive.hpp"



namespace fusedrive
{
    class Options {
    public:
        // Access level for Google Drive, one of the GDRIVE_ACCESS_* constants
        int gdrive_access;

        // Path to config/auth file
        std::string gdrive_auth_file;

        // Time (in seconds) to assume cached data is still valid
        time_t gdrive_cachettl;

        // Determines when user interaction is allowed if Google Drive
        // authentication fails
        enum Gdrive_Interaction gdrive_interaction_type;

        // Size of file chunks
        size_t gdrive_chunk_size;

        // Maximum number of chunks per file
        int gdrive_max_chunks;

        // Permissions for files. Interpreted as a 3-digit octal number
        unsigned long file_perms;

        // Permissions for files. Interpreted as a 3-digit octal number
        unsigned long dir_perms;

        // Arguments to be passed on to FUSE
        char** fuse_argv;

        // Length of fuse_argv array
        int fuse_argc;

        // True if there was an error parsing command line options
        bool error;

        // If non-NULL, an error message that may be displayed to the user
        // char* errorMsg;
        
        /**
         * Create a Options object and fill it with values based on 
         * user-specified command line arguments. Any options not specified
         * by command line arguments will have default values.
         * @param argc The same argc that was passed in to main().
         * @param argv The same argv that was passed in to main().
         */
        Options(int argc, char** argv);
        
        Options(const Options& orig);
        
        
        virtual ~Options();
        
    private:
        static const char OPTION_ACCESS;
        static const char OPTION_CONFIG;
        static const char OPTION_INTERACTION;
        static const char OPTION_FILEPERM;
        static const char OPTION_DIRPERM;
        static const unsigned int OPTION_CACHETTL;
        static const unsigned int OPTION_CHUNKSIZE;
        static const unsigned int OPTION_MAXCHUNKS;
        static const char* const OPTION_STRING;

        static const unsigned int DEFAULT_GDRIVE_ACCESS;
        static const char* const DEFAULT_AUTH_BASENAME;
        static const char* const DEFAULT_AUTH_RELPATH;
        static const unsigned int DEFAULT_CACHETTL;
        static const Gdrive_Interaction DEFAULT_INTERACTION;
        static const unsigned int DEFAULT_CHUNKSIZE;
        static const unsigned int DEFAULT_MAXCHUNKS;
        static const unsigned int DEFAULT_FILEPERMS;
        static const unsigned int DEFAULT_DIRPERMS;
        
        
        static void fudr_options_make_errormsg(char** pDest, 
            const char* fmtStr, const char* arg);
        
        void fudr_options_get_default_auth_file();

        void fudr_options_default();

        void fudr_options_set_access(const char* arg);

        void fudr_options_set_config(const char* arg);

        void fudr_options_set_interaction(const char* arg);

        void fudr_options_set_fileperm(const char* arg);

        void fudr_options_set_dirperm(const char* arg);

        void fudr_options_set_cachettl(const char* arg);

        void fudr_options_set_chunksize(const char* arg);

        void fudr_options_set_maxchunks(const char* arg);

        void fudr_options_set_failed(int arg, const struct option* longopts, 
                                            int longIndex);

        void cleanup();
        
    };
    
    class BadOptionException : std::exception
    {
    public:
        BadOptionException(const char* message);
        BadOptionException(const char* fmtStr, const char* arg);
        virtual const char* what() const noexcept;
        ~BadOptionException();
        
    private:
        char* msg;
        
    };
    
}   

    




#endif	/* OPTIONS_HPP */

