
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <GU/GU_PrimTube.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <PRM/PRM_Conditional.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include "FoamGeneratorNode.h"
#include <OP/OP_AutoLockInputs.h>

using namespace HDK_Sample;
namespace fs = std::filesystem;

#include <cstdlib> // For std::system

static PRM_Name inputDirPathName("inputDirPath", "Input Directory Path");
static PRM_Name outputDirPathName("outputDirPath", "Output Directory Path");
static PRM_Name generateName("generate", "Not Automatic");
static PRM_Name splitTypesName("split", "Split Foam Types");

void clearDirectoryFoam(const std::string& path) {
	try {
		// Create a directory iterator pointing to the start of the directory
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			std::filesystem::remove_all(entry.path());  // Recursively remove each entry
		}
		std::cout << "Directory cleared successfully." << std::endl;
	}
	catch (const std::filesystem::filesystem_error& e) {
		std::cerr << "Error clearing directory: " << e.what() << std::endl;
	}
}

static PRM_Default inputDirPathDefault(0, "C:/Users/cryst/Documents/Upenn/CIS6600/Final/basecode/OUTPUT/partio/ParticleData_Fluid_#.bgeo");
static PRM_Default outputDirPathDefault(0, "C:/Users/cryst/Documents/Upenn/CIS6600/Final/basecode/OUTPUT/foam2/foam_#.bgeo");
static PRM_Default generateDefault(0);
static PRM_Default splitTypesDefault(0);

static PRM_Name radiusName("radius", "Radius");
static PRM_Name buoyancyName("buoyancy", "Buoyancy");
static PRM_Name dragName("drag", "Drag Coefficient");
static PRM_Name foamScaleName("foamScale", "Foam Scale");
static PRM_Name timeStepName("timeStep", "Time Step");

static PRM_Default buoyancyDefault(2.0);
static PRM_Default dragDefault(1.0); // Assuming you need this parameter, as mentioned.
static PRM_Default foamScaleDefault(300);
static PRM_Default radiusDefault(0.025f); // Note the 'f' to indicate a float literal
static PRM_Default timeStepDefault(0.02f);

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

static PRM_Name keLimitsName("ke_limits", "Kinetic Energy limits (min/max)");
static PRM_Default keDefaults[] = { PRM_Default(5), PRM_Default(50) };
static PRM_Conditional disableCondition("generate == 0",PRM_CONDTYPE_HIDE);

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

static PRM_Name simulateButtonName("simulate", "Simulate");
static PRM_Default simulateButtonNameDefault(0);

int SOP_FOAMGENERATOR::simulateFoam(void* data, int index, float time, const PRM_Template* tplate){
    
    SOP_FOAMGENERATOR* sop = static_cast<SOP_FOAMGENERATOR*>(data);
    std::cout << "Button Pressed " <<sop->runCommand.c_str()<< std::endl;
    clearDirectoryFoam(sop->cleanpath);
    int result = system(sop->runCommand.c_str());
    if(result == -1) {
        perror("Error executing command");
    } else {
        std::cout << "Command returned: " << result << std::endl;
    }
    return 0;
}

PRM_Template
SOP_FOAMGENERATOR::myTemplateList[] = {
// PUT YOUR CODE HERE
// You now need to fill this template with your parameter name and their default value
// EXAMPLE : For the angle parameter this is how you should add into the template
// PRM_Template(PRM_FLT,	PRM_Template::PRM_EXPORT_MIN, 1, &angleName, &angleDefault, 0),
// Similarly add all the other parameters in the template format here
    PRM_Template(PRM_CALLBACK, 1, &simulateButtonName, &simulateButtonNameDefault, nullptr, nullptr, &simulateFoam, nullptr),
    PRM_Template(PRM_TOGGLE, 1, &generateName, &generateDefault), // Generate
    //splitTypes
    PRM_Template(PRM_TOGGLE, 1, &splitTypesName, &splitTypesDefault),
    PRM_Template(PRM_FLT, 1, &radiusName, &radiusDefault), // Radius
    PRM_Template(PRM_FLT, 1, &buoyancyName, &buoyancyDefault), // Buoyancy
    PRM_Template(PRM_FLT, 1, &dragName, &dragDefault), // Drag Coefficient
    PRM_Template(PRM_FLT, 1, &foamScaleName, &foamScaleDefault), // Foam Scale
    PRM_Template(PRM_FLT, 1, &timeStepName, &timeStepDefault), // Foam Scale

    PRM_Template(PRM_FLT, 2, &lifeLimitsName, lifeDefaults), // Lifetime limits (min/max)
    PRM_Template(PRM_FLT, 2, &frameLimitsName, frameDefaults), // Frame limits (start/end)

    PRM_Template(PRM_FLT, 1, &taFactorName, &taFactorDefault,nullptr, nullptr,nullptr, nullptr,1,nullptr,&disableCondition), // Trapped air factor
    PRM_Template(PRM_FLT, 2, &taLimitsName, taDefaults), // Trapped air limits (min/max)
    PRM_Template(PRM_FLT, 1, &wcFactorName, &wcFactorDefault), // Wave crest factor
    PRM_Template(PRM_FLT, 2, &wcLimitsName, wcDefaults), // Wave crest limits (min/max)
    PRM_Template(PRM_FLT, 1, &voFactorName, &voFactorDefault), // Vorticity factor
    PRM_Template(PRM_FLT, 2, &voLimitsName, voDefaults), // Vorticity limits (min/max)
    PRM_Template(PRM_FLT, 2, &keLimitsName, keDefaults),

    //PRM_Template(PRM_STRING, 1, &sopStringName,  0, &sopStringMenu),
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

std::string appendString(std::string flag, std::string input){
    return flag+" "+input+" ";
}

// get parameter from the handle
UT_String SOP_FOAMGENERATOR::getParameters(GA_ROHandleS paraHandle) {
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
            return UT_String("");
		}
	}
	else {
		std::cout << "Attribute handle is not valid." << std::endl;
        return UT_String("");
	}
}

OP_ERROR
SOP_FOAMGENERATOR::cookMySop(OP_Context &context)
{
	fpreal	now = context.getTime();

    //std::unordered_map<std::string, std::any> params;

    UT_String inputDirPath, outputDirPath;
    evalString(inputDirPath, inputDirPathName.getToken(), 0, now);
    evalString(outputDirPath, outputDirPathName.getToken(), 0, now);
    //std::cout<<"get here"<<std::endl;
    //params.insert_or_assign("inputDirPath", inputDirPath.toStdString());
    std::string inputPath = inputDirPath.toStdString();
    std::string outputPath = outputDirPath.toStdString();
    //std::cout<<"get here"<<std::endl;
    // Reading and storing toggle (boolean) parameter
    bool generate = evalInt(generateName.getToken(), 0, now)==1;
    bool split=evalInt(splitTypesName.getToken(), 0, now)==1;

    // Reading and storing float parameters
    std::string radius = std::to_string(evalFloat(radiusName.getToken(), 0, now));
    std::string buoyancy = std::to_string(evalFloat(buoyancyName.getToken(), 0, now));
    std::string drag = std::to_string(evalFloat(dragName.getToken(), 0, now));
    std::string foamScale = std::to_string(evalFloat(foamScaleName.getToken(), 0, now));
    std::string timeStep = std::to_string(evalFloat(timeStepName.getToken(), 0, now));

    // Reading and storing float parameters with limits (min/max)
    std::string lifeMin = std::to_string(evalFloat(lifeLimitsName.getToken(), 0, now));
    std::string lifeMax = std::to_string(evalFloat(lifeLimitsName.getToken(), 1, now));

    std::string startFrame = std::to_string(evalInt(frameLimitsName.getToken(), 0, now));
    std::string endFrame = std::to_string(evalInt(frameLimitsName.getToken(), 1, now));

    // Reading and storing float parameters and their limits
    std::string taFactor = std::to_string(evalFloat(taFactorName.getToken(), 0, now));
    std::string taMin = std::to_string(evalFloat(taLimitsName.getToken(), 0, now));
    std::string taMax = std::to_string(evalFloat(taLimitsName.getToken(), 1, now));

    std::string wcFactor = std::to_string(evalFloat(wcFactorName.getToken(), 0, now));
    std::string wcMin = std::to_string(evalFloat(wcLimitsName.getToken(), 0, now));
    std::string wcMax = std::to_string(evalFloat(wcLimitsName.getToken(), 1, now));

    std::string voFactor = std::to_string(evalFloat(voFactorName.getToken(), 0, now));
    std::string voMin = std::to_string(evalFloat(voLimitsName.getToken(), 0, now));
    std::string voMax = std::to_string(evalFloat(voLimitsName.getToken(), 1, now));

    std::string keMin = std::to_string(evalFloat(keLimitsName.getToken(), 0, now));
    std::string keMax = std::to_string(evalFloat(keLimitsName.getToken(), 1, now));
   
    //UT_String foamType;
    //evalString(foamType, sopStringName.getToken(), 0, now);

    runCommand = SOURCE_PATH;
    runCommand += " " + appendString("-s", startFrame) + appendString("-e", endFrame) + appendString("--lifetime", lifeMin + "," + lifeMax) + appendString("-r", radius) + appendString("-b", buoyancy) + appendString("-d", drag) + appendString("-t", timeStep) + appendString("-f", foamScale);
    if(split){
        runCommand+="--splittypes ";
    }
    if(generate){
        runCommand+="--no-auto "+appendString("--ta",taFactor)+appendString("--wc",wcFactor)+appendString("--vo",voFactor)+appendString("--limits",taMin+","+taMax+","+wcMin+","+wcMax+","+voMin+","+voMax+","+keMin+","+keMax);
    }

   
    

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
        OP_AutoLockInputs inputs(this);
		if (inputs.lock(context) >= UT_ERROR_ABORT)
			return error();

		// Duplicate our incoming geometry with the hint that we only
		// altered points.  Thus, if our input was unchanged, we can
	// easily roll back our changes by copying point values.
		duplicatePointSource(0, context);

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
            GA_ROHandleS pathHandle(gdp->findStringTuple(GA_ATTRIB_DETAIL, "fluid_patio_file_path"));
            std::string path = getParameters(pathHandle).toStdString();
            runCommand+=appendString("-i",path+"/partio/ParticleData_Fluid_#.bgeo") + appendString("-o", path+"/foam/foam_#.bgeo");
            cleanpath=path+"/foam";
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