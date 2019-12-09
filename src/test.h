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