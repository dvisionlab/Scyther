# Scyther
## Curvilinear Multiplanar Reformat for Dicom Images :man_health_worker: :desktop_computer:	
### Current version: 0.2.1

This library provides reslicing related functionalities to probe a volume with a surface defined by the extrusion of an arbitrary curve.
Powered by VTK and binded to python using pybind11. 

### Pre-requisite
- Install vmtk : see https://github.com/vmtk/vmtk
- Install Visual Studio 2017+ / a c++ compiler
- Install cmake-gui / ccmake

## :building_construction: Installation & build 

Clone repository:

    $ git clone https://github.com/dvisionlab/scyther
    $ cd scyther

Clone and install pybind11:  

    $ git clone https://github.com/pybind/pybind11
    $ cd pybind11
    $ python setup.py install

Then, from the project root, run cmake:  
    
    $ cmake-gui ./src ./build

- set `USE_PYHTON_INCLUDE_DIR ON`
- set `CMAKE_CONFIGURATION_TYPES Release`
- set `VMTK_DIR as path_to_vmtk_build/Install`
- set `VTK_DIR as path_to_vmtk_build/VTK_Build`
- set `ITK_DIR as path_to_vmtk_build/ITK_Build`

Configure & generate (alt+c, alt+g) to obtain the .sln file.
Open and build with Visual Studio.

In order to check the build and the python binding, open a shell and run:  
    
    $ pyhton
    >>> import pyCmpr

## :open_file_folder: Modules 
- `main` -> entry point for python binding or c++ stand-alone usage
- `cmpr` -> functions mapped to python, define the type of cmpr
- `stack` -> manipulate volume 
- `geometry` -> manipulate slice geometry
- `test` -> testing code
- `render` -> visualization tools (using VTK render), useful for debugging

## :world_map: Roadmap 
- [x] Return image pseudo-header
- [ ] Spatial points remapping
- [x] Update readme
- [ ] Open issue for vmtk static on Linux

## :family_man_woman_boy: Dependencies 
- VTK / ITK / VMTK
- Pybind11

## Usage example 
    import json
    import pyCmpr as cmpr

    # inputs
    image_path      = "/path/to/image.nrrd"
    frenetTangent   = list of frenet tangents for each spline point, [x,y,z,x,y,z,x,y,z...]
    ptn             = list the parallel transport normals for each spline point, [x,y,z,x,y,z,x,y,z...]
    seeds_pts       = list the spline points [x,y,z,x,y,z,x,y,z...]
    sweep_dir       = sweeping line direction [x, y, z]
    sweep_length    = sweeping line length, as float 
    resolution      = number of points in the cmpr direction (ie number of columns in the resultiong image), as int 
    stack_direction = direction to be used to create the slices stack for cmpr direction, as [x, y, z]
    slice_dimension = size of the (squared) axial slice, as float
    dist_btw_slices = distance between cmpr slices
    n_slices        = number of cmpr slices

    # straightened
    volume = cmpr.compute_cmpr_straight(image_path, seeds_pts, frenetTangent, ptn, resolution, sweep_dir,
                                          stack_direction, slice_dimension, dist_btw_slices, n_slices, True)
    # stretched
    volume = cmpr.compute_cmpr_stretch(image_path, seeds_pts, resolution, sweep_dir,
                                          stack_direction, dist_btw_slices, n_slices, True)
    # output
    volume is an object that contains : 

    volume["pixels_cmpr"]       = list of pixel values for cmpr volume
    volume["pixels_axial"]      = list of pixel values for axial volume
    volume["metadata"]          = list of metadata from original nrrd serie (origin, dimensions, bounds)
    volume["dimension_cmpr"]    = dimensions of the resulting cmpr volume, [i,j,k]
    volume["dimension_axial"]   = dimensions of the resulting axial volume, [i,j,k]
    volume["iop_axial"]         = list of image orientation patient vectors, [1, y1, z1, x2, y2, z2, x1, y1, y1, ...]
    volume["ipp_axial"]         = list of image position patient, [x, y, z, x, y, z, ...]
