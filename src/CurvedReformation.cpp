// std libs
#include <vector>
#include <time.h>

// pybind lib
#ifdef USE_PYBIND
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#endif

namespace py = pybind11;

// picojson
#include "picojson.h"

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

std::map<std::string, std::vector<float>>
compute_cmpr_stretch(std::string volumeFileName,
                     std::vector<float> seeds,
                     unsigned int resolution,
                     std::vector<int> dir,
                     std::vector<float> stack_direction,
                     float dist_slices,
                     int n_slices,
                     bool render);
std::map<std::string, py::array_t<float>>
compute_cmpr_straight(std::string volumeFileName,
                      // std::map<std::string, std::vector<float>> compute_cmpr_straight(std::string volumeFileName,
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
// - return the list of the keys present in the response, in order to avoid python remaining stuck trying to read something that doesn't exist

std::string LoadFile(const std::string &path)
{
  auto ss = std::ostringstream{};
  std::ifstream file(path);
  ss << file.rdbuf();
  return ss.str();
}

std::vector<int> ArrayToVectorInteger(picojson::array input_array)
{
  std::vector<int> output_vect = {};

  for (int s = 0; s < input_array.size(); s++)
  {
    output_vect.push_back((int)input_array[s].get<double>());
  }

  return output_vect;
}

std::vector<float> ArrayToVectorFloat(picojson::array input_array)
{
  std::vector<double> output_vect = {};

  for (int s = 0; s < input_array.size(); s++)
  {
    output_vect.push_back(input_array[s].get<double>());
  }

  std::vector<float> output_float(output_vect.begin(), output_vect.end());

  return output_float;
}

int main(int argc, char *argv[])
{
  // READ FILE
  std::cout << "Reading inputs from : " << argv[1] << std::endl;
  std::string content = LoadFile(argv[1]);

  // PARSE JSON
  picojson::value v;
  std::string err = picojson::parse(v, content);
  if (!err.empty())
  {
    std::cerr << err << std::endl;
  }

  // check if the type of the value is "object"
  if (!v.is<picojson::object>())
  {
    std::cerr << "JSON is not an object" << std::endl;
    exit(2);
  }

  std::string method = "";
  std::string volume_file_path = "";
  std::vector<float> seeds = {};
  std::vector<float> tng = {};
  std::vector<float> ptn = {};
  unsigned int resolution;
  std::vector<int> dir;
  std::vector<float> stack_direction;
  float dist_slices;
  float slice_dimension;
  int n_slices;
  bool render;

  const picojson::value::object &obj = v.get<picojson::object>();
  for (picojson::value::object::const_iterator i = obj.begin();
       i != obj.end();
       ++i)
  {
    std::cout << i->first << " : " << i->second.to_str() << std::endl;

    if (i->first == "method")
    {
      method = i->second.get<std::string>();
    }
    else if (i->first == "volume_file_path")
    {
      volume_file_path = i->second.get<std::string>();
    }
    else if (i->first == "seeds")
    {
      picojson::array p_array = i->second.get<picojson::array>();
      seeds = ArrayToVectorFloat(p_array);
    }
    else if (i->first == "resolution")
    {
      resolution = (unsigned int)i->second.get<double>();
    }
    else if (i->first == "dist_slices")
    {
      dist_slices = (float)i->second.get<double>();
    }
    else if (i->first == "render")
    {
      render = i->second.get<bool>();
    }
    else if (i->first == "sweep_dir")
    {
      picojson::array p_array = i->second.get<picojson::array>();
      dir = ArrayToVectorInteger(p_array);
    }
    else if (i->first == "stack_direction")
    {
      picojson::array p_array = i->second.get<picojson::array>();
      stack_direction = ArrayToVectorFloat(p_array);
    }
    else if (i->first == "tng")
    {
      picojson::array p_array = i->second.get<picojson::array>();
      tng = ArrayToVectorFloat(p_array);
    }
    else if (i->first == "ptn")
    {
      picojson::array p_array = i->second.get<picojson::array>();
      ptn = ArrayToVectorFloat(p_array);
    }
    else if (i->first == "slice_dimension")
    {
      slice_dimension = (float)i->second.get<double>();
    }
    else if (i->first == "n_slices")
    {
      n_slices = (int)i->second.get<double>();
    }
  }

  // CALL METHOD
  if (method == "straight")
  {
    compute_cmpr_straight(volume_file_path, seeds, tng, ptn, resolution, dir, stack_direction, slice_dimension, dist_slices, n_slices, render);
  }
  else if (method == "stretched")
  {
    compute_cmpr_stretch(volume_file_path, seeds, resolution, dir, stack_direction, dist_slices, n_slices, render);
  }
  else
  {
    std::cout << "NO METHOD PROVIDED" << std::endl;
    exit(1);
  }

  return 0;
}

#ifdef USE_PYBIND
// define a module to be imported by python
PYBIND11_MODULE(pyCmpr, m)
{
  m.def("compute_cmpr_straight", &compute_cmpr_straight, "", "");
  m.def("compute_cmpr_stretch", &compute_cmpr_stretch, "", "");
}
#endif
