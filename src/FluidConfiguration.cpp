#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <GU/GU_PrimTube.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include "FluidConfiguration.h"
#include "SPlisHSPlasH/XSPH.h"

using namespace HDK_Sample;
namespace fs = std::filesystem;

//PUT YOUR CODE HERE
//You need to declare your parameters here
//Example to declare a variable for angle you can do like this :
//static PRM_Name		angleName("angle", "Angle");

static PRM_Name tabName("fluid");

static PRM_Default  tabList[] = {
	PRM_Default(14, "configuration"),        // 1 is number of parameters in tab
	PRM_Default(14, "materials"),
	PRM_Default(0, "Other")
};

static PRM_Name ConfigurationNames[] = {
    PRM_Name("timeStepSize", "Initial Time Step Size"),
	PRM_Name("stopAt", "Stop At Time"),
    PRM_Name("particleRadius", "Particle Radius"),
    PRM_Name("enableZSort", "Enable Z-Sort"),
    PRM_Name("gravitation", "Gravitation"),
    PRM_Name("maxIterations", "Max Iterations"),
    PRM_Name("maxError", "Max Error"),
    PRM_Name("enableDivergenceSolver", "Enable Divergence Solver"),
    PRM_Name("maxIterationsV", "Max Iterations of Divergence Solver"),
    PRM_Name("maxErrorV", "Max Divergence Error"),
    PRM_Name("cflMethod", "CFL Method"),
    PRM_Name("cflFactor", "CFL Factor"),
    PRM_Name("cflMinTimeStepSize", "CFL Min Time Step Size"),
    PRM_Name("cflMaxTimeStepSize", "CFL Max Time Step Size"),
    PRM_Name(0) // Sentinel to mark the end of the array
};

static PRM_Default ConfigurationDefaults[] = {
    PRM_Default(0.001f),        // Default initial time step size
	PRM_Default(10.0f),
    PRM_Default(0.025f),        // Default radius of the particles
    PRM_Default(true),          // Default state for enabling Z-Sort
    PRM_Default(0),   // Default gravitational acceleration vector
    PRM_Default(100),           // Default maximal number of iterations for the pressure solver
    PRM_Default(0.01f),         // Default maximal density error in percent for the pressure solver
    PRM_Default(true),          // Default state for enabling the divergence solver
    PRM_Default(50),            // Default maximal number of iterations for the divergence solver
    PRM_Default(0.01f),         // Default maximal divergence error in percent
    PRM_Default(1),             // Default CFL method (using CFL condition)
    PRM_Default(1.0f),          // Default CFL factor
    PRM_Default(0.0001f),       // Default minimum allowed time step size
    PRM_Default(0.005f)         // Default maximum allowed time step size
};

static PRM_Default gravitationDefaults[] = { PRM_Default(0), PRM_Default(-9.81f), PRM_Default(0) };
static PRM_Default initialVelocityDefaults[] = { PRM_Default(0), PRM_Default(0.0f), PRM_Default(0.0) };
static PRM_Default initialAngularVelocityDefaults[] = { PRM_Default(0), PRM_Default(0.0f), PRM_Default(0) };

static PRM_Name MaterialsName[] = {
    PRM_Name("viscosityMethod", "Viscosity Method"),  // Method for viscosity computation
    PRM_Name("viscosity", "Viscosity"),               // Coefficient for the viscosity force computation
    PRM_Name("viscoMaxIter", "Max Viscosity Iterations"),  // Maximum iterations for viscosity solver
    PRM_Name("viscoMaxError", "Max Viscosity Error"),      // Maximum error allowed in the viscosity solver
    PRM_Name("viscoMaxIterOmega", "Max Iterations for Vorticity Diffusion"),  // Max iterations for vorticity diffusion solver
    PRM_Name("viscoMaxErrorOmega", "Max Error for Vorticity Diffusion"),      // Max error for vorticity diffusion solver
    PRM_Name("viscosityBoundary", "Boundary Viscosity"),  // Viscosity computation coefficient at the boundary
	PRM_Name("vorticityMethod", "Vorticity Method"),  // Method for vorticity computation
    PRM_Name("vorticity", "Vorticity"),               // Coefficient for vorticity force computation
    PRM_Name("viscosityOmega", "Viscosity Omega"),    // Viscosity coefficient for the angular velocity (Micropolar model)
    PRM_Name("inertiaInverse", "Inertia Inverse"),  
	PRM_Name("density", "Density"), 
	PRM_Name("initialVelocity", "Initial Velocity"), 
	PRM_Name("initialAngularVelocity", "Initial Angular Velocity"), 
    PRM_Name(0)                                            // End of array marker
};

static PRM_Default MaterialsDefaults[] = {
    PRM_Default(1),                 // Default viscosity calculation method
    PRM_Default(0.01f),             // Default viscosity value for the material
    PRM_Default(100),               // Default maximum iterations for viscosity solver
    PRM_Default(0.001f),            // Default maximum error for viscosity solver
    PRM_Default(50),                // Default maximum iterations for vorticity diffusion solver
    PRM_Default(0.0001f),           // Default maximum error for vorticity diffusion solver
    PRM_Default(0.05f),             // Default viscosity force computation coefficient at the boundary
    PRM_Default(0),                 // Default vorticity method
    PRM_Default(0.1f),              // Default coefficient for vorticity force computation
    PRM_Default(0.01f),             // Default viscosity coefficient for the angular velocity field (Micropolar model)
    PRM_Default(1.0f),             // Default inverse microinertia for the Micropolar model
	PRM_Default(1000.0f) 
};

static PRM_Name jsonUpdateName("json_update", "Update JSON");
static PRM_Default jsonUpdateDefault(0);
static PRM_Name inputPathName("Fluid_obj_path", "Fluid File Path");
static PRM_Default inputPathDefault(0, "");

PRM_Template
SOP_FLUIDCONFIGURATION::myTemplateList[] = {
// PUT YOUR CODE HERE
// You now need to fill this template with your parameter name and their default value
// EXAMPLE : For the angle parameter this is how you should add into the template
// PRM_Template(PRM_FLT,	PRM_Template::PRM_EXPORT_MIN, 1, &angleName, &angleDefault, 0),
// Similarly add all the other parameters in the template format here
	// PRM_Template(PRM_TOGGLE, 1, &jsonUpdateName, &jsonUpdateDefault),
	PRM_Template(
        PRM_FILE,                             // Parameter type for directories
        1,                                   // Number of elements (1 for a single directory path)
        &inputPathName,                   // Parameter name
        &inputPathDefault                 // Default value
    ),
	PRM_Template(PRM_SWITCHER, 2, &tabName, tabList),
	// Configuration tab
	// Configuration tab
    PRM_Template(PRM_FLT, 1, &ConfigurationNames[0], &ConfigurationDefaults[0]), // Initial Time Step Size
	PRM_Template(PRM_FLT, 1, &ConfigurationNames[1], &ConfigurationDefaults[1]), 
    PRM_Template(PRM_FLT, 1, &ConfigurationNames[2], &ConfigurationDefaults[2]), // Particle Radius
    PRM_Template(PRM_TOGGLE, 1, &ConfigurationNames[3], &ConfigurationDefaults[3]), // Enable Z-Sort
    PRM_Template(PRM_XYZ, 3, &ConfigurationNames[4], gravitationDefaults), // Gravitation
    PRM_Template(PRM_INT, 1, &ConfigurationNames[5], &ConfigurationDefaults[5]), // Max Iterations
    PRM_Template(PRM_FLT, 1, &ConfigurationNames[6], &ConfigurationDefaults[6]), // Max Error
    PRM_Template(PRM_TOGGLE, 1, &ConfigurationNames[7], &ConfigurationDefaults[7]), // Enable Divergence Solver
    PRM_Template(PRM_INT, 1, &ConfigurationNames[8], &ConfigurationDefaults[8]), // Max Iterations of Divergence Solver
    PRM_Template(PRM_FLT, 1, &ConfigurationNames[9], &ConfigurationDefaults[9]), // Max Divergence Error
    PRM_Template(PRM_ORD, 1, &ConfigurationNames[10], &ConfigurationDefaults[10]), // CFL Method
    PRM_Template(PRM_FLT, 1, &ConfigurationNames[11], &ConfigurationDefaults[11]), // CFL Factor
    PRM_Template(PRM_FLT, 1, &ConfigurationNames[12], &ConfigurationDefaults[12]), // CFL Min Time Step Size
    PRM_Template(PRM_FLT, 1, &ConfigurationNames[13], &ConfigurationDefaults[13]), // CFL Max Time Step Size

    // Materials tab
    PRM_Template(PRM_ORD, 1, &MaterialsName[0], &MaterialsDefaults[0]), // Viscosity Method
    PRM_Template(PRM_FLT, 1, &MaterialsName[1], &MaterialsDefaults[1]), // Viscosity
    PRM_Template(PRM_INT, 1, &MaterialsName[2], &MaterialsDefaults[2]), // Max Viscosity Iterations
    PRM_Template(PRM_FLT, 1, &MaterialsName[3], &MaterialsDefaults[3]), // Max Viscosity Error
    PRM_Template(PRM_INT, 1, &MaterialsName[4], &MaterialsDefaults[4]), // Max Iterations for Vorticity Diffusion
    PRM_Template(PRM_FLT, 1, &MaterialsName[5], &MaterialsDefaults[5]), // Max Error for Vorticity Diffusion
    PRM_Template(PRM_FLT, 1, &MaterialsName[6], &MaterialsDefaults[6]), // Boundary Viscosity
    PRM_Template(PRM_ORD, 1, &MaterialsName[7], &MaterialsDefaults[7]), // Vorticity Method
    PRM_Template(PRM_FLT, 1, &MaterialsName[8], &MaterialsDefaults[8]), // Vorticity
    PRM_Template(PRM_FLT, 1, &MaterialsName[9], &MaterialsDefaults[9]), // Viscosity Omega
    PRM_Template(PRM_FLT, 1, &MaterialsName[10], &MaterialsDefaults[10]), // Inertia Inverse
	PRM_Template(PRM_FLT, 1, &MaterialsName[11], &MaterialsDefaults[11]), // Inertia Inverse
	PRM_Template(PRM_XYZ, 3, &MaterialsName[12], initialVelocityDefaults), 
	PRM_Template(PRM_XYZ, 3, &MaterialsName[13], initialAngularVelocityDefaults), 

	PRM_Template() // Sentinel
};


// Here's how we define local variables for the SOP.
enum {
	VAR_PT,		// Point number of the star
	VAR_NPT		// Number of points in the star
};

CH_LocalVariable
SOP_FLUIDCONFIGURATION:: myVariables[] = {
    { "PT",	VAR_PT, 0 },		// The table provides a mapping
    { "NPT",	VAR_NPT, 0 },		// from text string to integer token
    { 0, 0, 0 },
};

bool
SOP_FLUIDCONFIGURATION:: evalVariableValue(fpreal &val, int index, int thread)
{
    // myCurrPoint will be negative when we're not cooking so only try to
    // handle the local variables when we have a valid myCurrPoint index.
    if (myCurrPoint >= 0)
    {
	// Note that "gdp" may be null here, so we do the safe thing
	// and cache values we are interested in.
	switch (index)
	{
	    case VAR_PT:
		val = (fpreal) myCurrPoint;
		return true;
	    case VAR_NPT:
		val = (fpreal) myTotalPoints;
		return true;
	    default:
		/* do nothing */;
	}
    }
    // Not one of our variables, must delegate to the base class.
    return SOP_Node::evalVariableValue(val, index, thread);
}

OP_Node *
SOP_FLUIDCONFIGURATION::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_FLUIDCONFIGURATION(net, name, op);
}

SOP_FLUIDCONFIGURATION::SOP_FLUIDCONFIGURATION(OP_Network *net, const char *name, OP_Operator *op)
	: SOP_Node(net, name, op)
{
    myCurrPoint = -1;	// To prevent garbage values from being returned
}

SOP_FLUIDCONFIGURATION::~SOP_FLUIDCONFIGURATION() {}

unsigned
SOP_FLUIDCONFIGURATION::disableParms()
{
    return 0;
}

OP_ERROR
SOP_FLUIDCONFIGURATION::cookMySop(OP_Context &context)
{
	fpreal	now = context.getTime();
	float initialTimeStepSize = evalFloat(ConfigurationNames[0].getToken(), 0, now);
	float stopAt = evalFloat(ConfigurationNames[1].getToken(), 0, now);
	float particleRadius = evalFloat(ConfigurationNames[2].getToken(), 0, now);
	bool enableZSort = evalInt(ConfigurationNames[3].getToken(), 0, now) != 0;
	float gravitationX = evalFloat(ConfigurationNames[4].getToken(), 0, now);
	float gravitationY = evalFloat(ConfigurationNames[4].getToken(), 1, now);
	float gravitationZ = evalFloat(ConfigurationNames[4].getToken(), 2, now);
	UT_Vector3 gravitation(gravitationX, gravitationY, gravitationZ);
	int maxIterations = evalInt(ConfigurationNames[5].getToken(), 0, now);
	float maxError = evalFloat(ConfigurationNames[6].getToken(), 0, now);
	bool enableDivergenceSolver = evalInt(ConfigurationNames[7].getToken(), 0, now) != 0;
	int maxIterationsDivergenceSolver = evalInt(ConfigurationNames[8].getToken(), 0, now);
	float maxErrorDivergence = evalFloat(ConfigurationNames[9].getToken(), 0, now);
	int cflMethod = evalInt(ConfigurationNames[10].getToken(), 0, now);
	float cflFactor = evalFloat(ConfigurationNames[11].getToken(), 0, now);
	float cflMinTimeStepSize = evalFloat(ConfigurationNames[12].getToken(), 0, now);
	float cflMaxTimeStepSize = evalFloat(ConfigurationNames[13].getToken(), 0, now);

	int viscosityMethod = evalInt(MaterialsName[0].getToken(), 0, now);
	float viscosity = evalFloat(MaterialsName[1].getToken(), 0, now);
	int maxViscosityIterations = evalInt(MaterialsName[2].getToken(), 0, now);
	float maxViscosityError = evalFloat(MaterialsName[3].getToken(), 0, now);
	int maxIterationsVorticityDiffusion = evalInt(MaterialsName[4].getToken(), 0, now);
	float maxErrorVorticityDiffusion = evalFloat(MaterialsName[5].getToken(), 0, now);
	float boundaryViscosity = evalFloat(MaterialsName[6].getToken(), 0, now);
	int vorticityMethod = evalInt(MaterialsName[7].getToken(), 0, now);
	float vorticity = evalFloat(MaterialsName[8].getToken(), 0, now);
	float viscosityOmega = evalFloat(MaterialsName[9].getToken(), 0, now);
	float inertiaInverse = evalFloat(MaterialsName[10].getToken(), 0, now);
	float density = evalFloat(MaterialsName[11].getToken(), 0, now);
	float initialVelocityX = evalFloat(MaterialsName[12].getToken(), 0, now);
	float initialVelocityY = evalFloat(MaterialsName[12].getToken(), 1, now);
	float initialVelocityZ = evalFloat(MaterialsName[12].getToken(), 2, now);
	UT_Vector3 initialVelocity(initialVelocityX, initialVelocityY, initialVelocityZ);
	float initialAngularVelocityX = evalFloat(MaterialsName[13].getToken(), 0, now);
	float initialAngularVelocityY = evalFloat(MaterialsName[13].getToken(), 1, now);
	float initialAngularVelocityZ = evalFloat(MaterialsName[13].getToken(), 2, now);
	UT_Vector3 initialAngularVelocity(initialAngularVelocityX, initialAngularVelocityY, initialAngularVelocityZ);
	// Now that you have all the branches ,which is the start and end point of each point ,its time to render 
	// these branches into Houdini 
	UT_String fluidFilePath;
	evalString(fluidFilePath,inputPathName.getToken(), 0, now);
	// PUT YOUR CODE HERE
	// Declare all the necessary variables for drawing cylinders for each branch 
    float		 rad, tx, ty, tz;
    int			 divisions, plane;
    int			 xcoord =0, ycoord = 1, zcoord =2;
    float		 tmp;
    UT_Vector4		 pos;
    GU_PrimPoly		*poly;
    int			 i;
    UT_Interrupt	*boss;

    // Since we don't have inputs, we don't need to lock them.

    divisions  = 5;	// We need twice our divisions of points
    myTotalPoints = divisions;		// Set the NPT local variable value
    myCurrPoint   = 0;			// Initialize the PT local variable



    // Check to see that there hasn't been a critical error in cooking the SOP.
    if (error() < UT_ERROR_ABORT)
    {
	boss = UTgetInterrupt();
	gdp->clearAndDestroy();

	// Start the interrupt server
	if (boss->opStart("Building LSYSTEM"))
	{
        // PUT YOUR CODE HERE
	    // Build a polygon
	    // You need to build your cylinders inside Houdini from here
		// TIPS:
		// Use GU_PrimPoly poly = GU_PrimPoly::build(see what values it can take)
		// Also use GA_Offset ptoff = poly->getPointOffset()
		// and gdp->setPos3(ptoff,YOUR_POSITION_VECTOR) to build geometry.

		// Iterate over each parameter
		// Iterate over each parameter and set its value to the corresponding detail attribute
		// Iterate over each parameter and set its value to the corresponding detail attribute
		for (int i = 0; i < sizeof(ConfigurationNames) / sizeof(ConfigurationNames[0]); ++i) {
			UT_String paramName(ConfigurationNames[i].getToken());
			GA_RWHandleS attrib(gdp->findStringTuple(GA_ATTRIB_DETAIL, paramName));

			if (!attrib.isValid()) {
				attrib = GA_RWHandleS(gdp->addStringTuple(GA_ATTRIB_DETAIL, paramName, 1));
			}

			if (attrib.isValid()) {
				std::string paramValue;
				switch (i) {
					case 0: // initialTimeStepSize
						paramValue = std::to_string(initialTimeStepSize);
						break;
					case 1: // particleRadius
						paramValue = std::to_string(stopAt);
						break;
					case 2: // particleRadius
						paramValue = std::to_string(particleRadius);
						break;
					case 3: // enableZSort
						paramValue = enableZSort ? "1" : "0";
						break;
					case 4: // gravitation
						paramValue = std::to_string(gravitationX) + "," + std::to_string(gravitationY) + "," + std::to_string(gravitationZ);
						break;
					case 5: // maxIterations
						paramValue = std::to_string(maxIterations);
						break;
					case 6: // maxError
						paramValue = std::to_string(maxError);
						break;
					case 7: // enableDivergenceSolver
						paramValue = enableDivergenceSolver ? "1" : "0";
						break;
					case 8: // maxIterationsDivergenceSolver
						paramValue = std::to_string(maxIterationsDivergenceSolver);
						break;
					case 9: // maxErrorDivergence
						paramValue = std::to_string(maxErrorDivergence);
						break;
					case 10: // cflMethod
						paramValue = std::to_string(cflMethod);
						break;
					case 11: // cflFactor
						paramValue = std::to_string(cflFactor);
						break;
					case 12: // cflMinTimeStepSize
						paramValue = std::to_string(cflMinTimeStepSize);
						break;
					case 13: // cflMaxTimeStepSize
						paramValue = std::to_string(cflMaxTimeStepSize);
						break;
					default:
						break;
				}
				UT_String outValue(paramValue.c_str());
				attrib.set(GA_Offset(0), outValue);
				attrib.bumpDataId();
			}
		}

		// Repeat the same process for parameters in MaterialsName
		for (int i = 0; i < sizeof(MaterialsName) / sizeof(MaterialsName[0]); ++i) {
			UT_String paramName(MaterialsName[i].getToken());
			GA_RWHandleS attrib(gdp->findStringTuple(GA_ATTRIB_DETAIL, paramName));

			if (!attrib.isValid()) {
				attrib = GA_RWHandleS(gdp->addStringTuple(GA_ATTRIB_DETAIL, paramName, 1));
			}

			if (attrib.isValid()) {
				std::string paramValue;
				switch (i) {
					case 0: // viscosityMethod
						paramValue = std::to_string(viscosityMethod);
						break;
					case 1: // viscosity
						paramValue = std::to_string(viscosity);
						break;
					case 2: // maxViscosityIterations
						paramValue = std::to_string(maxViscosityIterations);
						break;
					case 3: // maxViscosityError
						paramValue = std::to_string(maxViscosityError);
						break;
					case 4: // maxIterationsVorticityDiffusion
						paramValue = std::to_string(maxIterationsVorticityDiffusion);
						break;
					case 5: // maxErrorVorticityDiffusion
						paramValue = std::to_string(maxErrorVorticityDiffusion);
						break;
					case 6: // boundaryViscosity
						paramValue = std::to_string(boundaryViscosity);
						break;
					case 7: // vorticityMethod
						paramValue = std::to_string(vorticityMethod);
						break;
					case 8: // vorticity
						paramValue = std::to_string(vorticity);
						break;
					case 9: // viscosityOmega
						paramValue = std::to_string(viscosityOmega);
						break;
					case 10: // inertiaInverse
						paramValue = std::to_string(inertiaInverse);
						break;
					case 11: // inertiaInverse
						paramValue = std::to_string(density);
						break;
					case 12: // inertiaInverse
						paramValue = std::to_string(initialVelocityX) + "," + std::to_string(initialVelocityY) + "," + std::to_string(initialVelocityZ);
						break;
					case 13: // inertiaInverse
						paramValue = std::to_string(initialAngularVelocityX) + "," + std::to_string(initialAngularVelocityY) + "," + std::to_string(initialAngularVelocityZ);
						break;
					default:
						break;
				}
				UT_String outValue(paramValue.c_str());
				attrib.set(GA_Offset(0), outValue);
				attrib.bumpDataId();
			}

			
		}
		UT_String paramName(inputPathName.getToken());
		GA_RWHandleS attrib(gdp->findStringTuple(GA_ATTRIB_DETAIL, paramName));
		if (!attrib.isValid()) {
			attrib = GA_RWHandleS(gdp->addStringTuple(GA_ATTRIB_DETAIL, paramName, 1));
		}
		std::string paramValue=fluidFilePath.toStdString();
		UT_String outValue(paramValue.c_str());
		attrib.set(GA_Offset(0), outValue);
		attrib.bumpDataId();



		////////////////////////////////////////////////////////////////////////////////////////////

	    // Highlight the star which we have just generated.  This routine
	    // call clears any currently highlighted geometry, and then it
	    // highlights every primitive for this SOP. 
	    select(GU_SPrimitive);
	}

	// Tell the interrupt server that we've completed. Must do this
	// regardless of what opStart() returns.
	boss->opEnd();
    }

    myCurrPoint = -1;
    return error();
}