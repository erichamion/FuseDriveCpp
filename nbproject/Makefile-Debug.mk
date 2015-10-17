#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/FuseDrivePrivateData.o \
	${OBJECTDIR}/Options.o \
	${OBJECTDIR}/fuse-drive.o \
	${OBJECTDIR}/gdrive/Gdrive.o \
	${OBJECTDIR}/gdrive/GdriveInfo.o \
	${OBJECTDIR}/gdrive/gdrive-cache-node.o \
	${OBJECTDIR}/gdrive/gdrive-cache.o \
	${OBJECTDIR}/gdrive/gdrive-download-buffer.o \
	${OBJECTDIR}/gdrive/gdrive-file-contents.o \
	${OBJECTDIR}/gdrive/gdrive-fileid-cache-node.o \
	${OBJECTDIR}/gdrive/gdrive-fileinfo-array.o \
	${OBJECTDIR}/gdrive/gdrive-fileinfo.o \
	${OBJECTDIR}/gdrive/gdrive-info.o \
	${OBJECTDIR}/gdrive/gdrive-json.o \
	${OBJECTDIR}/gdrive/gdrive-query.o \
	${OBJECTDIR}/gdrive/gdrive-sysinfo.o \
	${OBJECTDIR}/gdrive/gdrive-transfer.o \
	${OBJECTDIR}/gdrive/gdrive-util.o


# C Compiler Flags
CFLAGS=-Wextra

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=`pkg-config --libs fuse` `pkg-config --libs libcurl` -lm  `pkg-config --libs json-c`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/fusedrive__

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/fusedrive__: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/fusedrive__ ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/FuseDrivePrivateData.o: FuseDrivePrivateData.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/FuseDrivePrivateData.o FuseDrivePrivateData.cpp

${OBJECTDIR}/Options.o: Options.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Options.o Options.cpp

${OBJECTDIR}/fuse-drive.o: fuse-drive.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/fuse-drive.o fuse-drive.cpp

${OBJECTDIR}/gdrive/Gdrive.o: gdrive/Gdrive.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/Gdrive.o gdrive/Gdrive.cpp

${OBJECTDIR}/gdrive/GdriveInfo.o: gdrive/GdriveInfo.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/GdriveInfo.o gdrive/GdriveInfo.cpp

${OBJECTDIR}/gdrive/gdrive-cache-node.o: gdrive/gdrive-cache-node.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-cache-node.o gdrive/gdrive-cache-node.cpp

${OBJECTDIR}/gdrive/gdrive-cache.o: gdrive/gdrive-cache.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-cache.o gdrive/gdrive-cache.cpp

${OBJECTDIR}/gdrive/gdrive-download-buffer.o: gdrive/gdrive-download-buffer.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-download-buffer.o gdrive/gdrive-download-buffer.cpp

${OBJECTDIR}/gdrive/gdrive-file-contents.o: gdrive/gdrive-file-contents.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-file-contents.o gdrive/gdrive-file-contents.cpp

${OBJECTDIR}/gdrive/gdrive-fileid-cache-node.o: gdrive/gdrive-fileid-cache-node.c 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c99  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-fileid-cache-node.o gdrive/gdrive-fileid-cache-node.c

${OBJECTDIR}/gdrive/gdrive-fileinfo-array.o: gdrive/gdrive-fileinfo-array.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-fileinfo-array.o gdrive/gdrive-fileinfo-array.cpp

${OBJECTDIR}/gdrive/gdrive-fileinfo.o: gdrive/gdrive-fileinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-fileinfo.o gdrive/gdrive-fileinfo.cpp

${OBJECTDIR}/gdrive/gdrive-info.o: gdrive/gdrive-info.c 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c99  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-info.o gdrive/gdrive-info.c

${OBJECTDIR}/gdrive/gdrive-json.o: gdrive/gdrive-json.c 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c99  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-json.o gdrive/gdrive-json.c

${OBJECTDIR}/gdrive/gdrive-query.o: gdrive/gdrive-query.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-query.o gdrive/gdrive-query.cpp

${OBJECTDIR}/gdrive/gdrive-sysinfo.o: gdrive/gdrive-sysinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-sysinfo.o gdrive/gdrive-sysinfo.cpp

${OBJECTDIR}/gdrive/gdrive-transfer.o: gdrive/gdrive-transfer.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 -I/usr/include/ `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c++11  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-transfer.o gdrive/gdrive-transfer.cpp

${OBJECTDIR}/gdrive/gdrive-util.o: gdrive/gdrive-util.c 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=700 `pkg-config --cflags fuse` `pkg-config --cflags libcurl` `pkg-config --cflags json-c` -std=c99  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-util.o gdrive/gdrive-util.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/fusedrive__

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
