
// Create a spline from an array of xyz points
vtkSmartPointer<vtkPolyData> CreateSpline(std::vector<float> seeds, int resolution, double origin[3], double normal[3])
{
  // Create a vtkPoints object and store the points in it, projecting them on the boundary plane
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();

  vtkSmartPointer<vtkPlane> plane = 
      vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(origin);
  plane->SetNormal(normal);
  
  double p[3];
  double projected[3];  

  for ( auto i = seeds.begin(); i != seeds.end(); i+=3 ) {
    std::cout << *i << " " << *(i+1) << " " << *(i+2) << std::endl;

    p[0]= *i;
    p[1]= *(i+1);
    p[2]= *(i+2);
    plane->ProjectPoint(p, origin, normal, projected);
    // points->InsertNextPoint(*i, *(i+1), *(i+2));

    std::cout << "SRC: " << p[0] << ", " << p[1] << ", " << p[2] << std::endl;
    std::cout << "PRJ: " << projected[0] << ", " << projected[1] << ", " << projected[2] << std::endl;

    points->InsertNextPoint(projected);
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
