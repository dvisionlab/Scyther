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
                                                                bool render)
{
    time_t time_0;
    time(&time_0);

    // Parse input
    double direction[3];
    std::copy(dir.begin(), dir.end(), direction);

    std::cout << "\n >> compute_cmpr_straight << \n"
              << std::endl;

    // Print arguments
    std::cout
        << "InputVolume: " << volumeFileName << std::endl
        << "Resolution: " << resolution << std::endl
        << "Seeds: " << seeds.size() / 3 << std::endl
        << "tng: " << tng.size() / 3 << std::endl
        << "ptn: " << ptn.size() / 3 << std::endl
        << "dir: " << dir[0] << ", " << dir[1] << ", " << dir[2] << std::endl
        << "stack_direction: " << stack_direction[0] << ", " << stack_direction[1] << ", " << stack_direction[2] << std::endl
        << "slice_dimension: " << slice_dimension << std::endl
        << "dist_slices: " << dist_slices << std::endl
        << "n_slices: " << n_slices << std::endl
        << "render: " << render << std::endl
        << " == END PARAMS == \n"
        << std::endl;

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

    std::cout << " > CreateSpline OK " << std::endl;

    // Sweep the line to form a surface
    vtkSmartPointer<vtkPolyData> master_slice = SweepLine(original_spline, ptn, slice_dimension, resolution);

    std::cout << " > SweepLine OK " << std::endl;

    // Shift the master slice to create a stack
    std::map<int, vtkSmartPointer<vtkPolyData>> stack_map = CreateStack(master_slice, n_slices, stack_direction, dist_slices);

    std::cout << " > CreateStack OK " << std::endl;

    // Squash stack map into a single polydata
    vtkSmartPointer<vtkPolyData> complete_stack = Squash(stack_map, false);

    std::cout << " > Squash OK " << std::endl;

    // Compute axial stack
    float axial_side_length = 120.0;
    std::vector<float> iop_axial;
    std::vector<float> ipp_axial;
    std::map<int, vtkSmartPointer<vtkPolyData>> axial_stack_map = CreateAxialStack(original_spline, axial_side_length, resolution, iop_axial, ipp_axial);

    // Squash stack map into a single polydata
    vtkSmartPointer<vtkPolyData> complete_axial_stack = Squash(axial_stack_map, false);

    // Probe the volume with the extruded surfaces
    vtkSmartPointer<vtkProbeFilter> sampleVolume = vtkSmartPointer<vtkProbeFilter>::New();
    sampleVolume->SetInputConnection(1, reader->GetOutputPort());
    sampleVolume->SetInputData(0, complete_stack);
    sampleVolume->Update();

    vtkSmartPointer<vtkProbeFilter> sampleVolumeAxial = vtkSmartPointer<vtkProbeFilter>::New();
    sampleVolumeAxial->SetInputConnection(1, reader->GetOutputPort());
    sampleVolumeAxial->SetInputData(0, complete_axial_stack);
    sampleVolumeAxial->Update();

    time_t time_1;
    time(&time_1);

    std::cout << "Total : " << difftime(time_1, time_0) << "[s]" << std::endl;

    // Get values from probe output
    std::vector<float> values_cmpr = GetPixelValues(sampleVolume->GetOutput(), false);
    std::vector<float> values_axial = GetPixelValues(sampleVolumeAxial->GetOutput(), true);

    std::map<std::string, std::vector<float>> response;

    // Compute mean distance btw points to be returned as image spacing
    float mean_pts_distance = GetMeanDistanceBtwPoints(original_spline);
    // float range = GetWindowWidth(reader->GetOutput());
    float range_cmpr = GetWindowWidth(values_cmpr, reader->GetOutput()->GetScalarRange()[1], reader->GetOutput()->GetScalarRange()[0]);
    float range_axial = GetWindowWidth(values_axial, reader->GetOutput()->GetScalarRange()[1], reader->GetOutput()->GetScalarRange()[0]);

#ifdef DYNAMIC_VMTK
    // Render
    if (render)
    {
        int res = renderAll(original_spline, sampleVolume, reader->GetOutput(), slice_dimension, range_cmpr);
    }
#endif

    // Compose response with metadata
    std::vector<float> dimension_cmpr = {
        float(seeds.size() / 3),
        float(resolution),
        GetDimensions(stack_map)[2]};
    std::vector<float>
        dimension_axial = GetDimensions(axial_stack_map);
    std::vector<float> spacing_cmpr = {
        slice_dimension / float(resolution),
        mean_pts_distance};
    std::vector<float> spacing_axial = {
        axial_side_length / resolution,
        axial_side_length / resolution};
    std::vector<float> wwwl_cmpr = {
        range_cmpr,
        range_cmpr / 2};
    std::vector<float> wwwl_axial = {
        range_axial,
        range_axial / 2};

    response["metadata"] = metadata;
    response["pixels_cmpr"] = values_cmpr;
    response["pixels_axial"] = values_axial;
    response["dimension_cmpr"] = dimension_cmpr;
    response["dimension_axial"] = dimension_axial;
    response["spacing_cmpr"] = spacing_cmpr;
    response["spacing_axial"] = spacing_axial;
    response["wwwl_cmpr"] = wwwl_cmpr;
    response["wwwl_axial"] = wwwl_axial;
    response["iop_axial"] = iop_axial;
    response["ipp_axial"] = ipp_axial;

    return response;
}

std::map<std::string, std::vector<float>> compute_cmpr_stretch(std::string volumeFileName,
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
              << "Resolution: " << resolution << std::endl
              << "Seeds: " << seeds.size() << std::endl;

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
    float distance;
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

    vtkSmartPointer<vtkPolyData> master_slice = SweepLineFixedDirection(spline, direction, distance, resolution);

    // Shift the master slice to create a stack
    std::map<int, vtkSmartPointer<vtkPolyData>> stack_map = CreateStack(master_slice, n_slices, stack_direction, dist_slices);

    // Squash stack map into a single polydata
    vtkSmartPointer<vtkPolyData> complete_stack = Squash(stack_map, false);

    // Compute axial stack
    float axial_side_length = 120.0;
    std::vector<float> iop_axial;
    std::vector<float> ipp_axial;
    std::map<int, vtkSmartPointer<vtkPolyData>> axial_stack_map = CreateAxialStack(original_spline, axial_side_length, resolution, iop_axial, ipp_axial);

    // Squash stack map into a single polydata
    vtkSmartPointer<vtkPolyData> complete_axial_stack = Squash(axial_stack_map, false);

    // Probe the volume with the extruded surfaces
    vtkSmartPointer<vtkProbeFilter> sampleVolume = vtkSmartPointer<vtkProbeFilter>::New();
    sampleVolume->SetInputConnection(1, reader->GetOutputPort());
    sampleVolume->SetInputData(0, complete_stack);
    sampleVolume->Update();

    vtkSmartPointer<vtkProbeFilter> sampleVolumeAxial = vtkSmartPointer<vtkProbeFilter>::New();
    sampleVolumeAxial->SetInputConnection(1, reader->GetOutputPort());
    sampleVolumeAxial->SetInputData(0, complete_axial_stack);
    sampleVolumeAxial->Update();

    time_t time_1;
    time(&time_1);

    std::cout << "Total : " << difftime(time_1, time_0) << "[s]" << std::endl;

    // Get values from probe output
    std::vector<float> values_cmpr = GetPixelValues(sampleVolume->GetOutput(), false);
    std::vector<float> values_axial = GetPixelValues(sampleVolumeAxial->GetOutput(), true);

    std::map<std::string, std::vector<float>> response;

    // Compute mean distance btw points to be returned as image spacing
    float mean_pts_distance = GetMeanDistanceBtwPoints(spline);
    float range_cmpr = GetWindowWidth(values_cmpr, reader->GetOutput()->GetScalarRange()[1], reader->GetOutput()->GetScalarRange()[0]);
    float range_axial = GetWindowWidth(values_axial, reader->GetOutput()->GetScalarRange()[1], reader->GetOutput()->GetScalarRange()[0]);

#ifdef DYNAMIC_VMTK
    // Render
    if (render)
    {
        int res = renderAll(original_spline, sampleVolume, reader->GetOutput(), distance, range_cmpr);
    }
#endif

    // Compose response with metadata
    std::vector<float> dimension_cmpr = {
        float(seeds.size() / 3),
        float(resolution),
        GetDimensions(stack_map)[2]};
    std::vector<float>
        dimension_axial = GetDimensions(axial_stack_map);
    std::vector<float> spacing_cmpr = {
        float(distance / resolution),
        mean_pts_distance};
    std::vector<float> spacing_axial = {
        axial_side_length / resolution,
        axial_side_length / resolution};
    std::vector<float> wwwl_cmpr = {
        range_cmpr,
        range_cmpr / 2};
    std::vector<float> wwwl_axial = {
        range_axial,
        range_axial / 2};

    response["metadata"] = metadata;
    response["pixels_cmpr"] = values_cmpr;
    response["pixels_axial"] = values_axial;
    response["dimension_cmpr"] = dimension_cmpr;
    response["dimension_axial"] = dimension_axial;
    response["spacing_cmpr"] = spacing_cmpr;
    response["spacing_axial"] = spacing_axial;
    response["wwwl_cmpr"] = wwwl_cmpr;
    response["wwwl_axial"] = wwwl_axial;
    response["iop_axial"] = iop_axial;
    response["ipp_axial"] = ipp_axial;

    return response;
}