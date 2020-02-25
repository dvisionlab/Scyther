// std libs
#include <vector>
#include <time.h>

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
#include <vtkSmoothPolyDataFilter.h>
#include <vtkTriangleFilter.h>
// #include <vtkvmtkPolyDataKiteRemovalFilter.h>
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyLine.h>

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

// ==== DECLARATONS  ====

std::map<std::string, std::vector<float> *> compute_cmpr_stretch(std::string volumeFileName,
                                                                 std::vector<float> seeds,
                                                                 unsigned int resolution,
                                                                 std::vector<int> dir,
                                                                 std::vector<float> stack_direction,
                                                                 float dist_slices,
                                                                 int n_slices,
                                                                 bool render);
std::map<std::string, std::vector<float>> compute_cmpr_straight(std::string volumeFileName,
                                                                std::vector<float> seeds,
                                                                std::vector<float> tng,
                                                                std::vector<float> ptn,
                                                                unsigned int resolution,
                                                                std::vector<int> dir,
                                                                std::vector<float> stack_direction,
                                                                float slice_dimension,
                                                                float dist_slices,
                                                                int n_slices,
                                                                bool render);
std::vector<float> GetMetadata(vtkImageData *image);
std::map<int, vtkSmartPointer<vtkPolyData>> CreateStack(vtkPolyData *master_slice, int n_slices, std::vector<float> direction, float dist_slices);
std::map<int, vtkSmartPointer<vtkPolyData>> CreateAxialStack(vtkPolyData *spline, float side_length, int resolution, std::vector<float> &iop_axial, std::vector<float> &ipp_axial);
vtkSmartPointer<vtkPolyData> Squash(std::map<int, vtkSmartPointer<vtkPolyData>> stack_map, bool reverse);
std::vector<float> GetPixelValues(vtkDataSet *dataset, bool reverse);
std::vector<float> GetDimensions(std::map<int, vtkSmartPointer<vtkPolyData>> stack);
float GetWindowWidth(vtkSmartPointer<vtkImageData> image, float max, float min);
vtkSmartPointer<vtkPolyData> GetPlanar(vtkDataArray *pixels, vtkPolyData *spline);
int renderAll(vtkPolyData *spline, vtkProbeFilter *sampleVolume, vtkImageData *image, int resolution, float range);
vtkSmartPointer<vtkPolyData> CreateSpline(std::vector<float> seeds, int resolution, double origin[3], double normal[3], bool project);
vtkSmartPointer<vtkPolyData> SweepLine(vtkPolyData *line, std::vector<float> directions, double distance, int cols);
vtkSmartPointer<vtkPolyData> ShiftMasterSlice(vtkPolyData *original_surface, int index, std::vector<float> dir);
void GetIOPIPP(vtkSmartPointer<vtkPlaneSource> slice, double iop[6], double ipp[3]);
vtkSmartPointer<vtkPolyData> GetOrientedPlane(double origin[3], double normal[3], float side_length, int resolution, std::vector<float> &iop_axial, std::vector<float> &ipp_axial);
double GetMeanDistanceBtwPoints(vtkSmartPointer<vtkPolyData> spline);

// custom libs

#include "geometry.h"
#ifdef DYNAMIC_VMTK
#include "render.h"
#endif
#include "stack.h"
#include "cmpr.h"
#include "test.h"

// TODO
// - project spline on volume bound plane using extrusion direction negated DONE
// - create stack in both direction DONE
// - return metadata DONE
// - set ifdef for linking static vmtk DONE
// - create pseudo axial stack (slices along spline) DONE
// - render source spline for debug DONE
// - return stack dimensions DONE
// - return the list of the keys present in the response, in order to avoid python remaining stuck trying to read something that doesn't exist

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
  m.def("compute_cmpr_straight", &compute_cmpr_straight, "", "");
  m.def("compute_cmpr_stretch", &compute_cmpr_stretch, "", "");
}
