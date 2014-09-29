CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(CMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT TRUE)
SET(CMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES FALSE)

PROJECT(OsgUEye)

SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Check if we are doing out of source builds
STRING(COMPARE NOTEQUAL ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} CMAKE_OUT_OF_SOURCE_BUILD)

# Relative paths can be trouble when doing out of source builds
IF(CMAKE_OUT_OF_SOURCE_BUILD)
	SET(DEFAULT_RELATIVE_PATHS OFF)
ELSE()
	SET(DEFAULT_RELATIVE_PATHS ON)
ENDIF()

# Show relative paths options
OPTION(CMAKE_USE_RELATIVE_PATHS "If true, cmake will use relative paths in makefiles and projects." ${DEFAULT_RELATIVE_PATHS})

IF(CMAKE_CONFIGURATION_TYPES)
   SET(CMAKE_CONFIGURATION_TYPES Debug Release)
   set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
     "Reset the configurations to what we need"
     FORCE)
ENDIF()


IF (WIN32)
	# Where to find OSG directory
	SET(OSG_DIR $ENV{OSG_DIR} CACHE PATH "Path where to find the OpenSceneGraph")
	IF(NOT OSG_DIR)
		MESSAGE(FATAL_ERROR "Error: OpenSceneGraph not found.")
	ENDIF(NOT OSG_DIR)
	
	# Where to find OSG 3rd party directory
	SET(OSG_THIRD_PARTY_DIR CACHE PATH "Path where to find the osg third party dependencies")
	IF(NOT OSG_THIRD_PARTY_DIR)
		MESSAGE(FATAL_ERROR "Error: OpenSceneGraph 3rd Party Directory not found.")
	ENDIF(NOT OSG_THIRD_PARTY_DIR)
	
	# Where to find OSG 3rd party directory
	SET(UEYE_SDK_ROOT_DIR CACHE PATH "Path where to find the UEye SDK Develop")
	IF(NOT UEYE_SDK_ROOT_DIR)
		MESSAGE(FATAL_ERROR "Error: UEye SDK Develop Directory not found.")
	ENDIF(NOT UEYE_SDK_ROOT_DIR)
ENDIF(WIN32)

#Solution
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

#######################################
# Library files
#######################################
FIND_PACKAGE( OpenGL REQUIRED )
FIND_PACKAGE( OpenSceneGraph REQUIRED osgViewer osgDB osgGA)
FIND_PACKAGE( UEyeSDK REQUIRED )

INCLUDE_DIRECTORIES(BEFORE
    ${OPENGL_INCLUDE_DIR}
    ${OPENSCENEGRAPH_INCLUDE_DIR}
	${UEYE_SDK_INCLUDE_DIRS}
)

###############################################################################
# Compiler Options
################################################################################
IF(WIN32)
	# Make sure we use minimal windows.h library without min max macros
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"WIN32_LEAN_AND_MEAN\"")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"NOMINMAX\"")
	
	# Use Link Time Code Generation
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	
    IF(MSVC80)
		# Detech 64-bit portability Issues
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wp64")
    ENDIF(MSVC80)
	
	OPTION(MULTI_PROCESSOR_COMPILATION "Use multiple processors when compiling" ON)
	IF(MULTI_PROCESSOR_COMPILATION)
		# Set multi processor build
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
	ENDIF(MULTI_PROCESSOR_COMPILATION)
	# Set warnings to level 4 
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	IF (WARNINGS_AS_ERRORS)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
	ENDIF (WARNINGS_AS_ERRORS)
ENDIF(WIN32)

IF(UNIX)
	IF(CMAKE_COMPILER_IS_GNUCC)
		ADD_DEFINITIONS(-Wall -ansi -pedantic -Wextra)
		IF (WARNINGS_AS_ERRORS)
			ADD_DEFINITIONS(-Werror)		
		ENDIF(WARNINGS_AS_ERRORS)
	ENDIF(CMAKE_COMPILER_IS_GNUCC)
ENDIF(UNIX)

###############################################################################
# Compile subdirectory
################################################################################
ADD_SUBDIRECTORY(src)