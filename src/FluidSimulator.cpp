
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
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


using namespace HDK_Sample;
using namespace Utilities;
namespace fs = std::filesystem;

unsigned int Utilities::Timing::m_stopCounter = 0;
unsigned int Utilities::Timing::m_startCounter = 0;
bool Utilities::Timing::m_dontPrintTimes = false;
std::stack<Utilities::TimingHelper> Utilities::Timing::m_timingStack;
std::unordered_map<int, Utilities::AverageTime> Utilities::Timing::m_averageTimes;
int Utilities::IDFactory::id = 0;
namespace Utilities {
	Logger logger; // Definition of the static logger instance
}
std::unordered_map<std::string, AverageCount> Utilities::Counting::m_averageCounts;

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

static PRM_Default particleRadiusDefault(0.025f);
static PRM_Default numberOfStepsPerRenderUpdateDefault(4);
static PRM_Default density0Default(1000);
static PRM_Default simulationMethodDefault(4);
static PRM_Default gravitationDefaults[] = { PRM_Default(0), PRM_Default(-9.81f), PRM_Default(0) };
static PRM_Default timeStepSizeDefault(0.005f);
static PRM_Default cflMethodDefault(1);
static PRM_Default cflFactorDefault(1);
static PRM_Default cflMaxTimeStepSizeDefault(0.005f);
static PRM_Default maxIterationsDefault(100);
static PRM_Default maxErrorDefault(0.05f);
static PRM_Default maxIterationsVDefault(100);
static PRM_Default maxErrorVDefault(0.1f);
static PRM_Default stiffnessDefault(50000);
static PRM_Default exponentDefault(7);
static PRM_Default velocityUpdateMethodDefault(0);
static PRM_Default enableDivergenceSolverDefault(1);
static PRM_Default particleAttributesDefault(0, "density;velocity");
static PRM_Default boundaryHandlingMethodDefault(2);
static PRM_Default viscosityDefault(0.01f); //Material 
static PRM_Default viscosityMethodDefault(1);
static PRM_Default colorMapTypeDefault(1);
static PRM_Default translationDefaults[] = { PRM_Default(0.0f), PRM_Default(1.5f), PRM_Default(0.0f) };
static PRM_Default rotationAxisDefaults[] = { PRM_Default(1.0f), PRM_Default(0.0f), PRM_Default(0.0f) };
static PRM_Default rotationAngleDefault(0.0f);
static PRM_Default scaleDefaults[] = { PRM_Default(4.0f), PRM_Default(3.0f), PRM_Default(1.5f) };
static PRM_Default colorDefaults[] = { PRM_Default(0.1f), PRM_Default(0.4f), PRM_Default(0.6f), PRM_Default(1.0f) };
static PRM_Default isDynamicDefault(0); // false
static PRM_Default isWallDefault(1); // true
static PRM_Default mapInvertDefault(1); // true
static PRM_Default mapThicknessDefault(0.0f);
static PRM_Default mapResolutionDefaults[] = { PRM_Default(40), PRM_Default(30), PRM_Default(15) };
static PRM_Default denseModeDefault(0);
static PRM_Default startDefaults[] = { PRM_Default(-0.5f), PRM_Default(0.0f), PRM_Default(-0.5f) };
static PRM_Default endDefaults[] = { PRM_Default(0.5f), PRM_Default(1.0f), PRM_Default(0.5f) };
static PRM_Default translationFBDefaults[] = { PRM_Default(-1.45f), PRM_Default(0.05f), PRM_Default(0.0f) };
static PRM_Default scaleFBDefaults[] = { PRM_Default(1.0f), PRM_Default(1.0f), PRM_Default(1.0f) };
static PRM_Default jsonUpdateDefault(0);
static PRM_Default simulateButtonNameDefault(0);

static PRM_Name names[] = {
	PRM_Name("particleRadius", "Particle Radius"),
	PRM_Name("numberOfStepsPerRenderUpdate", "Number of Steps Per Render Update"),
	PRM_Name("density0", "Initial Density"),
	PRM_Name("simulationMethod", "Simulation Method"),
	PRM_Name("gravitation", "Gravitation"),
	PRM_Name("timeStepSize", "Time Step Size"),
	PRM_Name("cflMethod", "CFL Method"),
	PRM_Name("cflFactor", "CFL Factor"),
	PRM_Name("cflMaxTimeStepSize", "CFL Max Time Step Size"),
	PRM_Name("maxIterations", "Max Iterations"),
	PRM_Name("maxError", "Max Error"),
	PRM_Name("maxIterationsV", "Max Iterations V"),
	PRM_Name("maxErrorV", "Max Error V"),
	PRM_Name("stiffness", "Stiffness"),
	PRM_Name("exponent", "Exponent"),
	PRM_Name("velocityUpdateMethod", "Velocity Update Method"),
	PRM_Name("enableDivergenceSolver", "Enable Divergence Solver"),
	PRM_Name("particleAttributes", "Particle Attributes"),
	PRM_Name("boundaryHandlingMethod", "Boundary Handling Method"),
	PRM_Name("viscosityFluid", "Viscosity"),
	PRM_Name("viscosityMethod", "ViscosityMethod"),
	PRM_Name("colorMapType", "ColorMapType"),

	PRM_Name(0) // Sentinel to mark the end of the array
};
static PRM_Name translationName("translation", "Translation");
static PRM_Name rotationAxisName("rotationAxis", "Rotation Axis");
static PRM_Name rotationAngleName("rotationAngle", "Rotation Angle");
static PRM_Name scaleName("scale", "Scale");
static PRM_Name colorName("color", "Color");
static PRM_Name isDynamicName("isDynamic", "Is Dynamic");
static PRM_Name isWallName("isWall", "Is Wall");
static PRM_Name mapInvertName("mapInvert", "Map Invert");
static PRM_Name mapThicknessName("mapThickness", "Map Thickness");
static PRM_Name mapResolutionName("mapResolution", "Map Resolution");
static PRM_Name denseModeName("denseMode", "Dense Mode");
static PRM_Name startName("start", "Start");
static PRM_Name endName("end", "End");
static PRM_Name translationFBName("translationFB", "Translation");
static PRM_Name scaleFBName("scaleFB", "Scale");
static PRM_Name jsonUpdateName("json_update", "Update JSON");
static PRM_Name simulateButtonName("simulate", "Simulate");

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

	//bool currentCheckboxState = sop->evalInt(jsonUpdateName.getToken(), 0, now) != 0;
	float particleRadius = sop->evalFloat(names[0].getToken(), 0, now);
	int numberOfStepsPerRenderUpdate = sop->evalInt(names[1].getToken(), 0, now);
	float density0 = sop->evalFloat(names[2].getToken(), 0, now);
	int simulationMethod = sop->evalInt(names[3].getToken(), 0, now);
	float timeStepSize = sop->evalFloat(names[5].getToken(), 0, now);
	int cflMethod = sop->evalInt(names[6].getToken(), 0, now);
	float cflFactor = sop->evalFloat(names[7].getToken(), 0, now);
	float cflMaxTimeStepSize = sop->evalFloat(names[8].getToken(), 0, now);
	int maxIterations = sop->evalInt(names[9].getToken(), 0, now);
	float maxError = sop->evalFloat(names[10].getToken(), 0, now);
	int maxIterationsV = sop->evalInt(names[11].getToken(), 0, now);
	float maxErrorV = sop->evalFloat(names[12].getToken(), 0, now);
	float stiffness = sop->evalFloat(names[13].getToken(), 0, now);
	int exponent = sop->evalInt(names[14].getToken(), 0, now);
	int velocityUpdateMethod = sop->evalInt(names[15].getToken(), 0, now);
	int enableDivergenceSolver = sop->evalInt(names[16].getToken(), 0, now); // This is a boolean represented as an int
	int boundaryHandlingMethod = sop->evalInt(names[18].getToken(), 0, now);
	float viscosity = sop->evalFloat(names[19].getToken(), 0, now);
	int viscosityMethod = sop->evalInt(names[20].getToken(), 0, now);
	int colorMapType = sop->evalInt(names[21].getToken(), 0, now);

	// Fetching vector values
	UT_Vector3 gravitation;
	sop->evalFloats(names[4].getToken(), gravitation.data(), now);

	UT_Vector3 translation;
	sop->evalFloats(translationName.getToken(), translation.data(), now);

	UT_Vector3 rotationAxis;
	sop->evalFloats(rotationAxisName.getToken(), rotationAxis.data(), now);

	float rotationAngle = sop->evalFloat(rotationAngleName.getToken(), 0, now);

	UT_Vector3 scale;
	sop->evalFloats(scaleName.getToken(), scale.data(), now);

	UT_Vector4 color;
	sop->evalFloats(colorName.getToken(), color.data(), now); // Assuming RGBA

	// Fetching string (particleAttributes)
	UT_String particleAttributes;
	sop->evalString(particleAttributes, names[17].getToken(), 0, now);

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
		sop->mySim->init(particleRadius, false);
		std::cout << "Simulator initialized." << std::endl;

		// pass parameters to mySim
		sop->mySim->setParticleRadius(particleRadius);
		

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
		PRM_Template(PRM_SWITCHER, 5, &tabName, tabList),
		// Configuration tab
		PRM_Template(PRM_FLT, 1, &names[0], &particleRadiusDefault),
		PRM_Template(PRM_INT, 1, &names[1], &numberOfStepsPerRenderUpdateDefault),
		PRM_Template(PRM_FLT, 1, &names[2], &density0Default),
		PRM_Template(PRM_INT, 1, &names[3], &simulationMethodDefault),
		PRM_Template(PRM_XYZ, 3, &names[4], gravitationDefaults),
		PRM_Template(PRM_FLT, 1, &names[5], &timeStepSizeDefault),
		PRM_Template(PRM_INT, 1, &names[6], &cflMethodDefault),
		PRM_Template(PRM_FLT, 1, &names[7], &cflFactorDefault),
		PRM_Template(PRM_FLT, 1, &names[8], &cflMaxTimeStepSizeDefault),
		PRM_Template(PRM_INT, 1, &names[9], &maxIterationsDefault),
		PRM_Template(PRM_FLT, 1, &names[10], &maxErrorDefault),
		PRM_Template(PRM_INT, 1, &names[11], &maxIterationsVDefault),
		PRM_Template(PRM_FLT, 1, &names[12], &maxErrorVDefault),
		PRM_Template(PRM_FLT, 1, &names[13], &stiffnessDefault),
		PRM_Template(PRM_INT, 1, &names[14], &exponentDefault),
		PRM_Template(PRM_INT, 1, &names[15], &velocityUpdateMethodDefault),
		PRM_Template(PRM_TOGGLE, 1, &names[16], &enableDivergenceSolverDefault),
		PRM_Template(PRM_STRING, 1, &names[17], &particleAttributesDefault),
		PRM_Template(PRM_INT, 1, &names[18], &boundaryHandlingMethodDefault),

		// Materials tab
		PRM_Template(PRM_FLT, 1, &names[19], &viscosityDefault),
		PRM_Template(PRM_INT, 1,  &names[20], &viscosityMethodDefault),
		PRM_Template(PRM_INT, 1,  &names[21], &colorMapTypeDefault),

		// RigidBodies tab
		PRM_Template(PRM_XYZ, 3, &translationName, translationDefaults),
		PRM_Template(PRM_XYZ, 3, &rotationAxisName, rotationAxisDefaults),
		PRM_Template(PRM_FLT, 1, &rotationAngleName, &rotationAngleDefault),
		PRM_Template(PRM_XYZ, 3, &scaleName, scaleDefaults),
		PRM_Template(PRM_RGB, 4, &colorName, colorDefaults), // Use PRM_RGB for color if alpha not needed, else use PRM_RGBA
		PRM_Template(PRM_TOGGLE, 1, &isDynamicName, &isDynamicDefault),
		PRM_Template(PRM_TOGGLE, 1, &isWallName, &isWallDefault),
		PRM_Template(PRM_TOGGLE, 1, &mapInvertName, &mapInvertDefault),
		PRM_Template(PRM_FLT, 1, &mapThicknessName, &mapThicknessDefault),
		PRM_Template(PRM_INT_XYZ, 3, &mapResolutionName, mapResolutionDefaults),

		// FluidBlocks tab
		PRM_Template(PRM_INT, 1, &denseModeName, &denseModeDefault),
		PRM_Template(PRM_XYZ, 3, &startName, startDefaults),
		PRM_Template(PRM_XYZ, 3, &endName, endDefaults),
		PRM_Template(PRM_XYZ, 3, &translationFBName, translationFBDefaults),
		PRM_Template(PRM_XYZ, 3, &scaleFBName, scaleFBDefaults),

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

void SOP_FUILDSIMULATOR::populateParameters(fpreal t) {

	GA_ROHandleS particleRadiusHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "particleRadius"));
	UT_String particleRadius = getParameters(particleRadiusHandle);
	std::cout << "Attribute set to: " << particleRadius.toStdString() << std::endl;
	std::cout << "getToken value is:" << names[0].getToken() << std::endl;
	setString(particleRadius, CH_StringMeaning(), names[0].getToken(), 0, t);
	std::cout << "Attribute set to: " << names[0].getToken() << " in: " << evalFloat(names[0].getToken(), 0, t) << std::endl;

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

	bool currentCheckboxState = evalInt(jsonUpdateName.getToken(), 0, now) != 0;
	float particleRadius = evalFloat(names[0].getToken(), 0, now);
	int numberOfStepsPerRenderUpdate = evalInt(names[1].getToken(), 0, now);
	float density0 = evalFloat(names[2].getToken(), 0, now);
	int simulationMethod = evalInt(names[3].getToken(), 0, now);
	float timeStepSize = evalFloat(names[5].getToken(), 0, now);
	int cflMethod = evalInt(names[6].getToken(), 0, now);
	float cflFactor = evalFloat(names[7].getToken(), 0, now);
	float cflMaxTimeStepSize = evalFloat(names[8].getToken(), 0, now);
	int maxIterations = evalInt(names[9].getToken(), 0, now);
	float maxError = evalFloat(names[10].getToken(), 0, now);
	int maxIterationsV = evalInt(names[11].getToken(), 0, now);
	float maxErrorV = evalFloat(names[12].getToken(), 0, now);
	float stiffness = evalFloat(names[13].getToken(), 0, now);
	int exponent = evalInt(names[14].getToken(), 0, now);
	int velocityUpdateMethod = evalInt(names[15].getToken(), 0, now);
	int enableDivergenceSolver = evalInt(names[16].getToken(), 0, now); // This is a boolean represented as an int
	int boundaryHandlingMethod = evalInt(names[18].getToken(), 0, now);
	float viscosity = evalFloat(names[19].getToken(), 0, now);
	int viscosityMethod = evalInt(names[20].getToken(), 0, now);
	int colorMapType = evalInt(names[21].getToken(), 0, now);

	// Fetching vector values
	UT_Vector3 gravitation;
	evalFloats(names[4].getToken(), gravitation.data(), now);

	UT_Vector3 translation;
	evalFloats(translationName.getToken(), translation.data(), now);

	UT_Vector3 rotationAxis;
	evalFloats(rotationAxisName.getToken(), rotationAxis.data(), now);

	float rotationAngle = evalFloat(rotationAngleName.getToken(), 0, now);

	UT_Vector3 scale;
	evalFloats(scaleName.getToken(), scale.data(), now);

	UT_Vector4 color;
	evalFloats(colorName.getToken(), color.data(), now); // Assuming RGBA

	// Fetching string (particleAttributes)
	UT_String particleAttributes;
	evalString(particleAttributes, names[17].getToken(), 0, now);

	// Assuming "isDynamic", "isWall", "mapInvert" are toggles represented as booleans
	bool isDynamic = evalInt(isDynamicName.getToken(), 0, now) != 0;
	bool isWall = evalInt(isWallName.getToken(), 0, now) != 0;
	bool mapInvert = evalInt(mapInvertName.getToken(), 0, now) != 0;

	// Assuming "denseMode" is an integer
	int denseMode = evalInt(denseModeName.getToken(), 0, now);

	// "start", "end", "translationFB", and "scaleFB" are vectors
	UT_Vector3 start, end, translationFB, scaleFB;
	evalFloats(startName.getToken(), start.data(), now);
	evalFloats(endName.getToken(), end.data(), now);
	evalFloats(translationFBName.getToken(), translationFB.data(), now);
	evalFloats(scaleFBName.getToken(), scaleFB.data(), now);



	// Add gravitation as an array
	fs::path jsonFilePath = fs::absolute("parameters.json");
	if (currentCheckboxState && !lastCheckboxState) {
		// The checkbox was just checked - Output the JSON file
		std::cout << "checked" << std::endl;
		fluid_printCurrentPath();

		std::ostringstream jsonStream;
		jsonStream << std::fixed; // Ensures floating point values are not written in scientific notation.

		jsonStream << "{\n";
		jsonStream << "  \"Configuration\": {\n";
		jsonStream << "    \"particleRadius\": " << particleRadius << ",\n";
		jsonStream << "    \"numberOfStepsPerRenderUpdate\": " << numberOfStepsPerRenderUpdate << ",\n";
		jsonStream << "    \"density0\": " << density0 << ",\n";
		jsonStream << "    \"simulationMethod\": " << simulationMethod << ",\n";
		jsonStream << "    \"timeStepSize\": " << timeStepSize << ",\n";
		jsonStream << "    \"cflMethod\": " << cflMethod << ",\n";
		jsonStream << "    \"cflFactor\": " << cflFactor << ",\n";
		jsonStream << "    \"cflMaxTimeStepSize\": " << cflMaxTimeStepSize << ",\n";
		jsonStream << "    \"maxIterations\": " << maxIterations << ",\n";
		jsonStream << "    \"maxError\": " << maxError << ",\n";
		jsonStream << "    \"maxIterationsV\": " << maxIterationsV << ",\n";
		jsonStream << "    \"maxErrorV\": " << maxErrorV << ",\n";
		jsonStream << "    \"stiffness\": " << stiffness << ",\n";
		jsonStream << "    \"exponent\": " << exponent << ",\n";
		jsonStream << "    \"velocityUpdateMethod\": " << velocityUpdateMethod << ",\n";
		jsonStream << "    \"enableDivergenceSolver\": " << (enableDivergenceSolver ? "true" : "false") << ",\n";
		jsonStream << "    \"boundaryHandlingMethod\": " << boundaryHandlingMethod << ",\n";
		jsonStream << "    \"particleAttributes\": \"" << particleAttributes.toStdString() << "\",\n";
		jsonStream << "    \"gravitation\": [" << gravitation[0] << ", " << gravitation[1] << ", " << gravitation[2] << "]\n";
		jsonStream << "  },\n";

		// Materials Section
		jsonStream << "  \"Materials\": [{\n";
		jsonStream << "    \"id\": \"Fluid\",\n";
		jsonStream << "    \"viscosity\": " << viscosity << ",\n";
		jsonStream << "    \"viscosityMethod\": " << viscosityMethod << ",\n";
		jsonStream << "    \"colorMapType\": " << colorMapType << "\n";
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


		// Your existing code to create or modify geometry goes here.
		// ...

	}
	else if (!currentCheckboxState && lastCheckboxState) {
		// The checkbox was just unchecked - Optionally handle this case
	}

	// Update the last known state
	lastCheckboxState = currentCheckboxState;
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
				std::string particleRadiusStr = std::to_string(particleRadius); // Convert path to std::string
				UT_String outvalue(particleRadiusStr.c_str());
				attrib.set(GA_Offset(0), outvalue); // Replace denseMode with the string variable
				std::cout << "Attribute set to: " << particleRadius << std::endl;
				attrib.bumpDataId(); // This is typically not necessary for setting attribute values
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