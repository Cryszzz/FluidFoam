
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
#include "FoamGeneratorNode.h"
using namespace HDK_Sample;
namespace fs = std::filesystem;

#include <cstdlib> // For std::system

void runFoamGenerator(int startFrame, int endFrame, float radius, int foamscale, std::string inputFile, std::string outputFile, float buoyancy, float drag, float lifemin, float lifemax) {
    fs::path inputPath = fs::absolute(inputFile);
    fs::path outputPath = fs::absolute(outputFile);

    // Update the original string variables
    inputFile = inputFile;
    outputFile = outputFile;
    // Use std::to_string to convert numeric values to strings and build the command
    std::string command = "FoamGenerator -s " + std::to_string(startFrame) +
                          " -e " + std::to_string(endFrame) +
                          " -r " + std::to_string(radius) +
                          " --foamscale " + std::to_string(foamscale) +
                          " -i " + inputFile +
                          " -o " + outputFile +
                          " --buoyancy " + std::to_string(buoyancy) +
                          " --drag " + std::to_string(drag) ;

    std::cout << command << std::endl;
    int result = std::system(command.c_str());

    // Check the result and respond accordingly
    if (result != 0) {
        std::cerr << "FoamGenerator command failed with code: " << result << std::endl;
    }
}
static PRM_Name inputDirPathName("inputDirPath", "Input Directory Path");
static PRM_Name outputDirPathName("outputDirPath", "Output Directory Path");
static PRM_Name buoyancyName("buoyancy", "Buoyancy");
static PRM_Name dragName("drag", "Drag Coefficient");
static PRM_Name foamScaleName("foamScale", "Foam Scale");
static PRM_Name lifeMinName("lifeMin", "Lifetime Min");
static PRM_Name lifeMaxName("lifeMax", "Lifetime Max");
static PRM_Name startFrameName("startFrame", "Start Frame");
static PRM_Name endFrameName("endFrame", "End Frame");
static PRM_Name radiusName("radius", "Radius");
static PRM_Name generateName("generate", "Generate");

static PRM_Default inputDirPathDefault(0, "C:/Documents/Upenn/CIS6600/FINAL/Output/partio");
static PRM_Default outputDirPathDefault(0, "C:/Documents/Upenn/CIS6600/FINAL/Output/foam");
static PRM_Default buoyancyDefault(2.0);
static PRM_Default dragDefault(1.0); // Assuming you need this parameter, as mentioned.
static PRM_Default foamScaleDefault(1000);
static PRM_Default lifeMinDefault(2.0);
static PRM_Default lifeMaxDefault(5.0);
static PRM_Default startFrameDefault(1);
static PRM_Default endFrameDefault(500);
static PRM_Default radiusDefault(0.025f); // Note the 'f' to indicate a float literal
static PRM_Default generateDefault(0);

PRM_Template
SOP_FOAMGENERATOR::myTemplateList[] = {
// PUT YOUR CODE HERE
// You now need to fill this template with your parameter name and their default value
// EXAMPLE : For the angle parameter this is how you should add into the template
// PRM_Template(PRM_FLT,	PRM_Template::PRM_EXPORT_MIN, 1, &angleName, &angleDefault, 0),
// Similarly add all the other parameters in the template format here
    PRM_Template(PRM_TOGGLE, 1, &generateName, &generateDefault),
	PRM_Template(PRM_INT, 1, &startFrameName, &startFrameDefault),
    PRM_Template(PRM_INT, 1, &endFrameName, &endFrameDefault),
    PRM_Template(PRM_FLT, 1, &radiusName, &radiusDefault),
    PRM_Template(
        PRM_FLT, 1, &buoyancyName, &buoyancyDefault
    ),
    // Assuming you need a parameter for drag as well
    PRM_Template(
        PRM_FLT, 1, &dragName, &dragDefault
    ),
    PRM_Template(
        PRM_INT, 1, &foamScaleName, &foamScaleDefault
    ),
    // Lifetime Min/Max as two separate parameters
    PRM_Template(
        PRM_FLT, 1, &lifeMinName, &lifeMinDefault
    ),
    PRM_Template(
        PRM_FLT, 1, &lifeMaxName, &lifeMaxDefault
    ),
    PRM_Template(
        PRM_FILE,                             // Parameter type for directories
        1,                                   // Number of elements (1 for a single directory path)
        &inputDirPathName,                   // Parameter name
        &inputDirPathDefault                 // Default value
    ),
    PRM_Template(
        PRM_FILE,                             // Parameter type for directories
        1,                                   // Number of elements
        &outputDirPathName,                  // Parameter name
        &outputDirPathDefault                // Default value
    ),
	PRM_Template() // Sentinel
};


// Here's how we define local variables for the SOP.
enum {
	VAR_PT,		// Point number of the star
	VAR_NPT		// Number of points in the star
};

CH_LocalVariable
SOP_FOAMGENERATOR::myVariables[] = {
    { "PT",	VAR_PT, 0 },		// The table provides a mapping
    { "NPT",	VAR_NPT, 0 },		// from text string to integer token
    { 0, 0, 0 },
};

bool
SOP_FOAMGENERATOR::evalVariableValue(fpreal &val, int index, int thread)
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
SOP_FOAMGENERATOR::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_FOAMGENERATOR(net, name, op);
}

SOP_FOAMGENERATOR::SOP_FOAMGENERATOR(OP_Network *net, const char *name, OP_Operator *op)
	: SOP_Node(net, name, op)
{
    myCurrPoint = -1;	// To prevent garbage values from being returned
}

SOP_FOAMGENERATOR::~SOP_FOAMGENERATOR() {}

unsigned
SOP_FOAMGENERATOR::disableParms()
{
    return 0;
}

OP_ERROR
SOP_FOAMGENERATOR::cookMySop(OP_Context &context)
{
	fpreal	now = context.getTime();

    bool currentCheckboxState = evalInt(generateName.getToken(), 0, now) != 0;
	UT_String inputDir, outputDir;
    evalString(inputDir, inputDirPathName.getToken(), 0, now);
    evalString(outputDir, outputDirPathName.getToken(), 0, now);
    inputDir+="/ParticleData_Fluid_#.bgeo";
    outputDir+="/Foam_#.bgeo";
	std::cout << inputDir.toStdString() << " " << outputDir.toStdString() << std::endl;
	// Now that you have all the branches ,which is the start and end point of each point ,its time to render 
	// these branches into Houdini 
    float buoyancy = evalFloat(buoyancyName.getToken(), 0, now);

    // Assuming you have a drag parameter
    float drag = evalFloat(dragName.getToken(), 0, now);

    // Fetch the foamScale value
    int foamScale = evalInt(foamScaleName.getToken(), 0, now);

    // Fetch the lifetime min and max values
    float lifeMin = evalFloat(lifeMinName.getToken(), 0, now);
    float lifeMax = evalFloat(lifeMaxName.getToken(), 0, now);
    int startFrame = evalInt(startFrameName.getToken(), 0, now);
    int endFrame = evalInt(endFrameName.getToken(), 0, now);
    float radius = evalFloat(radiusName.getToken(), 0, now);
    if (currentCheckboxState && !lastCheckboxState) {
        runFoamGenerator(startFrame, endFrame, radius, foamScale, inputDir.toStdString(), outputDir.toStdString(),buoyancy,drag, lifeMin, lifeMax);
    }
    lastCheckboxState = currentCheckboxState;

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
	if (divisions < 4)
	{
	    // With the range restriction we have on the divisions, this
	    //	is actually impossible, but it shows how to add an error
	    //	message or warning to the SOP.
	    addWarning(SOP_MESSAGE, "Invalid divisions");
	    divisions = 4;
	}
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