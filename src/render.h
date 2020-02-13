
// Paint pixels on a plane
vtkSmartPointer<vtkPolyData> GetPlanar(vtkDataArray *pixels, vtkPolyData *spline, float slice_dimension)
{
  float dist = GetMeanDistanceBtwPoints(spline);
  int nop = spline->GetNumberOfPoints();

  std::cout << "plane edges " << pixels->GetNumberOfValues() / nop << ", " << dist * nop << std::endl;

  vtkSmartPointer<vtkPlaneSource> targetPlane = vtkSmartPointer<vtkPlaneSource>::New();
  targetPlane->SetOrigin(0.0, 0.0, 0.0);
  targetPlane->SetPoint1(0.0, slice_dimension, 0.0);
  targetPlane->SetPoint2(0.0, 0.0, dist * nop);
  targetPlane->SetXResolution(pixels->GetNumberOfValues() / nop - 1); // this -1 matches with cols++
  targetPlane->SetYResolution(nop);
  targetPlane->Update();

  vtkSmartPointer<vtkPolyData> viewPlane = vtkSmartPointer<vtkPolyData>::New();
  viewPlane = targetPlane->GetOutput();
  viewPlane->GetPointData()->SetScalars(pixels);

  return viewPlane;
}

// Render curved & plane surfaces
int renderAll(vtkPolyData *spline, vtkProbeFilter *sampleVolume, vtkImageData *image, float slice_dimension, float range)
{
  vtkSmartPointer<vtkPolyData> viewPlane = GetPlanar(sampleVolume->GetOutput()->GetPointData()->GetArray("ImageFile"), spline, slice_dimension);

  // Compute a simple window/level based on scalar range
  vtkSmartPointer<vtkWindowLevelLookupTable> wlLut = vtkSmartPointer<vtkWindowLevelLookupTable>::New();
  double level = range / 2.0;

  wlLut->SetWindow(range);
  wlLut->SetLevel(level);

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(spline);

  vtkSmartPointer<vtkDataSetMapper> mapper1 = vtkSmartPointer<vtkDataSetMapper>::New();
  mapper1->SetInputData(viewPlane);
  mapper1->SetLookupTable(wlLut);
  mapper1->SetScalarRange(image->GetScalarRange()[0], image->GetScalarRange()[1]);

  vtkSmartPointer<vtkDataSetMapper> mapper2 = vtkSmartPointer<vtkDataSetMapper>::New();
  mapper2->SetInputConnection(sampleVolume->GetOutputPort());
  mapper2->SetLookupTable(wlLut);
  mapper2->SetScalarRange(image->GetScalarRange()[0], image->GetScalarRange()[1]);

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkActor> actor1 = vtkSmartPointer<vtkActor>::New();
  actor1->SetMapper(mapper1);

  vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();
  actor2->SetMapper(mapper2);

  // Create a renderer, render window, and interactor
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(1600, 1600);
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Add the actors to the scene
  renderer->AddActor(actor);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->SetBackground(.2, .3, .4);

  // Set the camera
  renderer->GetActiveCamera()->SetViewUp(0, 0, 1);
  renderer->GetActiveCamera()->SetPosition(0, 100, 100);
  renderer->GetActiveCamera()->SetFocalPoint(50, 50, 100);
  renderer->ResetCamera();

  // Render and interact
  renderWindow->Render();
  renderWindowInteractor->Start();

  return 0;
}
