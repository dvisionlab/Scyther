#include <time.h>

#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyLine.h>

#include "geometry.h"

std::vector<float> GetMetadata(vtkImageData *image)
{
  // Store the image and print some infos
  std::cout << "ORIGIN: "
            << image->GetOrigin()[0] << ", " << image->GetOrigin()[1] << ", " << image->GetOrigin()[2] << std::endl
            << "DIMENSION: "
            << image->GetDimensions()[0] << ", " << image->GetDimensions()[1] << ", " << image->GetDimensions()[2] << std::endl
            << "BOUNDS: "
            << image->GetBounds()[0] << ", " << image->GetBounds()[1] << ", " << image->GetBounds()[2] << std::endl
            << image->GetBounds()[3] << ", " << image->GetBounds()[4] << ", " << image->GetBounds()[5] << std::endl;

  std::vector<float> metadata;
  
  metadata.push_back(image->GetOrigin()[0]);
  metadata.push_back(image->GetOrigin()[1]);
  metadata.push_back(image->GetOrigin()[2]);
  metadata.push_back(image->GetDimensions()[0]);
  metadata.push_back(image->GetDimensions()[1]);
  metadata.push_back(image->GetDimensions()[2]);
  metadata.push_back(image->GetBounds()[0]);
  metadata.push_back(image->GetBounds()[1]);
  metadata.push_back(image->GetBounds()[2]);
  metadata.push_back(image->GetBounds()[3]);
  metadata.push_back(image->GetBounds()[4]);
  metadata.push_back(image->GetBounds()[5]);

  return metadata;
}

std::map<int, vtkSmartPointer<vtkPolyData>> CreateStack(vtkPolyData *master_slice, int n_slices, std::vector<float> direction, float dist_slices)
{
  std::map<int, vtkSmartPointer<vtkPolyData>> stack;

  time_t time_0;
  time(&time_0);

  vtkMath::MultiplyScalar(direction.data(), dist_slices);
  int slice_id = 0;

  for (int s = -n_slices/2; s < n_slices/2; s++)
  {
    slice_id = s + n_slices/2;
    stack[slice_id] = ShiftMasterSlice(master_slice, s, direction);
  }

  time_t time_1;
  time(&time_1);

  std::cout << "CreateStack tooks : " << difftime(time_1, time_0) << " [s]" << std::endl;

  return stack;
}

std::map<int, vtkSmartPointer<vtkPolyData>> CreateAxialStack(vtkPolyData *spline, int resolution)
{
  std::map<int, vtkSmartPointer<vtkPolyData>> stack;

  vtkSmartPointer<vtkPolyData> targetPlane = vtkSmartPointer<vtkPolyData>::New();

  double p0[3];
  double p1[3];
  double n[3];
  
  for (int frame=0; frame<spline->GetNumberOfPoints()-1; frame+=10) // DEV restore ++
  {
    spline->GetPoint(frame, p0);
    spline->GetPoint(frame+1, p1);

    
    n[0] = p1[0]-p0[0];
    n[1] = p1[1]-p0[1];
    n[2] = p1[2]-p0[2];
    
    targetPlane = GetOrientedPlane(p0, n, resolution);    
    stack[frame] = targetPlane; 
  }

  return stack;
}

vtkSmartPointer<vtkPolyData> Squash(std::map<int, vtkSmartPointer<vtkPolyData>> stack_map)
{

  time_t time_0;
  time(&time_0);

  //Append all the meshes to a single polydata
  vtkSmartPointer<vtkPolyData> squashed = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
  appendFilter->AddInputData(squashed);

  for (int p = 0; p < stack_map.size(); p++)
  {
    appendFilter->AddInputData(stack_map[p]);
  }

  appendFilter->Update();

  time_t time_1;
  time(&time_1);

  std::cout << "Squash tooks : " << difftime(time_1, time_0) << " [s]" << std::endl;

  return appendFilter->GetOutput();
}

std::vector<float> GetPixelValues(vtkDataSet *dataset)
{
  std::vector<float> values;

  for (int i = 0; i < dataset->GetNumberOfPoints(); i++)
  {
    values.push_back(dataset->GetPointData()->GetArray("ImageFile")->GetTuple(i)[0]);
  }

  std::cout << "array filled with " << values.size() << " elements. " << std::endl;

  return values;
}



