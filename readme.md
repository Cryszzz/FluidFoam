# FluidFoam

This is the Houdini Plugin tool that is based on paper [Turbulent Micropolar SPH fluid with Foam](https://animation.rwth-aachen.de/media/papers/2018-TVCG-MPSPH.pdf). This tool brings realistic fluid and foam generation capabilities to Houdini, filling gaps in existing functionality.

### Motivation

- Realistic Fluid-Foam Simulation: Achieving realistic fluid-foam interactions is challenging, especially within the confines of Houdini’s current toolset.
- Lack of SPH Fluid Solver in Houdini: Houdini does not have a built-in Smoothed Particle Hydrodynamics (SPH) fluid solver, which is essential for simulating detailed fluid dynamics.
- No Native Foam Particle Generation: Houdini’s native tools do not support foam particle generation, which is crucial for realistic water simulation.

### Workflow

![workflow](/img_videos/workflow.png)

There are in total five nodes can be utilized to do the simulation, see [detailed instructions](./instructions.md). 


To use our authoring tool, the user should first create a geometry node in Houdini and double click into the subnet work since our custom nodes are SOP nodes and must be placed inside a geometry network. 

Then the user should place a Fluid Configuration Node by searching “FluidConfiguration”, and set the simulation parameters. The geometry file for the shape of the fluid model should be in obj file format. This parameter must be set in order to run the simulation.

Search for the “RigidBody” and add a Rigid Body node. The user should at least set a Rigid Body node as the bounding box of the simulation scene. If the rigid body is to be set as a bounding box, the user must check the “IsWall” flag so the fluid will collide on the inside face of the rigid body geometry. 

Set a Simulation node by searching “FluidSimulator”. Then link the output of the Fluid Configuration to the first input of the Simulation node, and RigidBody nodes to the second input of the Simulation node.

In the simulation node, the user can set a folder for the cached fluid particle data. If not set, the folder will be the directory where the houdini file is stored. To simulate with the configuration, the user should click the “simulation” button. 

Set a Visualizer node by searching “MyVisualizer”. Link the output of the Simulator Node to visualize the particles as points in the scene.

Set a Foam Generator node by searching “FoamGenerator”. Set parameters for the foam simulation and whether split types for foams. Link the simulation node’s output to the input of this node and click the “simulate” button. 

Set a Visualizer node by searching “MyVisualizer”. Link the output of the Foam Generator Node to visualize the particles as points in the scene. If the user chooses to use split types for foams, they should choose which type of foam to visualize by selecting from a drop-down menu.

The users then can use other default nodes in Houdini to apply shaders and rendering techniques to the particle points.

### Result

![Foam Result](/img_videos/foamResult.png)

This is result differences between different types of foam generated from fluid. 