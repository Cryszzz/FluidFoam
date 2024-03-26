For /houdiniPlugin:
The cmake is OK, but the library path for glew, partio and zlib need to be changed to ../../lib/Released in VS 2022. Also, some codes in Partio library is not compatiable with C++ 17 you may want to change the C++ version to 14 for Partio in VS.

### The generated .dll can be add to the houdini /dso folder but cannot be loaded to houdini! Still trying to figure out why but I am very tired and will solve this when I wake up. The node's registered name supposed to be *MyPartioEmitter*

# FluidFoam
