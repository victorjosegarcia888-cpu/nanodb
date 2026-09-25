# Voxel bridge

This layer converts geometry profiles and SDF fields into NanoVDB/OpenVDB
grids. Every export must retain dimensions, origin, spacing, units, case id and
material labels before it is passed to AMReX embedded boundaries.