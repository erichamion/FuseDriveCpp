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
CND_CONF=Release
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
	${OBJECTDIR}/gdrive/Cache.o \
	${OBJECTDIR}/gdrive/CacheNode.o \
	${OBJECTDIR}/gdrive/DownloadBuffer.o \
	${OBJECTDIR}/gdrive/FileContents.o \
	${OBJECTDIR}/gdrive/FileIdCacheNode.o \
	${OBJECTDIR}/gdrive/Fileinfo.o \
	${OBJECTDIR}/gdrive/FileinfoArray.o \
	${OBJECTDIR}/gdrive/Gdrive.o \
	${OBJECTDIR}/gdrive/GdriveFile.o \
	${OBJECTDIR}/gdrive/GdriveInfo.o \
	${OBJECTDIR}/gdrive/HttpQuery.o \
	${OBJECTDIR}/gdrive/HttpTransfer.o \
	${OBJECTDIR}/gdrive/Json.o \
	${OBJECTDIR}/gdrive/NullStream.o \
	${OBJECTDIR}/gdrive/Sysinfo.o \
	${OBJECTDIR}/gdrive/Util.o \
	${OBJECTDIR}/gdrive/gdrive-transfer.o \
	${OBJECTDIR}/gdrive/gdrive-util.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/fusedrive__

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/fusedrive__: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/fusedrive__ ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/FuseDrivePrivateData.o: FuseDrivePrivateData.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/FuseDrivePrivateData.o FuseDrivePrivateData.cpp

${OBJECTDIR}/Options.o: Options.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Options.o Options.cpp

${OBJECTDIR}/fuse-drive.o: fuse-drive.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/fuse-drive.o fuse-drive.cpp

${OBJECTDIR}/gdrive/Cache.o: gdrive/Cache.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/Cache.o gdrive/Cache.cpp

${OBJECTDIR}/gdrive/CacheNode.o: gdrive/CacheNode.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/CacheNode.o gdrive/CacheNode.cpp

${OBJECTDIR}/gdrive/DownloadBuffer.o: gdrive/DownloadBuffer.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/DownloadBuffer.o gdrive/DownloadBuffer.cpp

${OBJECTDIR}/gdrive/FileContents.o: gdrive/FileContents.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/FileContents.o gdrive/FileContents.cpp

${OBJECTDIR}/gdrive/FileIdCacheNode.o: gdrive/FileIdCacheNode.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/FileIdCacheNode.o gdrive/FileIdCacheNode.cpp

${OBJECTDIR}/gdrive/Fileinfo.o: gdrive/Fileinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/Fileinfo.o gdrive/Fileinfo.cpp

${OBJECTDIR}/gdrive/FileinfoArray.o: gdrive/FileinfoArray.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/FileinfoArray.o gdrive/FileinfoArray.cpp

${OBJECTDIR}/gdrive/Gdrive.o: gdrive/Gdrive.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/Gdrive.o gdrive/Gdrive.cpp

${OBJECTDIR}/gdrive/GdriveFile.o: gdrive/GdriveFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/GdriveFile.o gdrive/GdriveFile.cpp

${OBJECTDIR}/gdrive/GdriveInfo.o: gdrive/GdriveInfo.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/GdriveInfo.o gdrive/GdriveInfo.cpp

${OBJECTDIR}/gdrive/HttpQuery.o: gdrive/HttpQuery.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/HttpQuery.o gdrive/HttpQuery.cpp

${OBJECTDIR}/gdrive/HttpTransfer.o: gdrive/HttpTransfer.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/HttpTransfer.o gdrive/HttpTransfer.cpp

${OBJECTDIR}/gdrive/Json.o: gdrive/Json.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/Json.o gdrive/Json.cpp

${OBJECTDIR}/gdrive/NullStream.o: gdrive/NullStream.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/NullStream.o gdrive/NullStream.cpp

${OBJECTDIR}/gdrive/Sysinfo.o: gdrive/Sysinfo.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/Sysinfo.o gdrive/Sysinfo.cpp

${OBJECTDIR}/gdrive/Util.o: gdrive/Util.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/Util.o gdrive/Util.cpp

${OBJECTDIR}/gdrive/gdrive-transfer.o: gdrive/gdrive-transfer.cpp 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-transfer.o gdrive/gdrive-transfer.cpp

${OBJECTDIR}/gdrive/gdrive-util.o: gdrive/gdrive-util.c 
	${MKDIR} -p ${OBJECTDIR}/gdrive
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdrive/gdrive-util.o gdrive/gdrive-util.c

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
