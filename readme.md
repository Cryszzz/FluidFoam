# FluidFoam

This is the Houdini Plugin tool that is based on paper [Turbulent Micropolar SPH fluid with Foam](https://animation.rwth-aachen.de/media/papers/2018-TVCG-MPSPH.pdf). This tool brings realistic fluid and foam generation capabilities to Houdini, filling gaps in existing functionality.

### Rendered Results

<video width="320" height="240" controls>
  <source src="https://github.com/Cryszzz/FluidFoam/edit/master/img_videos/fluidTypes.mp4" type="video/mp4">
</video>

<video src='img_videos/fluidTypes.mp4' width=180/>
  
### Motivation

- Realistic Fluid-Foam Simulation: Achieving realistic fluid-foam interactions is challenging, especially within the confines of Houdini’s current toolset.
- Lack of SPH Fluid Solver in Houdini: Houdini does not have a built-in Smoothed Particle Hydrodynamics (SPH) fluid solver, which is essential for simulating detailed fluid dynamics.
- No Native Foam Particle Generation: Houdini’s native tools do not support foam particle generation, which is crucial for realistic water simulation.

### Workflow

![workflow](/img_videos/workflow.png)


There are in total five nodes can be utilized to do the simulation, see [detailed instructions](./instructions.md). 

### Result
![Foam Result](/img_videos/foamResult.png)

This is result differences between different types of foam generated from fluid. 

