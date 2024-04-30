
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
//#include "FoamGenerationMain.h"

using namespace HDK_Sample;
namespace fs = std::filesystem;

#include <cstdlib> // For std::system

static PRM_Name inputDirPathName("inputDirPath", "Input Directory Path");
static PRM_Name outputDirPathName("outputDirPath", "Output Directory Path");
static PRM_Name generateName("generate", "Generate");

static PRM_Default inputDirPathDefault(0, "C:/Documents/Upenn/CIS6600/FINAL/Output/partio");
static PRM_Default outputDirPathDefault(0, "C:/Documents/Upenn/CIS6600/FINAL/Output/foam");
static PRM_Default generateDefault(0);

static PRM_Name radiusName("radius", "Radius");
static PRM_Name buoyancyName("buoyancy", "Buoyancy");
static PRM_Name dragName("drag", "Drag Coefficient");
static PRM_Name foamScaleName("foamScale", "Foam Scale");

static PRM_Default buoyancyDefault(2.0);
static PRM_Default dragDefault(1.0); // Assuming you need this parameter, as mentioned.
static PRM_Default foamScaleDefault(300);
static PRM_Default radiusDefault(0.025f); // Note the 'f' to indicate a float literal


static PRM_Name lifeLimitsName("life_limits", "Lifetime limits (min/max)");
static PRM_Default lifeDefaults[] = { PRM_Default(2), PRM_Default(5) };

static PRM_Name frameLimitsName("frame_limits", "Frame limits (start/end)");
static PRM_Default frameDefaults[] = { PRM_Default(1), PRM_Default(100) };

static PRM_Name taFactorName("ta", "Trapped air factor");
static PRM_Name taLimitsName("ta_limits", "Trapped air limits (min/max)");
static PRM_Default taFactorDefault(4000);
static PRM_Default taDefaults[] = { PRM_Default(5), PRM_Default(20) };

static PRM_Name wcFactorName("wc", "Wave crest factor");
static PRM_Name wcLimitsName("wc_limits", "Wave crest limits (min/max)");
static PRM_Default wcFactorDefault(50000);
static PRM_Default wcDefaults[] = { PRM_Default(2), PRM_Default(8) }; 

static PRM_Name voFactorName("vo", "Vorticity factor");
static PRM_Name voLimitsName("vo_limits", "Vorticity limits (min/max)");
static PRM_Default voFactorDefault(4000);
static PRM_Default voDefaults[] = { PRM_Default(5), PRM_Default(20) };

static PRM_Name splitGeneratorsName("splitgenerators", "Output different foam files depending on which potential generated the foam");
static PRM_Name trappedAirGeneratorName("splitTrappedAir", "Trapped Air Generator");
static PRM_Name waveCrestGeneratorName("splitWaveCrest", "Wave Crest Generator");
static PRM_Name vorticityGeneratorName("splitVorticity", "Vorticity Generator");

static PRM_Name         sopStringName("splittypes", "Output foam type");
static PRM_Name         sopStrChoices[] =
{
    PRM_Name("splitFoam", "Foam Type"),
    PRM_Name("splitSpray", "Spray Type"),
    PRM_Name("splitBubbles", "Bubbles Type"),
    PRM_Name(0)
};
static PRM_ChoiceList   sopStringMenu(PRM_CHOICELIST_TOGGLE, sopStrChoices);

PRM_Template
SOP_FOAMGENERATOR::myTemplateList[] = {
// PUT YOUR CODE HERE
// You now need to fill this template with your parameter name and their default value
// EXAMPLE : For the angle parameter this is how you should add into the template
// PRM_Template(PRM_FLT,	PRM_Template::PRM_EXPORT_MIN, 1, &angleName, &angleDefault, 0),
// Similarly add all the other parameters in the template format here
    PRM_Template(PRM_STRING, 1, &inputDirPathName, &inputDirPathDefault), // Input Directory Path
    PRM_Template(PRM_STRING, 1, &outputDirPathName, &outputDirPathDefault), // Output Directory Path
    PRM_Template(PRM_TOGGLE, 1, &generateName, &generateDefault), // Generate
    
    PRM_Template(PRM_FLT, 1, &radiusName, &radiusDefault), // Radius
    PRM_Template(PRM_FLT, 1, &buoyancyName, &buoyancyDefault), // Buoyancy
    PRM_Template(PRM_FLT, 1, &dragName, &dragDefault), // Drag Coefficient
    PRM_Template(PRM_FLT, 1, &foamScaleName, &foamScaleDefault), // Foam Scale

    PRM_Template(PRM_FLT, 2, &lifeLimitsName, lifeDefaults), // Lifetime limits (min/max)
    PRM_Template(PRM_FLT, 2, &frameLimitsName, frameDefaults), // Frame limits (start/end)
    PRM_Template(PRM_ORD, 1, &taFactorName, &taFactorDefault), // Trapped air factor
    PRM_Template(PRM_FLT, 2, &taLimitsName, taDefaults), // Trapped air limits (min/max)
    PRM_Template(PRM_ORD, 1, &wcFactorName, &wcFactorDefault), // Wave crest factor
    PRM_Template(PRM_FLT, 2, &wcLimitsName, wcDefaults), // Wave crest limits (min/max)
    PRM_Template(PRM_ORD, 1, &voFactorName, &voFactorDefault), // Vorticity factor
    PRM_Template(PRM_FLT, 2, &voLimitsName, voDefaults), // Vorticity limits (min/max)

    PRM_Template(PRM_STRING, 1, &sopStringName,  0, &sopStringMenu),
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

    std::unordered_map<std::string, std::any> params;

    UT_String inputDirPath, outputDirPath;
    evalString(inputDirPath, inputDirPathName.getToken(), 0, now);
    evalString(outputDirPath, outputDirPathName.getToken(), 0, now);
    
    /*params["inputDirPath"] = inputDirPath.toStdString();
    params["outputDirPath"] = outputDirPath.toStdString();

    // Reading and storing toggle (boolean) parameter
    params["generate"] = evalInt(generateName.getToken(), 0, now);

    // Reading and storing float parameters
    params["radius"] = evalFloat(radiusName.getToken(), 0, now);
    params["buoyancy"] = evalFloat(buoyancyName.getToken(), 0, now);
    params["drag"] = evalFloat(dragName.getToken(), 0, now);
    params["foamScale"] = evalFloat(foamScaleName.getToken(), 0, now);

    // Reading and storing float parameters with limits (min/max)
    params["lifeMin"] = evalFloat(lifeLimitsName.getToken(), 0, now);
    params["lifeMax"] = evalFloat(lifeLimitsName.getToken(), 1, now);

    params["startFrame"] = evalFloat(frameLimitsName.getToken(), 0, now);
    params["endFrame"] = evalFloat(frameLimitsName.getToken(), 1, now);

    // Reading and storing ORD parameters and their limits
    params["taFactor"] = evalInt(taFactorName.getToken(), 0, now);
    params["taMin"] = evalFloat(taLimitsName.getToken(), 0, now);
    params["taMax"] = evalFloat(taLimitsName.getToken(), 1, now);

    params["wcFactor"] = evalInt(wcFactorName.getToken(), 0, now);
    params["wcMin"] = evalFloat(wcLimitsName.getToken(), 0, now);
    params["wcMax"] = evalFloat(wcLimitsName.getToken(), 1, now);

    params["voFactor"] = evalInt(voFactorName.getToken(), 0, now);
    params["voMin"] = evalFloat(voLimitsName.getToken(), 0, now);
    params["voMax"] = evalFloat(voLimitsName.getToken(), 1, now);*/
    
    
    //runSimulationFromNode(params); 

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