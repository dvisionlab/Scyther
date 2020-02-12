
// Create a spline from an array of xyz points
vtkSmartPointer<vtkPolyData> CreateSpline(std::vector<float> seeds, int resolution, double origin[3], double normal[3], bool project)
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
  int n = 0;

  // create point list from seeds (project onto the bb plane if requested)
  for (auto i = seeds.begin(); i != seeds.end(); i += 3)
  {
    // std::cout << *i << " " << *(i+1) << " " << *(i+2) << std::endl;
    p[0] = *i;
    p[1] = *(i + 1);
    p[2] = *(i + 2);
    if (project)
    {
      plane->ProjectPoint(p, origin, normal, projected);
      points->InsertNextPoint(projected);
      n++;
    }
    else
    {
      points->InsertNextPoint(p);
    }
  }

  // Init a polyline
  vtkSmartPointer<vtkPolyLine> polyLine =
      vtkSmartPointer<vtkPolyLine>::New();
  polyLine->GetPointIds()->SetNumberOfIds(seeds.size() / 3);
  for (unsigned int i = 0; i < seeds.size() / 3; i++)
  {
    polyLine->GetPointIds()->SetId(i, i);
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

  return polyData;

  // // Sample polyline to spline REMOVED to keep number of spline pts fixed
  // vtkSmartPointer<vtkSplineFilter> spline = vtkSmartPointer<vtkSplineFilter>::New();
  // spline->SetInputData(polyData);
  // spline->SetSubdivideToSpecified();
  // spline->SetNumberOfSubdivisions(resolution - 1); // n subdivisions = n+1 points
  // spline->Update();

  // return spline->GetOutput();
}

// Extrude a spline to create a curved plane
// vtkSmartPointer<vtkPolyData> SweepLine(vtkPolyData *line, double direction[3], double distance, unsigned int cols)
vtkSmartPointer<vtkPolyData> SweepLine(vtkPolyData *line, std::vector<float> directions, double distance, int cols)
{
  unsigned int rows = line->GetNumberOfPoints();
  double spacing = distance / cols;
  spacing = 1.0;
  cols = 100;

  std::cout
      << "rows, cols: " << rows << ", " << cols << std::endl;

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

  double x[3], direction[3];
  unsigned int cnt = 0;
  for (unsigned int row = 0; row < rows; row++)
  {
    for (int col = -cols / 2; col < cols / 2; col++)
    {
      double p[3];
      line->GetPoint(row, p);
      direction[0] = directions[row * 3];
      direction[1] = directions[row * 3 + 1];
      direction[2] = directions[row * 3 + 2];
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

  vtkSmartPointer<vtkTriangleFilter> triangleFilter =
      vtkSmartPointer<vtkTriangleFilter>::New();
  triangleFilter->SetInputData(surface);
  triangleFilter->Update();

  // TODO test with c++ interface (not pybind)
  // vtkSmartPointer<vtkvmtkPolyDataKiteRemovalFilter> kiteRemovalFilter = vtkSmartPointer<vtkvmtkPolyDataKiteRemovalFilter>::New();
  // kiteRemovalFilter->SetInputConnection(triangleFilter->GetOutputPort());
  // kiteRemovalFilter->SetSizeFactor(2);
  // kiteRemovalFilter->Update();

  vtkSmartPointer<vtkSmoothPolyDataFilter> smoothFilter =
      vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
  smoothFilter->SetInputData(triangleFilter->GetOutput());
  smoothFilter->SetNumberOfIterations(1000); // opt param
  smoothFilter->SetRelaxationFactor(0.1);    // opt param
  smoothFilter->FeatureEdgeSmoothingOff();
  smoothFilter->BoundarySmoothingOn();
  smoothFilter->Update();

  return smoothFilter->GetOutput();
}

// Extrude a spline to create a curved plane
vtkSmartPointer<vtkPolyData> SweepLineFixedDirection(vtkPolyData *line, double direction[3], double distance, unsigned int cols)
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

void GetIOPIPP(vtkSmartPointer<vtkPlaneSource> slice, double iop[6], double ipp[3])
{
  double p0[3], p1[3], p2[3], v01[3], v02[3];

  slice->GetOrigin(ipp);
  slice->GetPoint1(p1);
  slice->GetPoint2(p2);

  // std::cout << slice->GetOrigin()[0] << ", "
  //           << slice->GetOrigin()[1] << ", "
  //           << slice->GetOrigin()[2] << std::endl;
  // std::cout << slice->GetPoint1()[0] << ", "
  //           << slice->GetPoint1()[1] << ", "
  //           << slice->GetPoint1()[2] << std::endl;
  // std::cout << slice->GetPoint2()[0] << ", "
  //           << slice->GetPoint2()[1] << ", "
  //           << slice->GetPoint2()[2] << std::endl;

  vtkMath::Subtract(p1, ipp, v01);
  vtkMath::Normalize(v01);
  vtkMath::Subtract(p2, ipp, v02);
  vtkMath::Normalize(v02);

  // std::cout << v01[0] << ", "
  //           << v01[1] << ", "
  //           << v01[2] << std::endl;

  // std::cout << v02[0] << ", "
  //           << v02[1] << ", "
  //           << v02[2] << std::endl;

  // std::cout << " --- " << std::endl;

  iop[0] = v01[0];
  iop[1] = v01[1];
  iop[2] = v01[2];
  iop[3] = v02[0];
  iop[4] = v02[1];
  iop[5] = v02[2];

  return;
}

vtkSmartPointer<vtkPolyData> GetOrientedPlane(double origin[3], double normal[3], float side_length, int resolution, std::vector<float> &iop_axial, std::vector<float> &ipp_axial)
{

  double normal_z[3] = {0, 0, normal[2]};
  vtkMath::Normalize(normal_z);
  double normal_yz[3] = {0, normal[1], normal[2]};
  vtkMath::Normalize(normal_yz);
  double normal_xyz[3] = {normal[0], normal[1], normal[2]};
  vtkMath::Normalize(normal_xyz);

  vtkSmartPointer<vtkPlaneSource>
      targetPlane = vtkSmartPointer<vtkPlaneSource>::New();
  targetPlane->SetOrigin(0.0, 0.0, 0.0);
  targetPlane->SetPoint1(side_length, 0.0, 0.0);
  targetPlane->SetPoint2(0.0, side_length, 0.0);
  targetPlane->SetNormal(normal_z);
  targetPlane->SetNormal(normal_yz);
  targetPlane->SetNormal(normal_xyz);
  targetPlane->SetCenter(origin);
  targetPlane->SetXResolution(resolution);
  targetPlane->SetYResolution(resolution);
  targetPlane->Update();

  double iop[6];
  double ipp[3];
  GetIOPIPP(targetPlane, iop, ipp);

  iop_axial.push_back(iop[0]);
  iop_axial.push_back(iop[1]);
  iop_axial.push_back(iop[2]);
  iop_axial.push_back(iop[3]);
  iop_axial.push_back(iop[4]);
  iop_axial.push_back(iop[5]);

  ipp_axial.push_back(ipp[0]);
  ipp_axial.push_back(ipp[1]);
  ipp_axial.push_back(ipp[2]);

  return targetPlane->GetOutput();
}

double GetMeanDistanceBtwPoints(vtkSmartPointer<vtkPolyData> spline)
{
  double p1[3];
  double p2[3];
  double sum = 0;
  int numberOfSegments = spline->GetNumberOfPoints() - 1;

  for (int i = 0; i < numberOfSegments; i++)
  {
    spline->GetPoint(i, p1);
    spline->GetPoint(i + 1, p2);
    // std::cout << "dist " << sqrt(vtkMath::Distance2BetweenPoints(p1, p2)) << std::endl;
    sum += sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
  }

  std::cout << ">>> numberOfSegments  " << numberOfSegments << std::endl;
  std::cout << ">>> sum  " << sum << std::endl;
  std::cout << ">>> mean  " << sum / double(numberOfSegments) << std::endl;

  return sum / double(numberOfSegments);
}