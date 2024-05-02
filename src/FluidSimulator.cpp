
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


void initFluidSimulator(SPH::SimulatorBase& obj,
	std::string sceneFile = "data/Scenes/DoubleDamBreak.json", 
	std::string programName = "SPlisHSPlasH",
	bool useCache = true,
	std::string stateFile = "",
	std::string outputDir = "",
	bool initialPause = true,
	bool useGui = true,
	float stopAt = -1.0f,
	std::string param = "")  {

	std::cout << "Initializing Fluid Simulator..." << std::endl;

	std::vector<std::string> argv;
	argv.push_back(programName);
	argv.push_back("--scene-file"); argv.push_back(sceneFile);
	if (!useCache) argv.push_back("--no-cache");
	argv.push_back("--stopAt");
	argv.push_back(std::to_string(stopAt));
	if (strcmp(param.c_str(), "") != 0) {
		argv.push_back("--param");
		argv.push_back(param);
	}
	if (strcmp(stateFile.c_str(), "") != 0) {
		argv.push_back("--state-file");
		argv.push_back(stateFile);
	}
	if (strcmp(outputDir.c_str(), "") != 0) {
		argv.push_back("--output-dir");
		argv.push_back(outputDir);
	}
	if (!initialPause) argv.push_back("--no-initial-pause");
	if (!useGui) argv.push_back("--no-gui");
	obj.init(argv, "SPlisHSPlasH");
}

void myRunSimulation(SPH::SimulatorBase& obj, std::vector<std::vector<std::vector<Vector3r>>>& my_particles) {
	std::cout << "Running Simulation..." << std::endl;
	obj.deferredInit();
	obj.setCommandLineParameter_pub();


	
	const Real stopAt = obj.getValue<Real>(obj.SimulatorBase::STOP_AT);
	std::cout << "StopAt: " << stopAt << std::endl;

	if (stopAt < 0.0)
	{
		LOG_ERR << "StopAt parameter must be set when starting without GUI.";
		exit(1);
	}

	int frameCounter = 0;

	while (true)
	{
			
		if (!obj.myTimeStepNoGUI(frameCounter, my_particles))
			break;

		frameCounter++;
	}
	

}	

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

static PRM_Name cacheFluidPathName("cacheFluidPath", "Cache Fluid Path");
static PRM_Default cacheFluidPathDefault(0, "$HIP");


// simulate button parameters
static PRM_Name simulateButtonName("simulate", "Simulate");
static PRM_Default simulateButtonNameDefault(0);

// rigid body parameters
static PRM_Name wallName("is_Wall", "Wall");
static PRM_Default wallDefault = PRM_Default(false);
static PRM_Name inputRigidObjPathName("Rigid_obj_path", "Rigidbody File Path");
static PRM_Default inputRigidObjPathDefault(0, "");

// output path parameters


float getStringParamAsFloat(static PRM_Name &name) {
	//OP_Parameters::evalFloat(names[0].getToken(), 0, t)
	return 0.0;
}

void SOP_FUILDSIMULATOR::drawParticles(int frame, std::vector<std::vector<std::vector<Vector3r>>>& particles_in_frames) {
	std::cout << "Drawing Particles in frame" << frame << std::endl;
	//std::cout << "Number of particles for drawing: " << pos.size() << std::endl;
	// get the particles of the current frame
	std::vector<std::vector<Vector3r>> particles = particles_in_frames[frame];
	// get the number of particles in the current frame
	std::cout << "Number of particles in the current frame: " << particles.size() << std::endl;
	for (size_t i = 0; i < particles.size(); ++i) {
		
		GA_Offset ptoff = gdp->appendPoint();
		fpreal32 x = particles[i][0].x();
		fpreal32 y = particles[i][0].y();
		fpreal32 z = particles[i][0].z();
		UT_Vector3 posH(x, y, z);
		gdp->setPos3(ptoff, posH);
	}
}

/*static*/ int SOP_FUILDSIMULATOR::simulateFluid(void* data, int index, float time, const PRM_Template* tplate) {
	// Handle the button press event here
	std::cout << "Button Pressed" << std::endl;
	SOP_FUILDSIMULATOR* sop = static_cast<SOP_FUILDSIMULATOR*>(data);
	//OP_Context& context = sop->getContext();
	fpreal	now = sop->lastCookTime;
	
	try
	{
		static unsigned int m_stopCounter;
		std::cout << "Simulator initializing ..." << std::endl;

		initFluidSimulator(*sop->mySimulator, sop->mySceneFile, "SPlisHSPlasH", true, "", sop->myOutputPath.toStdString(), false, false, 10.f, "");
		sop->mySimulator->initSimulation(); // this line is working good
		// need to divide runSimulation() so I can have the attributes of the particles not in patio format
		// checked the runSimulation() function in SimulatorBase.cpp and all necessary member functions are not private
		if (!sop->my_pos || !sop->my_vel || !sop->my_angVel || !sop->my_particles) {
			// Handle the uninitialized case
			std::cerr << "One or more vectors are not initialized properly.\n";
			return 0;
		}

		//myRunSimulation(*sop->mySimulator, *sop->my_particles);
		// activate patio exporter
		//std::string patioExporterName = sop->mySimulator->getParticleExporters;
		sop->mySimulator->activateExporter("Partio Exporter", true);
		sop->mySimulator->runSimulation();

		sop->mySimulator->cleanup();

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
		PRM_Template(PRM_FILE, 1, &cacheFluidPathName, &cacheFluidPathDefault),


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
	mySimulator = std::unique_ptr<SPH::SimulatorBase>(new SPH::SimulatorBase());
	my_pos = std::make_unique<std::vector<Vector3r>>();
	my_vel = std::make_unique<std::vector<Vector3r>>();
	my_angVel = std::make_unique<std::vector<Vector3r>>();
	my_particles = std::make_unique<std::vector<std::vector<std::vector<Vector3r>>>>();
	fs::path patio_path = fs::absolute("patrio_output/");
	myOutputPath = patio_path.string();
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

	std::cout << "Populating parameters..." << std::endl;

	// set inputPathName
	GA_RWHandleS inputPathHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "Fluid_obj_path"));
	UT_String inputPath = getParameters(inputPathHandle);
	setString(inputPath, CH_StringMeaning(), inputPathName.getToken(), 0, t);
	inputPathHandle.set(GA_Offset(0), inputPath);


	GA_RWHandleS particleRadiusHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "particleRadius"));
	UT_String particleRadius = getParameters(particleRadiusHandle);
	std::cout << "Attribute set to: " << particleRadius.toStdString() << std::endl;
	std::cout << "getToken value is:" << ConfigurationNames[1].getToken() << std::endl;
	setString(particleRadius, CH_StringMeaning(), ConfigurationNames[1].getToken(), 0, t);
	std::cout << "Attribute set to: " << ConfigurationNames[1].getToken() << " in: " << evalFloat(ConfigurationNames[1].getToken(), 0, t) << std::endl;
	float particleRadiusValue = evalFloat(ConfigurationNames[1].getToken(), 0, t);

	// populating configuration parameters
	GA_RWHandleS timeStepSizeHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "timeStepSize"));
	UT_String timeStepSize = getParameters(timeStepSizeHandle);
	setString(timeStepSize, CH_StringMeaning(), ConfigurationNames[0].getToken(), 0, t);
	float timeStepSizeValue = evalFloat(ConfigurationNames[0].getToken(), 0, t);

	GA_RWHandleS enableZSortHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "enableZSort"));
	UT_String enableZSort = getParameters(enableZSortHandle);
	setString(enableZSort, CH_StringMeaning(), ConfigurationNames[2].getToken(), 0, t);
	float enableZSortValue = evalFloat(ConfigurationNames[2].getToken(), 0, t);

	GA_RWHandleS gravitationHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "gravitation"));
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

	GA_RWHandleS maxIterationsHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxIterations"));
	UT_String maxIterations = getParameters(maxIterationsHandle);
	setString(maxIterations, CH_StringMeaning(), ConfigurationNames[4].getToken(), 0, t);
	float maxIterationsValue = evalFloat(ConfigurationNames[4].getToken(), 0, t);


	GA_RWHandleS maxErrorHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxError"));
	UT_String maxError = getParameters(maxErrorHandle);
	setString(maxError, CH_StringMeaning(), ConfigurationNames[5].getToken(), 0, t);
	float maxErrorValue = evalFloat(ConfigurationNames[5].getToken(), 0, t);

	GA_RWHandleS enableDivergenceSolverHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "enableDivergenceSolver"));
	UT_String enableDivergenceSolver = getParameters(enableDivergenceSolverHandle);
	setString(enableDivergenceSolver, CH_StringMeaning(), ConfigurationNames[6].getToken(), 0, t);
	float enableDivergenceSolverValue = evalFloat(ConfigurationNames[6].getToken(), 0, t);

	GA_RWHandleS maxIterationsVHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxIterationsV"));
	UT_String maxIterationsV = getParameters(maxIterationsVHandle);
	setString(maxIterationsV, CH_StringMeaning(), ConfigurationNames[7].getToken(), 0, t);
	float maxIterationsVValue = evalFloat(ConfigurationNames[7].getToken(), 0, t);

	GA_RWHandleS maxErrorVHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "maxErrorV"));
	UT_String maxErrorV = getParameters(maxErrorVHandle);
	setString(maxErrorV, CH_StringMeaning(), ConfigurationNames[8].getToken(), 0, t);
	float maxErrorVValue = evalFloat(ConfigurationNames[8].getToken(), 0, t);

	GA_RWHandleS cflMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflMethod"));
	UT_String cflMethod = getParameters(cflMethodHandle);
	setString(cflMethod, CH_StringMeaning(), ConfigurationNames[9].getToken(), 0, t);
	float cflMethodValue = evalFloat(ConfigurationNames[9].getToken(), 0, t);

	GA_RWHandleS cflFactorHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflFactor"));
	UT_String cflFactor = getParameters(cflFactorHandle);
	setString(cflFactor, CH_StringMeaning(), ConfigurationNames[10].getToken(), 0, t);
	float cflFactorValue = evalFloat(ConfigurationNames[10].getToken(), 0, t);

	GA_RWHandleS cflMaxTimeStepSizeHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflMaxTimeStepSize"));
	UT_String cflMaxTimeStepSize = getParameters(cflMaxTimeStepSizeHandle);
	setString(cflMaxTimeStepSize, CH_StringMeaning(), ConfigurationNames[11].getToken(), 0, t);
	float cflMaxTimeStepSizeValue = evalFloat(ConfigurationNames[11].getToken(), 0, t);

	GA_RWHandleS cflMinTimeStepSizeHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "cflMinTimeStepSize"));
	UT_String cflMinTimeStepSize = getParameters(cflMinTimeStepSizeHandle);
	setString(cflMinTimeStepSize, CH_StringMeaning(), ConfigurationNames[12].getToken(), 0, t);
	float cflMinTimeStepSizeValue = evalFloat(ConfigurationNames[12].getToken(), 0, t);

	// populating materials parameters
	GA_RWHandleS viscosityMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscosityMethod"));
	UT_String viscosityMethod = getParameters(viscosityMethodHandle);
	setString(viscosityMethod, CH_StringMeaning(), MaterialsName[0].getToken(), 0, t);
	float viscosityMethodValue = evalFloat(MaterialsName[0].getToken(), 0, t);

	GA_RWHandleS viscosityHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscosity"));
	UT_String viscosity = getParameters(viscosityHandle);
	setString(viscosity, CH_StringMeaning(), MaterialsName[1].getToken(), 0, t);
	float viscosityValue = evalFloat(MaterialsName[1].getToken(), 0, t);

	GA_RWHandleS viscoMaxIterHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscoMaxIter"));
	UT_String viscoMaxIter = getParameters(viscoMaxIterHandle);
	setString(viscoMaxIter, CH_StringMeaning(), MaterialsName[2].getToken(), 0, t);
	float viscoMaxIterValue = evalFloat(MaterialsName[2].getToken(), 0, t);

	GA_RWHandleS viscoMaxErrorHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscoMaxError"));
	UT_String viscoMaxError = getParameters(viscoMaxErrorHandle);
	setString(viscoMaxError, CH_StringMeaning(), MaterialsName[3].getToken(), 0, t);
	float viscoMaxErrorValue = evalFloat(MaterialsName[3].getToken(), 0, t);

	GA_RWHandleS viscoMaxIterOmegaHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscoMaxIterOmega"));
	UT_String viscoMaxIterOmega = getParameters(viscoMaxIterOmegaHandle);
	setString(viscoMaxIterOmega, CH_StringMeaning(), MaterialsName[4].getToken(), 0, t);
	float viscoMaxIterOmegaValue = evalFloat(MaterialsName[4].getToken(), 0, t);

	GA_RWHandleS viscoMaxErrorOmegaHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscoMaxErrorOmega"));
	UT_String viscoMaxErrorOmega = getParameters(viscoMaxErrorOmegaHandle);
	setString(viscoMaxErrorOmega, CH_StringMeaning(), MaterialsName[5].getToken(), 0, t);
	float viscoMaxErrorOmegaValue = evalFloat(MaterialsName[5].getToken(), 0, t);

	GA_RWHandleS viscosityBoundaryHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscosityBoundary"));
	UT_String viscosityBoundary = getParameters(viscosityBoundaryHandle);
	setString(viscosityBoundary, CH_StringMeaning(), MaterialsName[6].getToken(), 0, t);
	float viscosityBoundaryValue = evalFloat(MaterialsName[6].getToken(), 0, t);

	GA_RWHandleS vorticityMethodHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "vorticityMethod"));
	UT_String vorticityMethod = getParameters(vorticityMethodHandle);
	setString(vorticityMethod, CH_StringMeaning(), MaterialsName[7].getToken(), 0, t);
	float vorticityMethodValue = evalFloat(MaterialsName[7].getToken(), 0, t);

	GA_RWHandleS vorticityHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "vorticity"));
	UT_String vorticity = getParameters(vorticityHandle);
	setString(vorticity, CH_StringMeaning(), MaterialsName[8].getToken(), 0, t);
	float vorticityValue = evalFloat(MaterialsName[8].getToken(), 0, t);

	GA_RWHandleS viscosityOmegaHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "viscosityOmega"));
	UT_String viscosityOmega = getParameters(viscosityOmegaHandle);
	setString(viscosityOmega, CH_StringMeaning(), MaterialsName[9].getToken(), 0, t);
	float viscosityOmegaValue = evalFloat(MaterialsName[9].getToken(), 0, t);

	GA_RWHandleS inertiaInverseHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "inertiaInverse"));
	UT_String inertiaInverse = getParameters(inertiaInverseHandle);
	setString(inertiaInverse, CH_StringMeaning(), MaterialsName[10].getToken(), 0, t);
	float inertiaInverseValue = evalFloat(MaterialsName[10].getToken(), 0, t);

	// populate rigid body parameters
	GA_RWHandleS wallHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "isWall"));
	UT_String isWall = getParameters(wallHandle);
	setString(isWall, CH_StringMeaning(), wallName.getToken(), 0, t);
	float isWallValue = evalFloat(wallName.getToken(), 0, t);

	GA_RWHandleS rigidObjPathHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "Rigid_obj_path"));
	UT_String rigidObjPath = getParameters(rigidObjPathHandle);
	setString(rigidObjPath, CH_StringMeaning(), inputRigidObjPathName.getToken(), 0, t);
	
	
	// Handle to manage the file path attribute
	evalString(myOutputPath, cacheFluidPathName.getToken(), 0, t);
	//GA_RWHandleS file_path_handle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "fluid_patio_file_path"));
	//if (!file_path_handle.isValid()) {
	//	file_path_handle = GA_RWHandleS(gdp->addStringTuple(GA_ATTRIB_DETAIL, "fluid_patio_file_path", 1));
	//}

	//if (file_path_handle.isValid()) {
	//	file_path_handle.set(GA_Offset(0), myOutputPath);
	//}

	UT_String paramName(inputPathName.getToken());
	GA_RWHandleS attrib(gdp->findStringTuple(GA_ATTRIB_DETAIL, "fluid_patio_file_path"));
	if (!attrib.isValid()) {
		attrib = GA_RWHandleS(gdp->addStringTuple(GA_ATTRIB_DETAIL, "fluid_patio_file_path", 1));
	}
	std::string paramValue = myOutputPath.toStdString();
	UT_String outValue(paramValue.c_str());
	attrib.set(GA_Offset(0), outValue);
	attrib.bumpDataId();


	// write to json file
	fs::path jsonFilePath = fs::absolute("testParametersForSimulator.json");

	mySceneFile = jsonFilePath.string();

	std::ostringstream jsonStream;
	jsonStream << std::fixed; // Ensures floating point values are not written in scientific notation.

	try
	{
		jsonStream << "{\n";
		jsonStream << "  \"Configuration\": {\n";
		jsonStream << "    \"particleRadius\": " << particleRadius << ",\n";
		jsonStream << "    \"timeStepSize\": " << timeStepSizeValue << ",\n";
		jsonStream << "    \"density0\": " << 500 << ",\n";
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
		jsonStream << "    \"gravitation\": [" << gravitationValue[0] << ", " << gravitationValue[1] << ", " << gravitationValue[2] << "]\n";
		jsonStream << "  },\n";
	}
	catch (const std::exception&)
	{
		std::cout << "Could not write configuration parameters to JSON file." << std::endl;
	}
	

	try
	{
		// Materials Section
		jsonStream << "  \"Materials\": [{\n";
		jsonStream << "    \"id\": \"Fluid\",\n";
		jsonStream << "    \"viscosity\": " << viscosity << ",\n";
		jsonStream << "    \"viscosityMethod\": " << viscosityMethod << ",\n";
		jsonStream << "    \"colorMapType\": " << 1 << ",\n";
		jsonStream << "	   \"viscoMaxIter\": " << viscoMaxIter << ",\n";
		jsonStream << "    \"viscoMaxError\": " << viscoMaxError << ",\n";
		jsonStream << "    \"viscoMaxIterOmega\": " << viscoMaxIterOmega << ",\n";
		jsonStream << "    \"viscoMaxErrorOmega\": " << viscoMaxErrorOmega << ",\n";
		jsonStream << "    \"viscosityBoundary\": " << viscosityBoundary << ",\n";
		jsonStream << "    \"vorticityMethod\": " << vorticityMethod << ",\n";
		jsonStream << "    \"vorticity\": " << vorticity << ",\n";
		jsonStream << "    \"viscosityOmega\": " << viscosityOmega << ",\n";
		jsonStream << "    \"inertiaInverse\": " << inertiaInverse << "\n";
		jsonStream << "  }],\n";
	}
	catch (const std::exception&)
	{
		std::cout << "Could not write materials parameters to JSON file." << std::endl;
	}
	
	try
	{
		// RigidBodies Section
		jsonStream << "  \"RigidBodies\": [{\n";
		jsonStream << "    \"geometryFile\": \"" << rigidObjPath.toStdString() << "\",\n";
		jsonStream << "    \"translation\": [" << 0 << ", " << 0 << ", " << 0 << "],\n";
		jsonStream << "    \"rotationAxis\": [" << 0 << ", " << 0 << ", " << 0 << "],\n";
		jsonStream << "    \"scale\": [" << 1 << ", " << 1 << ", " << 1 << "],\n";
		jsonStream << "    \"color\": [" << 0 << ", " << 0 << ", " << 0 << ", " << 1 << "],\n";
		jsonStream << "    \"isDynamic\": " << "false" << ",\n";
		jsonStream << "    \"isWall\": " << (isWall ? "true" : "false") << ",\n";
		jsonStream << "    \"mapInvert\": " << "true" << "\n";
		jsonStream << "  }],\n";
	}
	catch (const std::exception&)
	{
		std::cout << "Could not write rigid body parameters to JSON file." << std::endl;

	}

	try
	{
		// FluidBlocks Section
		jsonStream << "  \"FluidModels\": [{\n";
		jsonStream << "    \"particleFile\": \""<< inputPath.toStdString()  <<"\", \n";
		jsonStream << "    \"translation\": [" << 0 << ", " << 0 << ", " << 0 << "],\n";
		jsonStream << "    \"rotationAxis\": [" << 0 << ", " << 0 << ", " << 0 << "],\n";
		jsonStream << "    \"rotationAngle\": " << 0 <<  ",\n";
		jsonStream << "    \"scale\": [" << 0.5 << ", " << 0.5 << ", " << 0.5 << "]\n";
		jsonStream << "  }]\n";
		jsonStream << "}\n";
	}
	catch (const std::exception&)
	{
		std::cout << "Could not write fluid block parameters to JSON file." << std::endl;
	}


	// Write the constructed JSON-like string to a file

	std::ofstream file(jsonFilePath);
	if (file) {
		file << jsonStream.str();
	}
	else {
		// Error handling
		std::cout << "Error: Could not open file for writing." << std::endl;
	}
	
}

OP_ERROR
SOP_FUILDSIMULATOR::cookMySop(OP_Context& context)
{
	fpreal	now = context.getTime();
	lastCookTime = now;

	UT_Interrupt* boss;

	myTotalPoints = 0;		// Set the NPT local variable value
	myCurrPoint = 0;			// Initialize the PT local variable

	// get current frame
	int frame = context.getFrame();

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
			

			// test get detail attribute
			populateParameters(now);

			std::cout << "my_particles size: " << my_particles->size() << std::endl;

			if (!my_particles->empty()) {
				// draw particles
				std::cout << "Drawing particles on cook on frame: " << frame << std::endl;
				//drawParticles(frame, *my_particles);
			}
			

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