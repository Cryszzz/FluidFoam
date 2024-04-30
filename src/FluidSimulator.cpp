
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_String.h>
#include <UT/UT_Vector3.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <GU/GU_PrimTube.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>


#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <string>
#include "FluidSimulator.h"
#include "SPlisHSPlasH/XSPH.h"
#include "Utilities/Logger.h"
#include "Utilities/Timing.h"
#include "Utilities/Counting.h"


using namespace HDK_Sample;
using namespace Utilities;
namespace fs = std::filesystem;

INIT_COUNTING

//unsigned int Utilities::Timing::m_stopCounter = 0;
//unsigned int Utilities::Timing::m_startCounter = 0;
//bool Utilities::Timing::m_dontPrintTimes = false;
//std::stack<Utilities::TimingHelper> Utilities::Timing::m_timingStack;
//std::unordered_map<int, Utilities::AverageTime> Utilities::Timing::m_averageTimes;
//int Utilities::IDFactory::id = 0;
//namespace Utilities {
//	Logger logger; // Definition of the static logger instance
//}
//std::unordered_map<std::string, AverageCount> Utilities::Counting::m_averageCounts;
//
//PUT YOUR CODE HERE
//You need to declare your parameters here
//Example to declare a variable for angle you can do like this :
//static PRM_Name		angleName("angle", "Angle");

void fluid_printCurrentPath() {
	std::cout << "Current Path: " << fs::current_path() << std::endl;
}
static PRM_Name tabName("fluid");
static PRM_Default  tabList[] = {
	PRM_Default(19, "configuration"),        // 1 is number of parameters in tab
	PRM_Default(3, "materials"),
	PRM_Default(10, "rigidBodies"),
	PRM_Default(5, "fluidBlocks"),
	PRM_Default(0, "Other")
};

static PRM_Name ConfigurationNames[] = {
	PRM_Name("timeStepSize", "Initial Time Step Size"),
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
	PRM_Default(1.0f)               // Default inverse microinertia for the Micropolar model
};

static PRM_Name jsonUpdateName("json_update", "Update JSON");
static PRM_Default jsonUpdateDefault(0);
static PRM_Name inputPathName("Fluid_obj_path", "Fluid File Path");
static PRM_Default inputPathDefault(0, "");


// simulate button parameters
static PRM_Name simulateButtonName("simulate", "Simulate");
static PRM_Default simulateButtonNameDefault(0);

float getStringParamAsFloat(static PRM_Name &name) {
	//OP_Parameters::evalFloat(names[0].getToken(), 0, t)
	return 0.0;
}

/*static*/ int SOP_FUILDSIMULATOR::simulateFluid(void* data, int index, float time, const PRM_Template* tplate) {
	// Handle the button press event here
	std::cout << "Button Pressed" << std::endl;
	SOP_FUILDSIMULATOR* sop = static_cast<SOP_FUILDSIMULATOR*>(data);
	//OP_Context& context = sop->getContext();
	fpreal	now = sop->lastCookTime;

	
	UT_Vector3 translation;
	sop->evalFloats(translationName.getToken(), translation.data(), now);

	UT_Vector3 rotationAxis;
	sop->evalFloats(rotationAxisName.getToken(), rotationAxis.data(), now);

	float rotationAngle = sop->evalFloat(rotationAngleName.getToken(), 0, now);

	UT_Vector3 scale;
	sop->evalFloats(scaleName.getToken(), scale.data(), now);

	UT_Vector4 color;
	sop->evalFloats(colorName.getToken(), color.data(), now); // Assuming RGBA


	// Assuming "isDynamic", "isWall", "mapInvert" are toggles represented as booleans
	bool isDynamic = sop->evalInt(isDynamicName.getToken(), 0, now) != 0;
	bool isWall = sop->evalInt(isWallName.getToken(), 0, now) != 0;
	bool mapInvert = sop->evalInt(mapInvertName.getToken(), 0, now) != 0;

	// Assuming "denseMode" is an integer
	int denseMode = sop->evalInt(denseModeName.getToken(), 0, now);

	// "start", "end", "translationFB", and "scaleFB" are vectors
	UT_Vector3 start, end, translationFB, scaleFB;
	sop->evalFloats(startName.getToken(), start.data(), now);
	sop->evalFloats(endName.getToken(), end.data(), now);
	sop->evalFloats(translationFBName.getToken(), translationFB.data(), now);
	sop->evalFloats(scaleFBName.getToken(), scaleFB.data(), now);

	try
	{
		static unsigned int m_stopCounter;
		//sop->mySim->init(particleRadius, false);
		std::cout << "Simulator initialized." << std::endl;

		// pass parameters to mySim
		//sop->mySim->setParticleRadius(particleRadius);
		

	}
	catch (const std::exception&)
	{
		std::cout << "Simulator initialization failed." << std::endl;

	}
	

	return 1; // Return 1 to indicate that the action was handled
}

PRM_Template
SOP_FUILDSIMULATOR::myTemplateList[] = {
	// PUT YOUR CODE HERE
	// You now need to fill this template with your parameter name and their default value
	// EXAMPLE : For the angle parameter this is how you should add into the template
	// PRM_Template(PRM_FLT,	PRM_Template::PRM_EXPORT_MIN, 1, &angleName, &angleDefault, 0),
	// Similarly add all the other parameters in the template format here

		// simulation button
		PRM_Template(PRM_CALLBACK, 1, &simulateButtonName, &simulateButtonNameDefault, nullptr, nullptr, &simulateFluid, nullptr),
		PRM_Template(PRM_TOGGLE, 1, &jsonUpdateName, &jsonUpdateDefault),
		//PRM_Template(PRM_SWITCHER, 5, &tabName, tabList),
		// Configuration tab
		//PRM_Template(PRM_FLT, 1, &ConfigurationNames[0], &ConfigurationDefaults[0]), // Initial Time Step Size
		//PRM_Template(PRM_FLT, 1, &ConfigurationNames[1], &ConfigurationDefaults[1]), // Particle Radius
		//PRM_Template(PRM_TOGGLE, 1, &ConfigurationNames[2], &ConfigurationDefaults[2]), // Enable Z-Sort
		//PRM_Template(PRM_XYZ, 3, &ConfigurationNames[3], gravitationDefaults), // Gravitation
		//PRM_Template(PRM_INT, 1, &ConfigurationNames[4], &ConfigurationDefaults[4]), // Max Iterations
		//PRM_Template(PRM_FLT, 1, &ConfigurationNames[5], &ConfigurationDefaults[5]), // Max Error
		//PRM_Template(PRM_TOGGLE, 1, &ConfigurationNames[6], &ConfigurationDefaults[6]), // Enable Divergence Solver
		//PRM_Template(PRM_INT, 1, &ConfigurationNames[7], &ConfigurationDefaults[7]), // Max Iterations of Divergence Solver
		//PRM_Template(PRM_FLT, 1, &ConfigurationNames[8], &ConfigurationDefaults[8]), // Max Divergence Error
		//PRM_Template(PRM_ORD, 1, &ConfigurationNames[9], &ConfigurationDefaults[9]), // CFL Method
		//PRM_Template(PRM_FLT, 1, &ConfigurationNames[10], &ConfigurationDefaults[10]), // CFL Factor
		//PRM_Template(PRM_FLT, 1, &ConfigurationNames[11], &ConfigurationDefaults[11]), // CFL Min Time Step Size
		//PRM_Template(PRM_FLT, 1, &ConfigurationNames[12], &ConfigurationDefaults[12]), // CFL Max Time Step Size

		//// Materials tab
		//PRM_Template(PRM_ORD, 1, &MaterialsName[0], &MaterialsDefaults[0]), // Viscosity Method
		//PRM_Template(PRM_FLT, 1, &MaterialsName[1], &MaterialsDefaults[1]), // Viscosity
		//PRM_Template(PRM_INT, 1, &MaterialsName[2], &MaterialsDefaults[2]), // Max Viscosity Iterations
		//PRM_Template(PRM_FLT, 1, &MaterialsName[3], &MaterialsDefaults[3]), // Max Viscosity Error
		//PRM_Template(PRM_INT, 1, &MaterialsName[4], &MaterialsDefaults[4]), // Max Iterations for Vorticity Diffusion
		//PRM_Template(PRM_FLT, 1, &MaterialsName[5], &MaterialsDefaults[5]), // Max Error for Vorticity Diffusion
		//PRM_Template(PRM_FLT, 1, &MaterialsName[6], &MaterialsDefaults[6]), // Boundary Viscosity
		//PRM_Template(PRM_ORD, 1, &MaterialsName[7], &MaterialsDefaults[7]), // Vorticity Method
		//PRM_Template(PRM_FLT, 1, &MaterialsName[8], &MaterialsDefaults[8]), // Vorticity
		//PRM_Template(PRM_FLT, 1, &MaterialsName[9], &MaterialsDefaults[9]), // Viscosity Omega
		//PRM_Template(PRM_FLT, 1, &MaterialsName[10], &MaterialsDefaults[10]), // Inertia Inverse

		PRM_Template() // Sentinel
};


// Here's how we define local variables for the SOP.
enum {
	VAR_PT,		// Point number of the star
	VAR_NPT		// Number of points in the star
};

CH_LocalVariable
SOP_FUILDSIMULATOR::myVariables[] = {
	{ "PT",	VAR_PT, 0 },		// The table provides a mapping
	{ "NPT",	VAR_NPT, 0 },		// from text string to integer token
	{ 0, 0, 0 },
};

bool
SOP_FUILDSIMULATOR::evalVariableValue(fpreal& val, int index, int thread)
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
			val = (fpreal)myCurrPoint;
			return true;
		case VAR_NPT:
			val = (fpreal)myTotalPoints;
			return true;
		default:
			/* do nothing */;
		}
	}
	// Not one of our variables, must delegate to the base class.
	return SOP_Node::evalVariableValue(val, index, thread);
}

OP_Node*
SOP_FUILDSIMULATOR::myConstructor(OP_Network* net, const char* name, OP_Operator* op)
{
	return new SOP_FUILDSIMULATOR(net, name, op);
}

SOP_FUILDSIMULATOR::SOP_FUILDSIMULATOR(OP_Network* net, const char* name, OP_Operator* op)
	: SOP_Node(net, name, op)
{
	myCurrPoint = -1;	// To prevent garbage values from being returned
	mySim = std::unique_ptr<SPH::Simulation>(new SPH::Simulation());
	mySceneLoader = std::unique_ptr<Utilities::SceneLoader>(new Utilities::SceneLoader());
}

SOP_FUILDSIMULATOR::~SOP_FUILDSIMULATOR() {}

unsigned
SOP_FUILDSIMULATOR::disableParms()
{
	return 0;
}

// get parameter from the handle
UT_String SOP_FUILDSIMULATOR::getParameters(GA_ROHandleS paraHandle) {
	// get parameters from the input from last ndoe
	if (paraHandle.isValid()) {
		UT_String value;
		value = paraHandle.get(GA_Offset(0));
		if (value) {
			std::cout << "Retrieved attribute value: " << value.toStdString() << std::endl;
			return value;
		}
		else {
			std::cout << "Failed to get attribute value." << std::endl;
		}
	}
	else {
		std::cout << "Attribute handle is not valid." << std::endl;
	}
}

// get parameters from the previous node's detail list
void SOP_FUILDSIMULATOR::populateParameters(fpreal t) {

	// set inputPathName
	GA_ROHandleS inputPathHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "Fluid_obj_path"));
	UT_String inputPath = getParameters(inputPathHandle);
	setString(inputPath, CH_StringMeaning(), inputPathName.getToken(), 0, t);


	GA_ROHandleS particleRadiusHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "particleRadius"));
	UT_String particleRadius = getParameters(particleRadiusHandle);
	std::cout << "Attribute set to: " << particleRadius.toStdString() << std::endl;
	std::cout << "getToken value is:" << ConfigurationNames[1].getToken() << std::endl;
	setString(particleRadius, CH_StringMeaning(), ConfigurationNames[1].getToken(), 0, t);
	std::cout << "Attribute set to: " << ConfigurationNames[1].getToken() << " in: " << evalFloat(ConfigurationNames[1].getToken(), 0, t) << std::endl;
	float particleRadiusValue = evalFloat(ConfigurationNames[1].getToken(), 0, t);

	// populating configuration parameters
	GA_ROHandleS timeStepSizeHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "timeStepSize"));
	UT_String timeStepSize = getParameters(timeStepSizeHandle);
	setString(timeStepSize, CH_StringMeaning(), ConfigurationNames[0].getToken(), 0, t);
	float timeStepSizeValue = evalFloat(ConfigurationNames[0].getToken(), 0, t);

	GA_ROHandleS enableZSortHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "enableZSort"));
	UT_String enableZSort = getParameters(enableZSortHandle);
	setString(enableZSort, CH_StringMeaning(), ConfigurationNames[2].getToken(), 0, t);
	float enableZSortValue = evalFloat(ConfigurationNames[2].getToken(), 0, t);

	GA_ROHandleS gravitationHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "gravitation"));
	UT_String gravitation = getParameters(gravitationHandle);
	setString(gravitation, CH_StringMeaning(), ConfigurationNames[3].getToken(), 0, t);
	// Tokenize the string based on the comma delimiter
	UT_StringArray components;
	gravitation.tokenize(components, ",");
	// Convert components to floats
	float x = static_cast<float>(atof(components(0).buffer()));
	float y = static_cast<float>(atof(components(1).buffer()));
	float z = static_cast<float>(atof(components(2).buffer()));
	// Create a UT_Vector3
	UT_Vector3 gravitationValue(x, y, z);

	GA_ROHandleS maxIterationsHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxIterations"));
	UT_String maxIterations = getParameters(maxIterationsHandle);
	setString(maxIterations, CH_StringMeaning(), ConfigurationNames[4].getToken(), 0, t);
	float maxIterationsValue = evalFloat(ConfigurationNames[4].getToken(), 0, t);


	GA_ROHandleS maxErrorHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxError"));
	UT_String maxError = getParameters(maxErrorHandle);
	setString(maxError, CH_StringMeaning(), ConfigurationNames[5].getToken(), 0, t);
	float maxErrorValue = evalFloat(ConfigurationNames[5].getToken(), 0, t);

	GA_ROHandleS enableDivergenceSolverHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "enableDivergenceSolver"));
	UT_String enableDivergenceSolver = getParameters(enableDivergenceSolverHandle);
	setString(enableDivergenceSolver, CH_StringMeaning(), ConfigurationNames[6].getToken(), 0, t);
	float enableDivergenceSolverValue = evalFloat(ConfigurationNames[6].getToken(), 0, t);

	GA_ROHandleS maxIterationsVHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxIterationsV"));
	UT_String maxIterationsV = getParameters(maxIterationsVHandle);
	setString(maxIterationsV, CH_StringMeaning(), ConfigurationNames[7].getToken(), 0, t);
	float maxIterationsVValue = evalFloat(ConfigurationNames[7].getToken(), 0, t);

	GA_ROHandleS maxErrorVHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxErrorV"));
	UT_String maxErrorV = getParameters(maxErrorVHandle);
	setString(maxErrorV, CH_StringMeaning(), ConfigurationNames[8].getToken(), 0, t);
	float maxErrorVValue = evalFloat(ConfigurationNames[8].getToken(), 0, t);

	GA_ROHandleS cflMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflMethod"));
	UT_String cflMethod = getParameters(cflMethodHandle);
	setString(cflMethod, CH_StringMeaning(), ConfigurationNames[9].getToken(), 0, t);
	float cflMethodValue = evalFloat(ConfigurationNames[9].getToken(), 0, t);

	GA_ROHandleS cflFactorHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflFactor"));
	UT_String cflFactor = getParameters(cflFactorHandle);
	setString(cflFactor, CH_StringMeaning(), ConfigurationNames[10].getToken(), 0, t);
	float cflFactorValue = evalFloat(ConfigurationNames[10].getToken(), 0, t);

	GA_ROHandleS cflMaxTimeStepSizeHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflMaxTimeStepSize"));
	UT_String cflMaxTimeStepSize = getParameters(cflMaxTimeStepSizeHandle);
	setString(cflMaxTimeStepSize, CH_StringMeaning(), ConfigurationNames[11].getToken(), 0, t);
	float cflMaxTimeStepSizeValue = evalFloat(ConfigurationNames[11].getToken(), 0, t);

	GA_ROHandleS cflMinTimeStepSizeHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflMinTimeStepSize"));
	UT_String cflMinTimeStepSize = getParameters(cflMinTimeStepSizeHandle);
	setString(cflMinTimeStepSize, CH_StringMeaning(), ConfigurationNames[12].getToken(), 0, t);
	float cflMinTimeStepSizeValue = evalFloat(ConfigurationNames[12].getToken(), 0, t);

	// populating materials parameters
	GA_ROHandleS viscosityMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscosityMethod"));
	UT_String viscosityMethod = getParameters(viscosityMethodHandle);
	setString(viscosityMethod, CH_StringMeaning(), MaterialsName[0].getToken(), 0, t);
	float viscosityMethodValue = evalFloat(MaterialsName[0].getToken(), 0, t);

	GA_ROHandleS viscosityHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscosity"));
	UT_String viscosity = getParameters(viscosityHandle);
	setString(viscosity, CH_StringMeaning(), MaterialsName[1].getToken(), 0, t);
	float viscosityValue = evalFloat(MaterialsName[1].getToken(), 0, t);

	GA_ROHandleS viscoMaxIterHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscoMaxIter"));
	UT_String viscoMaxIter = getParameters(viscoMaxIterHandle);
	setString(viscoMaxIter, CH_StringMeaning(), MaterialsName[2].getToken(), 0, t);
	float viscoMaxIterValue = evalFloat(MaterialsName[2].getToken(), 0, t);

	GA_ROHandleS viscoMaxErrorHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscoMaxError"));
	UT_String viscoMaxError = getParameters(viscoMaxErrorHandle);
	setString(viscoMaxError, CH_StringMeaning(), MaterialsName[3].getToken(), 0, t);
	float viscoMaxErrorValue = evalFloat(MaterialsName[3].getToken(), 0, t);

	GA_ROHandleS viscoMaxIterOmegaHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscoMaxIterOmega"));
	UT_String viscoMaxIterOmega = getParameters(viscoMaxIterOmegaHandle);
	setString(viscoMaxIterOmega, CH_StringMeaning(), MaterialsName[4].getToken(), 0, t);
	float viscoMaxIterOmegaValue = evalFloat(MaterialsName[4].getToken(), 0, t);

	GA_ROHandleS viscoMaxErrorOmegaHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscoMaxErrorOmega"));
	UT_String viscoMaxErrorOmega = getParameters(viscoMaxErrorOmegaHandle);
	setString(viscoMaxErrorOmega, CH_StringMeaning(), MaterialsName[5].getToken(), 0, t);
	float viscoMaxErrorOmegaValue = evalFloat(MaterialsName[5].getToken(), 0, t);

	GA_ROHandleS viscosityBoundaryHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscosityBoundary"));
	UT_String viscosityBoundary = getParameters(viscosityBoundaryHandle);
	setString(viscosityBoundary, CH_StringMeaning(), MaterialsName[6].getToken(), 0, t);
	float viscosityBoundaryValue = evalFloat(MaterialsName[6].getToken(), 0, t);

	GA_ROHandleS vorticityMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "vorticityMethod"));
	UT_String vorticityMethod = getParameters(vorticityMethodHandle);
	setString(vorticityMethod, CH_StringMeaning(), MaterialsName[7].getToken(), 0, t);
	float vorticityMethodValue = evalFloat(MaterialsName[7].getToken(), 0, t);

	GA_ROHandleS vorticityHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "vorticity"));
	UT_String vorticity = getParameters(vorticityHandle);
	setString(vorticity, CH_StringMeaning(), MaterialsName[8].getToken(), 0, t);
	float vorticityValue = evalFloat(MaterialsName[8].getToken(), 0, t);

	GA_ROHandleS viscosityOmegaHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscosityOmega"));
	UT_String viscosityOmega = getParameters(viscosityOmegaHandle);
	setString(viscosityOmega, CH_StringMeaning(), MaterialsName[9].getToken(), 0, t);
	float viscosityOmegaValue = evalFloat(MaterialsName[9].getToken(), 0, t);

	GA_ROHandleS inertiaInverseHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "inertiaInverse"));
	UT_String inertiaInverse = getParameters(inertiaInverseHandle);
	setString(inertiaInverse, CH_StringMeaning(), MaterialsName[10].getToken(), 0, t);
	float inertiaInverseValue = evalFloat(MaterialsName[10].getToken(), 0, t);

	// write to json file
	fs::path jsonFilePath = fs::absolute("parameters.json");

	std::ostringstream jsonStream;
	jsonStream << std::fixed; // Ensures floating point values are not written in scientific notation.

	jsonStream << "{\n";
	jsonStream << "  \"Configuration\": {\n";
	jsonStream << "    \"particleRadius\": " << particleRadius << ",\n";
	jsonStream << "    \"timeStepSize\": " << timeStepSizeValue << ",\n";
	jsonStream << "    \"density0\": " << 1000 << ",\n";
	jsonStream << "    \"simulationMethod\": " << 4 << ",\n";
	jsonStream << "    \"cflMethod\": " << cflMethod << ",\n";
	jsonStream << "    \"cflFactor\": " << cflFactor << ",\n";
	jsonStream << "    \"cflMaxTimeStepSize\": " << cflMaxTimeStepSize << ",\n";
	jsonStream << "    \"maxIterations\": " << maxIterations << ",\n";
	jsonStream << "    \"maxError\": " << maxError << ",\n";
	jsonStream << "    \"maxIterationsV\": " << maxIterationsV << ",\n";
	jsonStream << "    \"maxErrorV\": " << maxErrorV << ",\n";
	jsonStream << "    \"stiffness\": " << 5000 << ",\n";
	jsonStream << "    \"exponent\": " << 7 << ",\n";
	jsonStream << "    \"velocityUpdateMethod\": " << 0 << ",\n";
	jsonStream << "    \"enableDivergenceSolver\": " << (enableDivergenceSolver ? "true" : "false") << ",\n";
	jsonStream << "    \"boundaryHandlingMethod\": " << 2 << ",\n";
	jsonStream << "    \"gravitation\": [" << gravitation[0] << ", " << gravitation[1] << ", " << gravitation[2] << "]\n";
	jsonStream << "  },\n";

	// Materials Section
	jsonStream << "  \"Materials\": [{\n";
	jsonStream << "    \"id\": \"Fluid\",\n";
	jsonStream << "    \"viscosity\": " << viscosity << ",\n";
	jsonStream << "    \"viscosityMethod\": " << viscosityMethod << ",\n";
	jsonStream << "    \"colorMapType\": " << 1 << "\n";
	jsonStream << "  }],\n";

	// RigidBodies Section
	jsonStream << "  \"RigidBodies\": [{\n";
	jsonStream << "    \"geometryFile\": \"../models/UnitBox.obj\",\n";
	jsonStream << "    \"translation\": [" << translation[0] << ", " << translation[1] << ", " << translation[2] << "],\n";
	jsonStream << "    \"rotationAxis\": [" << rotationAxis[0] << ", " << rotationAxis[1] << ", " << rotationAxis[2] << "],\n";
	jsonStream << "    \"scale\": [" << scale[0] << ", " << scale[1] << ", " << scale[2] << "],\n";
	jsonStream << "    \"color\": [" << color[0] << ", " << color[1] << ", " << color[2] << ", " << color[3] << "],\n";
	jsonStream << "    \"isDynamic\": " << (isDynamic ? "true" : "false") << ",\n";
	jsonStream << "    \"isWall\": " << (isWall ? "true" : "false") << ",\n";
	jsonStream << "    \"mapInvert\": " << (mapInvert ? "true" : "false") << "\n";
	jsonStream << "  }],\n";

	// FluidBlocks Section
	jsonStream << "  \"FluidBlocks\": [{\n";
	jsonStream << "    \"denseMode\": " << denseMode << ",\n";
	jsonStream << "    \"start\": [" << start[0] << ", " << start[1] << ", " << start[2] << "],\n";
	jsonStream << "    \"end\": [" << end[0] << ", " << end[1] << ", " << end[2] << "],\n";
	jsonStream << "    \"translation\": [" << translationFB[0] << ", " << translationFB[1] << ", " << translationFB[2] << "],\n";
	jsonStream << "    \"scale\": [" << scaleFB[0] << ", " << scaleFB[1] << ", " << scaleFB[2] << "]\n";
	jsonStream << "  }]\n";
	jsonStream << "}\n";

	// Write the constructed JSON-like string to a file

	std::ofstream file(jsonFilePath);
	if (file) {
		file << jsonStream.str();
	}
	else {
		// Error handling
		return error();
	}


	

#if 0
	// Fetching the rest of the parameters
	GA_ROHandleS numberOfStepsPerRenderUpdateHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "numberOfStepsPerRenderUpdate"));
	UT_String numberOfStepsPerRenderUpdate = getParameters(numberOfStepsPerRenderUpdateHandle);
	setString(numberOfStepsPerRenderUpdate, CH_StringMeaning(), names[1].getToken(), 0, t);

	GA_ROHandleS density0Handle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "density0"));
	UT_String density0 = getParameters(density0Handle);
	setString(density0, CH_StringMeaning(), names[2].getToken(), 0, t);

	GA_ROHandleS simulationMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "simulationMethod"));
	UT_String simulationMethod = getParameters(simulationMethodHandle);
	setString(simulationMethod, CH_StringMeaning(), names[3].getToken(), 0, t);

	GA_ROHandleS gravitationHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "gravitation"));
	UT_String gravitation = getParameters(gravitationHandle);
	setString(gravitation, CH_StringMeaning(), names[4].getToken(), 0, t);

	GA_ROHandleS timeStepSizeHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "timeStepSize"));
	UT_String timeStepSize = getParameters(timeStepSizeHandle);
	setString(timeStepSize, CH_StringMeaning(), names[5].getToken(), 0, t);

	GA_ROHandleS cflMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflMethod"));
	UT_String cflMethod = getParameters(cflMethodHandle);
	setString(cflMethod, CH_StringMeaning(), names[6].getToken(), 0, t);

	GA_ROHandleS cflFactorHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflFactor"));
	UT_String cflFactor = getParameters(cflFactorHandle);
	setString(cflFactor, CH_StringMeaning(), names[7].getToken(), 0, t);

	GA_ROHandleS cflMaxTimeStepSizeHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflMaxTimeStepSize"));
	UT_String cflMaxTimeStepSize = getParameters(cflMaxTimeStepSizeHandle);
	setString(cflMaxTimeStepSize, CH_StringMeaning(), names[8].getToken(), 0, t);

	GA_ROHandleS maxIterationsHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxIterations"));
	UT_String maxIterations = getParameters(maxIterationsHandle);
	setString(maxIterations, CH_StringMeaning(), names[9].getToken(), 0, t);

	GA_ROHandleS maxErrorHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxError"));
	UT_String maxError = getParameters(maxErrorHandle);
	setString(maxError, CH_StringMeaning(), names[10].getToken(), 0, t);

	GA_ROHandleS maxIterationsVHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxIterationsV"));
	UT_String maxIterationsV = getParameters(maxIterationsVHandle);
	setString(maxIterationsV, CH_StringMeaning(), names[11].getToken(), 0, t);

	GA_ROHandleS maxErrorVHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxErrorV"));
	UT_String maxErrorV = getParameters(maxErrorVHandle);
	setString(maxErrorV, CH_StringMeaning(), names[12].getToken(), 0, t);

	GA_ROHandleS stiffnessHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "stiffness"));
	UT_String stiffness = getParameters(stiffnessHandle);
	setString(stiffness, CH_StringMeaning(), names[13].getToken(), 0, t);

	GA_ROHandleS exponentHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "exponent"));
	UT_String exponent = getParameters(exponentHandle);
	setString(exponent, CH_StringMeaning(), names[14].getToken(), 0, t);

	GA_ROHandleS velocityUpdateMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "velocityUpdateMethod"));
	UT_String velocityUpdateMethod = getParameters(velocityUpdateMethodHandle);
	setString(velocityUpdateMethod, CH_StringMeaning(), names[15].getToken(), 0, t);

	GA_ROHandleS enableDivergenceSolverHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "enableDivergenceSolver"));
	UT_String enableDivergenceSolver = getParameters(enableDivergenceSolverHandle);
	setString(enableDivergenceSolver, CH_StringMeaning(), names[16].getToken(), 0, t);

	GA_ROHandleS particleAttributesHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "particleAttributes"));
	UT_String particleAttributes = getParameters(particleAttributesHandle);
	setString(particleAttributes, CH_StringMeaning(), names[17].getToken(), 0, t);

	GA_ROHandleS boundaryHandlingMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "boundaryHandlingMethod"));
	UT_String boundaryHandlingMethod = getParameters(boundaryHandlingMethodHandle);
	setString(boundaryHandlingMethod, CH_StringMeaning(), names[18].getToken(), 0, t);

	GA_ROHandleS viscosityHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscosity"));
	UT_String viscosity = getParameters(viscosityHandle);
	setString(viscosity, CH_StringMeaning(), names[19].getToken(), 0, t);

	GA_ROHandleS viscosityMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscosityMethod"));
	UT_String viscosityMethod = getParameters(viscosityMethodHandle);
	setString(viscosityMethod, CH_StringMeaning(), names[20].getToken(), 0, t);

	GA_ROHandleS colorMapTypeHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "colorMapType"));
	UT_String colorMapType = getParameters(colorMapTypeHandle);
	setString(colorMapType, CH_StringMeaning(), names[21].getToken(), 0, t);

	// Fetching RigidBodies values and fluid blocks values
	GA_ROHandleS translationHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "translation"));
	UT_String translation = getParameters(translationHandle);
	setString(translation, CH_StringMeaning(), translationName.getToken(), 0, t);

	GA_ROHandleS rotationAxisHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "rotationAxis"));
	UT_String rotationAxis = getParameters(rotationAxisHandle);
	setString(rotationAxis, CH_StringMeaning(), rotationAxisName.getToken(), 0, t);

	GA_ROHandleS rotationAngleHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "rotationAngle"));
	UT_String rotationAngle = getParameters(rotationAngleHandle);
	setString(rotationAngle, CH_StringMeaning(), rotationAngleName.getToken(), 0, t);

	GA_ROHandleS scaleHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "scale"));
	UT_String scale = getParameters(scaleHandle);
	setString(scale, CH_StringMeaning(), scaleName.getToken(), 0, t);

	GA_ROHandleS colorHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "color"));
	UT_String color = getParameters(colorHandle);
	setString(color, CH_StringMeaning(), colorName.getToken(), 0, t);

	GA_ROHandleS isDynamicHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "isDynamic"));
	UT_String isDynamic = getParameters(isDynamicHandle);
	setString(isDynamic, CH_StringMeaning(), isDynamicName.getToken(), 0, t);

	GA_ROHandleS isWallHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "isWall"));
	UT_String isWall = getParameters(isWallHandle);
	setString(isWall, CH_StringMeaning(), isWallName.getToken(), 0, t);

	GA_ROHandleS mapInvertHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "mapInvert"));
	UT_String mapInvert = getParameters(mapInvertHandle);
	setString(mapInvert, CH_StringMeaning(), mapInvertName.getToken(), 0, t);

	GA_ROHandleS mapThicknessHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "mapThickness"));
	UT_String mapThickness = getParameters(mapThicknessHandle);
	setString(mapThickness, CH_StringMeaning(), mapThicknessName.getToken(), 0, t);

	GA_ROHandleS mapResolutionHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "mapResolution"));
	UT_String mapResolution = getParameters(mapResolutionHandle);
	setString(mapResolution, CH_StringMeaning(), mapResolutionName.getToken(), 0, t);

	GA_ROHandleS denseModeHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "denseMode"));
	UT_String denseMode = getParameters(denseModeHandle);
	setString(denseMode, CH_StringMeaning(), denseModeName.getToken(), 0, t);

	GA_ROHandleS startHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "start"));
	UT_String start = getParameters(startHandle);
	setString(start, CH_StringMeaning(), startName.getToken(), 0, t);

	GA_ROHandleS endHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "end"));
	UT_String end = getParameters(endHandle);
	setString(end, CH_StringMeaning(), endName.getToken(), 0, t);

	GA_ROHandleS translationFBHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "translationFB"));
	UT_String translationFB = getParameters(translationFBHandle);
	setString(translationFB, CH_StringMeaning(), translationFBName.getToken(), 0, t);

	GA_ROHandleS scaleFBHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "scaleFB"));
	UT_String scaleFB = getParameters(scaleFBHandle);
	setString(scaleFB, CH_StringMeaning(), scaleFBName.getToken(), 0, t);
#endif
	
}

OP_ERROR
SOP_FUILDSIMULATOR::cookMySop(OP_Context& context)
{
	fpreal	now = context.getTime();
	lastCookTime = now;

	//bool currentCheckboxState = evalInt(jsonUpdateName.getToken(), 0, now) != 0;
	//float particleRadius = evalFloat(names[0].getToken(), 0, now);
	//int numberOfStepsPerRenderUpdate = evalInt(names[1].getToken(), 0, now);
	//float density0 = evalFloat(names[2].getToken(), 0, now);
	//int simulationMethod = evalInt(names[3].getToken(), 0, now);
	//float timeStepSize = evalFloat(names[5].getToken(), 0, now);
	//int cflMethod = evalInt(names[6].getToken(), 0, now);
	//float cflFactor = evalFloat(names[7].getToken(), 0, now);
	//float cflMaxTimeStepSize = evalFloat(names[8].getToken(), 0, now);
	//int maxIterations = evalInt(names[9].getToken(), 0, now);
	//float maxError = evalFloat(names[10].getToken(), 0, now);
	//int maxIterationsV = evalInt(names[11].getToken(), 0, now);
	//float maxErrorV = evalFloat(names[12].getToken(), 0, now);
	//float stiffness = evalFloat(names[13].getToken(), 0, now);
	//int exponent = evalInt(names[14].getToken(), 0, now);
	//int velocityUpdateMethod = evalInt(names[15].getToken(), 0, now);
	//int enableDivergenceSolver = evalInt(names[16].getToken(), 0, now); // This is a boolean represented as an int
	//int boundaryHandlingMethod = evalInt(names[18].getToken(), 0, now);
	//float viscosity = evalFloat(names[19].getToken(), 0, now);
	//int viscosityMethod = evalInt(names[20].getToken(), 0, now);
	//int colorMapType = evalInt(names[21].getToken(), 0, now);

	//// Fetching vector values
	//UT_Vector3 gravitation;
	//evalFloats(names[4].getToken(), gravitation.data(), now);

	//UT_Vector3 translation;
	//evalFloats(translationName.getToken(), translation.data(), now);

	//UT_Vector3 rotationAxis;
	//evalFloats(rotationAxisName.getToken(), rotationAxis.data(), now);

	//float rotationAngle = evalFloat(rotationAngleName.getToken(), 0, now);

	//UT_Vector3 scale;
	//evalFloats(scaleName.getToken(), scale.data(), now);

	//UT_Vector4 color;
	//evalFloats(colorName.getToken(), color.data(), now); // Assuming RGBA

	//// Fetching string (particleAttributes)
	//UT_String particleAttributes;
	//evalString(particleAttributes, names[17].getToken(), 0, now);

	// Assuming "isDynamic", "isWall", "mapInvert" are toggles represented as booleans
	//bool isDynamic = evalInt(isDynamicName.getToken(), 0, now) != 0;
	//bool isWall = evalInt(isWallName.getToken(), 0, now) != 0;
	//bool mapInvert = evalInt(mapInvertName.getToken(), 0, now) != 0;

	//// Assuming "denseMode" is an integer
	//int denseMode = evalInt(denseModeName.getToken(), 0, now);

	//// "start", "end", "translationFB", and "scaleFB" are vectors
	//UT_Vector3 start, end, translationFB, scaleFB;
	//evalFloats(startName.getToken(), start.data(), now);
	//evalFloats(endName.getToken(), end.data(), now);
	//evalFloats(translationFBName.getToken(), translationFB.data(), now);
	//evalFloats(scaleFBName.getToken(), scaleFB.data(), now);



	// Add gravitation as an array
	fs::path jsonFilePath = fs::absolute("parameters.json");
	//if (currentCheckboxState && !lastCheckboxState) {
	//	// The checkbox was just checked - Output the JSON file
	//	std::cout << "checked" << std::endl;
	//	fluid_printCurrentPath();

	

		// Your existing code to create or modify geometry goes here.
		// ...

	//}
	//else if (!currentCheckboxState && lastCheckboxState) {
	//	// The checkbox was just unchecked - Optionally handle this case
	//}

	// Update the last known state
	//lastCheckboxState = currentCheckboxState;
	// Now that you have all the branches ,which is the start and end point of each point ,its time to render 
	// these branches into Houdini 


	UT_Interrupt* boss;

	myTotalPoints = 0;		// Set the NPT local variable value
	myCurrPoint = 0;			// Initialize the PT local variable



	// Check to see that there hasn't been a critical error in cooking the SOP.
	if (error() < UT_ERROR_ABORT)
	{
		boss = UTgetInterrupt();
		gdp->clearAndDestroy();

		////////////////////////////////////////////////////////////////////////////////////////////
		// test on the input geometry
		// Access input geometry
		OP_AutoLockInputs inputs(this);
		if (inputs.lock(context) >= UT_ERROR_ABORT)
			return error();

		// Duplicate our incoming geometry with the hint that we only
		// altered points.  Thus, if our input was unchanged, we can
	// easily roll back our changes by copying point values.
		duplicatePointSource(0, context);


		////////////////////////////////////////////////////////////////////////////////////////////

		// Start the interrupt server
		if (boss->opStart("Building Fluid Simulator"))
		{
			UT_String aname("particleRadius");
			// Using GA_RWHandleS for string attributes
			GA_RWHandleS attrib(gdp->findStringTuple(GA_ATTRIB_DETAIL, aname));

			// Not present, so create the detail attribute:
			if (!attrib.isValid()) {
				attrib = GA_RWHandleS(gdp->addStringTuple(GA_ATTRIB_DETAIL, aname, 1));
				std::cout << "Attribute added." << std::endl;
			}

			if (attrib.isValid()) {
				// Store the value in the detail attribute
				// NOTE: The detail is *always* at GA_Offset(0)
				// Assuming jsonFilePath is a std::string or similar containing the file path
				//UT_String particleRadius; // Convert path to std::string
				//std::string particleRadiusStr = std::to_string(particleRadius); // Convert path to std::string
				//UT_String outvalue(particleRadiusStr.c_str());
				//attrib.set(GA_Offset(0), outvalue); // Replace denseMode with the string variable
				//std::cout << "Attribute set to: " << particleRadius << std::endl;
				//attrib.bumpDataId(); // This is typically not necessary for setting attribute values
			}

			// test get detail attribute
			populateParameters(now);

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