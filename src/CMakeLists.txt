cmake_minimum_required(VERSION 3.5)

project(pyCmpr)

# define prepcompiler flag
# add_compile_definitions(DYNAMIC_VMTK)

# include vtk headers
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

# include itk headers
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

# include pybind11
find_path(PYBIND_DIRECTORY ${PYBIND_DIRECTORY} pybind11.egg-info)
# add pybind11
add_subdirectory(${PYBIND_DIRECTORY} ${PYBIND_DIRECTORY})

# include vmtk headers
find_path(VMTK_DIRECTORY ${VMTK_DIRECTORY} vmtk_env.sh)
include_directories(${VMTK_DIRECTORY}/include/vmtk)

# link vmtk libraries
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    link_directories(${VMTK_DIRECTORY}/lib)
    FILE(GLOB vmtk ${VMTK_DIRECTORY}/lib/*.a)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    link_directories(${VMTK_DIRECTORY}/lib)
    FILE(GLOB vmtk ${VMTK_DIRECTORY}/lib/*.a)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  link_directories(${VMTK_DIRECTORY}/lib)
  FILE(GLOB vmtk ${VMTK_DIRECTORY}/lib/*.lib)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# adds a library pybindings that can be called by python
pybind11_add_module(pyCmpr CurvedReformation.cpp)

# link external libraries
set_property(TARGET pyCmpr PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(pyCmpr PRIVATE ${VTK_LIBRARIES} ${ITK_LIBRARIES} ${vmtk} )
