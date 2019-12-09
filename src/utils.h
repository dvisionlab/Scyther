#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyLine.h>

#include <time.h>

// Create a spline from an array of xyz points
vtkSmartPointer<vtkPolyData> CreateSpline(std::vector<float> seeds, int resolution)
{
  // Create a vtkPoints object and store the points in it
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();

  for ( auto i = seeds.begin(); i != seeds.end(); i+=3 ) {
    std::cout << *i << " " << *(i+1) << " " << *(i+2) << std::endl;
    points->InsertNextPoint(*i, *(i+1), *(i+2));
  } 

  vtkSmartPointer<vtkPolyLine> polyLine =
    vtkSmartPointer<vtkPolyLine>::New();
  polyLine->GetPointIds()->SetNumberOfIds(seeds.size()/3);
  for(unsigned int i = 0; i < seeds.size()/3; i++)
  {
    polyLine->GetPointIds()->SetId(i,i);
  }

  // Create a cell array to store the lines in and add the lines to it
  vtkSmartPointer<vtkCellArray> cells =
    vtkSmartPointer<vtkCellArray>::New();
  cells->InsertNextCell(polyLine);

  // Create a polydata to store everything in
  vtkSmartPointer<vtkPolyData> polyData =
    vtkSmartPointer<vtkPolyData>::New();

  // Add the points to the dataset
  polyData->SetPoints(points);

  // Add the lines to the dataset
  polyData->SetLines(cells);

    // Sample polyline to spline
  vtkSmartPointer<vtkSplineFilter> spline = vtkSmartPointer<vtkSplineFilter>::New();
  spline->SetInputData(polyData);
  spline->SetSubdivideToSpecified();
  spline->SetNumberOfSubdivisions(resolution);
  spline->Update();

  return spline->GetOutput();
}

// Extrude a spline to create a curved plane
vtkSmartPointer<vtkPolyData> SweepLine(vtkPolyData *line, double direction[3], double distance, unsigned int cols)
{
  unsigned int rows = line->GetNumberOfPoints();
  double spacing = distance / cols;

  std::cout << "rows, cols: " << rows << ", " << cols << std::endl; 

  vtkSmartPointer<vtkPolyData> surface = vtkSmartPointer<vtkPolyData>::New();

  // Generate the points
  // cols++; TODO evaluate if necessary
  unsigned int numberOfPoints = rows * cols;
  unsigned int numberOfPolys = (rows - 1) * (cols - 1);
  vtkSmartPointer<vtkPoints> points =
      vtkSmartPointer<vtkPoints>::New();
  points->Allocate(numberOfPoints);
  vtkSmartPointer<vtkCellArray> polys =
      vtkSmartPointer<vtkCellArray>::New();
  polys->Allocate(numberOfPolys * 4);

  double x[3];
  unsigned int cnt = 0;
  for (unsigned int row = 0; row < rows; row++)
  {
    for (unsigned int col = 0; col < cols; col++)
    {
      double p[3];
      line->GetPoint(row, p);
      x[0] = p[0] + direction[0] * col * spacing;
      x[1] = p[1] + direction[1] * col * spacing;
      x[2] = p[2] + direction[2] * col * spacing;
      points->InsertPoint(cnt++, x);
    }
  }
  // Generate the quads
  vtkIdType pts[4];
  for (unsigned int row = 0; row < rows - 1; row++)
  {
    for (unsigned int col = 0; col < cols - 1; col++)
    {
      pts[0] = col + row * (cols);
      pts[1] = pts[0] + 1;
      pts[2] = pts[0] + cols + 1;
      pts[3] = pts[0] + cols;
      polys->InsertNextCell(4, pts);
    }
  }
  surface->SetPoints(points);
  surface->SetPolys(polys);

  return surface;
}

vtkSmartPointer<vtkPolyData> ShiftMasterSlice(vtkPolyData *original_surface, int index, std::vector<float> dir)
{
  float _dir[3] = {dir[0], dir[1], dir[2]}; // TODO change this hack
  vtkMath::MultiplyScalar(_dir, float(index));

  // Set up the transform filter

  vtkSmartPointer<vtkTransform> translation =
      vtkSmartPointer<vtkTransform>::New();
  translation->Translate(_dir);

  vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter =
      vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  transformFilter->SetInputData(original_surface);
  transformFilter->SetTransform(translation);
  transformFilter->Update();

  return transformFilter->GetOutput();
}

std::map<int, vtkSmartPointer<vtkPolyData>> CreateStack(vtkPolyData *master_slice, int n_slices, std::vector<float> direction, float dist_slices)
{
  std::map<int, vtkSmartPointer<vtkPolyData>> stack;

  time_t time_0;
  time(&time_0);

  vtkMath::MultiplyScalar(direction.data(), dist_slices);

  for (int s = 0; s < n_slices; s++)
  {
    // std::cout << "shifting slice " << s << std::endl;
    stack[s] = ShiftMasterSlice(master_slice, s, direction);
  }

  time_t time_1;
  time(&time_1);

  std::cout << "CreateStack tooks : " << difftime(time_1, time_0) << " [s]" << std::endl;

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

std::vector<int> GetPixelValues(vtkDataSet *dataset)
{
  std::vector<int> values;

  for (int i = 0; i < dataset->GetNumberOfPoints(); i++)
  {
    values.push_back(dataset->GetPointData()->GetArray("ImageFile")->GetTuple(i)[0]);
  }

  std::cout << "array filled with " << values.size() << " elements. " << std::endl;

  return values;
}

void GetMetadata(vtkImageData *image)
{
  // Store the image and print some infos
  std::cout << "ORIGIN: "
            << image->GetOrigin()[0] << ", " << image->GetOrigin()[1] << ", " << image->GetOrigin()[2] << std::endl
            << "DIMENSION: "
            << image->GetDimensions()[0] << ", " << image->GetDimensions()[1] << ", " << image->GetDimensions()[2] << std::endl
            << "BOUNDS: "
            << image->GetBounds()[0] << ", " << image->GetBounds()[1] << ", " << image->GetBounds()[2] << std::endl
            << image->GetBounds()[0] << ", " << image->GetBounds()[1] << ", " << image->GetBounds()[2] << std::endl;
}

bool test_alg(vtkPolyData *plane, vtkDataSet *curved_surf)
{
  int vp, vc;
  for (int i = 0; i < plane->GetNumberOfPoints(); i++)
  {
    vp = plane->GetPointData()->GetArray("ImageFile")->GetTuple(i)[0];
    vc = curved_surf->GetPointData()->GetArray("ImageFile")->GetTuple(i)[0];
    if (vp != vc)
    {
      return false;
    }
  }
  return true;
}