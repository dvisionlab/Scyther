// comment this line if vmtk is compiled static on Linux: exclude rendering vtk libs
// #define DYNAMIC_VMTK

// std libs
#include <vector>

// pybind lib
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// vtk stuff
#include <vtkSmartPointer.h>
#include <vtkPolyDataReader.h>
#include <vtkProbeFilter.h>
#include <vtkImageData.h>
#include <vtkSplineFilter.h>
#include <vtkPoints.h>
#include <vtkPlane.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkNrrdReader.h>
#include <vtkArrayData.h>
#include <vtkPointData.h>
#include <vtkDoubleArray.h>
#include <vtkPlaneSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkAppendPolyData.h>

#ifdef DYNAMIC_VMTK
#include <vtkWindowLevelLookupTable.h>
#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#endif

// custom libs
#include "stack.h"
#include "test.h"

#ifdef DYNAMIC_VMTK
#include "render.h"
#endif

// TODO
// - project spline on volume bound plane using extrusion direction negated DONE
// - create stack in both direction DONE
// - return metadata DONE
// - set ifdef for linking static vmtk DONE
// - create pseudo axial stack (slices along spline) DONE
// - render source spline for debug DONE
// - return stack dimensions DONE

std::map<std::string, std::vector<float>> compute_cmpr(std::string volumeFileName,
                                                       std::vector<float> seeds,
                                                       unsigned int resolution,
                                                       std::vector<int> dir,
                                                       std::vector<float> stack_direction,
                                                       float dist_slices,
                                                       int n_slices,
                                                       bool render)
{
  time_t time_0;
  time(&time_0);

  // Parse input
  double direction[3];
  std::copy(dir.begin(), dir.end(), direction);

  // Print arguments
  std::cout << "InputVolume: " << volumeFileName << std::endl
            << "Resolution: " << resolution << std::endl;

  // Read the volume data
  vtkSmartPointer<vtkNrrdReader> reader = vtkSmartPointer<vtkNrrdReader>::New();
  reader->SetFileName(volumeFileName.c_str());
  reader->Update();

  std::vector<float> metadata = GetMetadata(reader->GetOutput());

  double origin[3] = {
      metadata[0],
      metadata[1],
      metadata[2],
  };
  double neg_direction[3] = {
      -direction[0],
      -direction[1],
      -direction[2]};

  // Recreate source spline
  vtkSmartPointer<vtkPolyData> original_spline = vtkSmartPointer<vtkPolyData>::New();
  original_spline = CreateSpline(seeds, resolution, origin, neg_direction, false);

  // Compute spline projection
  vtkSmartPointer<vtkPolyData> spline = vtkSmartPointer<vtkPolyData>::New();
  spline = CreateSpline(seeds, resolution, origin, neg_direction, true);

  // Compute sweep distance
  double distance;
  // TODO get max direction
  if (direction[0] == 1.0)
  {
    distance = metadata[7] - metadata[6];
  }
  else if (direction[1] == 1.0)
  {
    distance = metadata[9] - metadata[8];
  }
  else if (direction[2] == 1.0)
  {
    distance = metadata[11] - metadata[10];
  }

  std::cout << "sweep distance " << distance << std::endl;

  // Sweep the line to form a surface
  vtkSmartPointer<vtkPolyData> master_slice = SweepLine(spline, direction, distance, resolution);

  // Shift the master slice to create a stack
  std::map<int, vtkSmartPointer<vtkPolyData>> stack_map = CreateStack(master_slice, n_slices, stack_direction, dist_slices);

  // Squash stack map into a single polydata
  vtkSmartPointer<vtkPolyData> complete_stack = Squash(stack_map, false);

  // Compute axial stack
  float axial_side_length = 120.0;
  std::map<int, vtkSmartPointer<vtkPolyData>> axial_stack_map = CreateAxialStack(original_spline, axial_side_length, resolution);

  // Squash stack map into a single polydata
  vtkSmartPointer<vtkPolyData> complete_axial_stack = Squash(axial_stack_map, false);

  // for (int i = 0; i < stack_map.size(); i++)
  // {
  //    std::cout << i << " stack: " << stack_map[i]->GetNumberOfPoints() << std::endl;
  // }

  std::cout << "complete_stack number of points: " << complete_stack->GetNumberOfPoints() << std::endl;

  // Probe the volume with the extruded surfaces
  vtkSmartPointer<vtkProbeFilter> sampleVolume = vtkSmartPointer<vtkProbeFilter>::New();
  sampleVolume->SetInputConnection(1, reader->GetOutputPort());
  sampleVolume->SetInputData(0, complete_stack);
  sampleVolume->Update();

  vtkSmartPointer<vtkProbeFilter> sampleVolumeAxial = vtkSmartPointer<vtkProbeFilter>::New();
  sampleVolumeAxial->SetInputConnection(1, reader->GetOutputPort());
  sampleVolumeAxial->SetInputData(0, complete_axial_stack);
  sampleVolumeAxial->Update();

  // Test
  // bool response = test_alg(viewPlane, sampleVolume->GetOutput());
  // if (response) std::cout << "test passed" << std::endl;
  // else std::cout << "test failed" << std::endl;

  time_t time_1;
  time(&time_1);

  std::cout << "Total : " << difftime(time_1, time_0) << "[s]" << std::endl;

#ifdef DYNAMIC_VMTK
  // Render
  if (render)
  {
    int res = renderAll(original_spline, sampleVolumeAxial, reader->GetOutput(), resolution);
    // int res = renderAll(original_spline, sampleVolume, reader->GetOutput(), resolution);
  }
#endif

  // Get values from probe output
  std::vector<float> values_cmpr = GetPixelValues(sampleVolume->GetOutput(), false);
  std::vector<float> values_axial = GetPixelValues(sampleVolumeAxial->GetOutput(), true);

  std::map<std::string, std::vector<float>> response;

  // Compute mean distance btw points to be returned as image spacing
  float mean_pts_distance = GetMeanDistanceBtwPoints(spline);
  float range = GetWindowWidth(reader->GetOutput());

  // Compose response with metadata
  std::vector<float> dimension_cmpr = GetDimensions(stack_map);
  std::vector<float> dimension_axial = GetDimensions(axial_stack_map);
  std::vector<float> spacing_cmpr = {
      float(distance / resolution),
      mean_pts_distance};
  std::vector<float> spacing_axial = {
      axial_side_length / resolution,
      axial_side_length / resolution};
  std::vector<float> wwwl_cmpr = {
      range,
      range / 2};
  std::vector<float> wwwl_axial = {
      range,
      range / 2};

  response["metadata"] = metadata;
  response["pixels_cmpr"] = values_cmpr;
  response["pixels_axial"] = values_axial;
  response["dimension_cmpr"] = dimension_cmpr;
  response["dimension_axial"] = dimension_axial;
  response["spacing_cmpr"] = spacing_cmpr;
  response["spacing_axial"] = spacing_axial;
  response["wwwl_cmpr"] = wwwl_cmpr;
  response["wwwl_axial"] = wwwl_axial;

  return response;
}

int main(int argc, char *argv[])
{
  // TODO get input from argv:
  // - nrrd file path
  // - vtk polyline file path
  // - resolution
  // - direction
  // then convert file to points array

  // Verify arguments
  if (argc < 4)
  {
    std::cout << "Usage: " << argv[0]
              << " InputVolume PolyDataInput"
              << " Resolution"
              << std::endl;
    return EXIT_FAILURE;
  }

  // Parse arguments
  std::string volumeFileName = argv[1];
  std::string polyDataFileName = argv[2];
  unsigned int resolution;

  // Output arguments
  std::cout << "InputVolume: " << volumeFileName << std::endl
            << "PolyDataInput: " << polyDataFileName << std::endl
            << "Resolution: " << resolution << std::endl;

  // Read the Polyline
  vtkSmartPointer<vtkPolyDataReader> polyLineReader =
      vtkSmartPointer<vtkPolyDataReader>::New();
  polyLineReader->SetFileName(polyDataFileName.c_str());
  polyLineReader->Update();

  std::cout << "---> " << polyLineReader->GetOutput()->GetNumberOfPoints() << std::endl;

  return 0;
}

// define a module to be imported by python
PYBIND11_MODULE(pyCmpr, m)
{
  m.def("compute_cmpr", &compute_cmpr, "", "");
}
