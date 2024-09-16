# FluidFoam

This is the Houdini Plugin tool that is based on paper [Turbulent Micropolar SPH fluid with Foam](https://animation.rwth-aachen.de/media/papers/2018-TVCG-MPSPH.pdf). This tool brings realistic fluid and foam generation capabilities to Houdini, filling gaps in existing functionality.

### Rendered Results

https://github.com/user-attachments/assets/6898d2be-3e87-4b22-819d-47f92c014ed7


### Particles-view Results

https://github.com/user-attachments/assets/4f55a789-17ab-4b4d-9100-936205754756



  
### Motivation

- Realistic Fluid-Foam Simulation: Achieving realistic fluid-foam interactions is challenging, especially within the confines of Houdini’s current toolset.
- Lack of SPH Fluid Solver in Houdini: Houdini does not have a built-in Smoothed Particle Hydrodynamics (SPH) fluid solver, which is essential for simulating detailed fluid dynamics.
- No Native Foam Particle Generation: Houdini’s native tools do not support foam particle generation, which is crucial for realistic water simulation.

### Workflow

![workflow](/img_videos/workflow.png)


There are in total five nodes can be utilized to do the simulation, see [detailed instructions](./instructions.md). 

### Result
![Foam Result](/img_videos/foamResult.png)

This is a detailed result differences between different types of foam generated from fluid. 

<b>sprays: </b> This is particles that were generated from turbulent fluid dynamics. For example, when fluid was droping from hill, they will spray out some particles.

<b>bubbles: </b> This is particles that were that is close related to air bubbles, the particles that were trapped in fluid particles. 

<b>foams: </b> This is the particles that were most attached to the surface of the fluid, like particles floating on the fluid. 

Basic post-processing step: diffuse particles with less than six fluid neighbors are considered as spray particle. Particles with more than 20 fluid neighbors are classified as air bubbles. In all other cases, particles are
considered to be foam. 

Usage: users should be able to use each types of particles separately for their stylizations once they enable the splittypes. 
