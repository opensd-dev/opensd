cmake_minimum_required(VERSION 3.10)

project(OpenSD)

find_package(Boost REQUIRED COMPONENTS serialization)
find_package(MPI REQUIRED)

if (MPI_FOUND)
  message(STATUS "Found MPI: ${MPI_CXX_COMPILER}")
else()
  message(FATAL_ERROR "MPI not found!")
endif()


set(COOLPROP_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
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


add_subdirectory ("${CMAKE_SOURCE_DIR}/externals/CoolProp")

file(GLOB SOURCES "src/*.cpp")
add_executable(${EXENAME} ${SOURCES})

target_include_directories(${EXENAME} PUBLIC
  include
  externals/CoolProp/externals/Eigen
  externals/CoolProp/include
#  externals/fmtlib
  ${MPI_CXX_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

target_link_libraries(${EXENAME}
  PUBLIC
    CoolProp
    pugixml
    MPI::MPI_CXX
    Boost::serialization
)

install(TARGETS ${EXENAME})
