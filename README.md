# Scyther
## Curvilinear Multiplanar Reformat for Dicom Images 
### Current version: 0.1.0

This library provides reslicing related functionalities to probe a volume with a surface defined by the extrusion of an arbitrary curve.
Powered by VTK and binded to python using pybind11. 

### Pre-requisite
- Install vmtk : see https://github.com/vmtk/vmtk
- Install Visual Studio 2017+
- Install cmake-gui

## Installation & build

Clone repository:
`
$ git clone https://github.com/dvisionlab/scyther \n
$ cd scyther
`
Clone and install pybind11:
`
$ git clone https://github.com/pybind/pybind11
$ cd pybind11
$ python setup.py install
`
Then, from the project root, run cmake:
`
$ cmake-gui ./src ./build
`

- set `USE_PYHTON_INCLUDE_DIR ON`
- set `CMAKE_CONFIGURATION_TYPES Release`
- set `VMTK_DIR as path_to_vmtk_build/Install`
- set `VTK_DIR as path_to_vmtk_build/VTK_Build`
- set `ITK_DIR as path_to_vmtk_build/ITK_Build`

Configure & generate (alt+c, alt+g) to obtain the .sln file.
Open and build with Visual Studio.

In order to check the build and the python binding, open a shell and run:
`
$ pyhton
$ >>> import pyCmpr
`
# Modules
- `main` -> entry point for python binding or c++ stand-alone usage
- `render` -> visualization tools (using VTK render), useful for debugging

# Roadmap
- Spatial points remapping

# Dependencies
- VTK
- Pybind11
