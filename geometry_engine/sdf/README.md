# Signed-distance geometry

SDF generation belongs here. The first implementation is exposed through the
existing CUDA kernels in `rocket_advanced`; future PicoGK adapters should
export the same sign convention and metadata: negative inside solid, positive
outside, SI origin, voxel spacing and material id.