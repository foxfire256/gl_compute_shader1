# OpenGL Compute Shader 1

This is a basic test of an OpenGL compute shader. This program sort of works but the problem may not be suited to this solution. A inner loop (acceleration calc) needs to access to the entire data set.

## Status

This program mostly seems to work. There are some weird errors at times. A debug build works ok but the release build is really flaky. The delta_t value might be too small for a 32 bit float. 

Setting the object count (obj_count) greater than the work group size causes nothing to be displayed. The shader may not be able to access memory outside of the local_size_x (work group size).


