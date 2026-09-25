# Woven and lattice structures

This module is reserved for woven, lattice and gyroid representations used by PicoGK/NanoVDB geometry generation. Keep topology generation separate from material-property models and from AMReX field data.

The first implementation should export a signed-distance field and manufacturing metadata, then verify watertightness, minimum feature size and disconnected components before any thermal or flow coupling.