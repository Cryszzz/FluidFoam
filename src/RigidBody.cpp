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
#include "RigidBody.h"
using namespace HDK_Sample;
namespace fs = std::filesystem;

#include <cstdlib> // For std::system

static PRM_Name wallName("is_Wall", "Wall");
static PRM_Default wallDefault = PRM_Default(false);
static PRM_Name inputPathName("Rigid_obj_path", "Rigidbody File Path");
static PRM_Default inputPathDefault(0, "");

PRM_Template
SOP_RIGIDBODY::myTemplateList[] = {
// PUT YOUR CODE HERE
// You now need to fill this template with your parameter name and their default value
// EXAMPLE : For the angle parameter this is how you should add into the template
// PRM_Template(PRM_FLT,	PRM_Template::PRM_EXPORT_MIN, 1, &angleName, &angleDefault, 0),
// Similarly add all the other parameters in the template format here
    PRM_Template(PRM_TOGGLE, 1, &wallName, &wallDefault), 
    PRM_Template(
        PRM_FILE,                             // Parameter type for directories
        1,                                   // Number of elements (1 for a single directory path)
        &inputPathName,                   // Parameter name
        &inputPathDefault                 // Default value
    ),
	PRM_Template() // Sentinel
};


// Here's how we define local variables for the SOP.
enum {
	VAR_PT,		// Point number of the star
	VAR_NPT		// Number of points in the star
};

CH_LocalVariable
SOP_RIGIDBODY::myVariables[] = {
    { "PT",	VAR_PT, 0 },		// The table provides a mapping
    { "NPT",	VAR_NPT, 0 },		// from text string to integer token
    { 0, 0, 0 },
};

bool
SOP_RIGIDBODY::evalVariableValue(fpreal &val, int index, int thread)
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
SOP_RIGIDBODY::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_RIGIDBODY(net, name, op);
}

SOP_RIGIDBODY::SOP_RIGIDBODY(OP_Network *net, const char *name, OP_Operator *op)
	: SOP_Node(net, name, op)
{
    myCurrPoint = -1;	// To prevent garbage values from being returned
}

SOP_RIGIDBODY::~SOP_RIGIDBODY() {}

unsigned
SOP_RIGIDBODY::disableParms()
{
    return 0;
}

OP_ERROR
SOP_RIGIDBODY::cookMySop(OP_Context &context)
{
	fpreal	now = context.getTime();
    UT_String rigidBodyFilePath;
    evalString(rigidBodyFilePath,inputPathName.getToken(), 0, now);
    bool isWall = evalInt(wallName.getToken(), 0, now) != 0;
    // std::cout << isWall << std::endl;
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
		
        GA_RWHandleS attribRigidBodyFilePath(gdp->findStringTuple(GA_ATTRIB_DETAIL, inputPathName.getToken()));
        if (!attribRigidBodyFilePath.isValid()) {
            attribRigidBodyFilePath = GA_RWHandleS(gdp->addStringTuple(GA_ATTRIB_DETAIL, inputPathName.getToken(), 1));
        }
        std::string rigidBodyFilePathString = rigidBodyFilePath.toStdString();
        attribRigidBodyFilePath.set(GA_Offset(0), rigidBodyFilePathString.c_str());
        attribRigidBodyFilePath.bumpDataId();

        GA_RWHandleI attribIsWall(gdp->findIntTuple(GA_ATTRIB_DETAIL, wallName.getToken()));
        if (!attribIsWall.isValid()) {
            attribIsWall = GA_RWHandleI(gdp->addIntTuple(GA_ATTRIB_DETAIL, wallName.getToken(), 1));
        }
        attribIsWall.set(GA_Offset(0), isWall ? 1 : 0); // Convert boolean to int
        attribIsWall.bumpDataId();


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