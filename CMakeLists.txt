cmake_minimum_required(VERSION 3.10)

project(OpenSD)

#===============================================================================
# Command line options
#===============================================================================
option(OPENSD_USE_MPI         "Enable MPI"                                           OFF)

message(STATUS "OPENSD_USE_MPI ${OPENSD_USE_MPI}")

#===============================================================================
# MPI for distributed-memory parallelism
#===============================================================================

if(OPENSD_USE_MPI)
  find_package(MPI REQUIRED)
endif()

find_package(HDF5 REQUIRED COMPONENTS C HL)

if (MPI_FOUND)
  message(STATUS "Found MPI: ${MPI_CXX_COMPILER}")
else()
  message(FATAL_ERROR "MPI not found!")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(PETSC REQUIRED IMPORTED_TARGET petsc)

find_package(fmt REQUIRED)

set(COOLPROP_STATIC_LIBRARY ON CACHE BOOL "Build static CoolProp library" FORCE)
SET(EXENAME opensd)


macro(find_package_write_status pkg)
  find_package(${pkg} QUIET NO_SYSTEM_ENVIRONMENT_PATH)
  if(${pkg}_FOUND)
    message(STATUS "Found ${pkg}: ${${pkg}_DIR} (version ${${pkg}_VERSION})")
  else()
    message(STATUS "Did not find ${pkg}, will use submodule instead")
  endif()
endmacro()

# Check to see if submodules exist (by checking one)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/externals/pugixml/CMakeLists.txt")
  message(FATAL_ERROR "The git submodules were not downloaded! GIT_SUBMODULE was \
    turned off or failed. Please update submodules and try again.")
endif()

#===============================================================================
# pugixml library
#===============================================================================

find_package_write_status(pugixml)
if (NOT pugixml_FOUND)
  add_subdirectory(externals/pugixml)
  set_target_properties(pugixml PROPERTIES CXX_STANDARD 14 CXX_EXTENSIONS OFF)
endif()

# Find METIS manually since no config file is provided
find_path(METIS_INCLUDE_DIR metis.h
  PATHS /usr/include /usr/local/include
)

find_library(METIS_LIBRARY metis
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

if(NOT METIS_INCLUDE_DIR OR NOT METIS_LIBRARY)
  message(FATAL_ERROR "Could not find METIS library or include path")
else()
  message(STATUS "Found METIS library: ${METIS_LIBRARY}")
  message(STATUS "Found METIS include: ${METIS_INCLUDE_DIR}")
endif()


add_subdirectory ("${CMAKE_SOURCE_DIR}/externals/CoolProp")
include_directories(${HDF5_INCLUDE_DIRS})
include_directories(include)

file(GLOB SOURCES "src/*.cpp")
add_executable(${EXENAME} ${SOURCES})

target_include_directories(${EXENAME} PUBLIC
  include
  externals/CoolProp/externals/Eigen
  externals/CoolProp/include
  external/fmtlib
  ${MPI_CXX_INCLUDE_DIRS}
  ${METIS_INCLUDE_DIRS}
)

if (OPENSD_USE_MPI)
  target_compile_definitions(${EXENAME} PUBLIC -DOPENSD_MPI)
endif()

target_link_libraries(${EXENAME}
  PUBLIC
    CoolProp
    pugixml
	${HDF5_C_LIBRARIES} ${HDF5_HL_LIBRARIES}
	MPI::MPI_CXX
    PkgConfig::PETSC
    fmt::fmt-header-only
	${METIS_LIBRARY}
)

# if (OPENSD_USE_MPI)
#   target_link_libraries(${EXENAME} MPI::MPI_CXX)
# endif()

install(TARGETS ${EXENAME})
