## CRYSTAL's IMPLEMENTATION

Extra credits:

- I implement the option for changing between cylinder drawings and line drawing mode for lsystem node
- I implement the random seed which can change the shape the lsystem based on some random behaviour
- I implement the random scale from -1 to 1 which has similar behaviour as the original lsystem in houdini, to have some changes in both rotation of the tree as well as the branch size

# CMake Build Instructions

This is an alternative way to build the project. If you don't want to use CMake, you can just follow the steps in the write-up.

## Preparation for building the project

### Set up Houdini environment variables

You need to set two environment variables:
HOUDINI_DSO_PATH
%CUSTOM_DSO_PATH%;&

CUSTOM_DSO_PATH
D:\Documents\houdini\dso

For CUSTOM_DSO_PATH, you can set it to any directory you want.

### Set up variables in CMakelists.txt

You need to set `HOUDINI_INSTALL_PATH` to the path where Houdini is installed. For example, if Houdini is installed in `D:/Program Files/Side Effects Software/Houdini 19.5.368`, then you should set `HOUDINI_INSTALL_PATH` to `D:/Program Files/Side Effects Software/Houdini 19.5.368`.

## Building the project using CMake and loading the plugin in Houdini

After setting these two environment variables, just use cmake to build the project. After building, Follow the steps under "Loading your Houdini Plugin" in the write-up to load the plugin in Houdini.

