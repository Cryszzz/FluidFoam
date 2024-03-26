#include <UT/UT_DSOVersion.h>
//#include <RE/RE_EGLServer.h>


#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <UT/UT_Debug.h>
#include <UT/UT_WorkBuffer.h>

#include <limits.h>
#include "PluginMain.h"
#include "PartioEmitter.h"

using namespace HDK_Sample;

///
/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case we add ourselves
/// to the specified operator table.
///
void
newSopOperator(OP_OperatorTable* table)
{
	table->addOperator(
		new OP_Operator("CusPartioEmitter",			// Internal name
			"MyPartioEmitter",			// UI name
			SOP_PartioEmitter::myConstructor,	// How to build the SOP
			SOP_PartioEmitter::myTemplateList,	// My parameters
			0,				// Min # of sources
			10,				// Max # of sources
			SOP_PartioEmitter::myVariables,	// Local variables
			OP_FLAG_GENERATOR)		// Flag it as generator
	);
}

// Declare parameters' name for the SOP
static PRM_Name partioFile("partioFile", "Partio File Path");
static PRM_Name colorAttrName("colorAttrName", "Color Attribute Name");
static PRM_Name rotationAttrName("rotationAttrName", "Rotation Attribute Name");
static PRM_Name minVal("minVal", "Min Value");
static PRM_Name maxVal("maxVal", "Max Value");
static PRM_Name frameIndex("frameIndex", "Frame Index");

// Declare parameters' default value for the SOP
static PRM_Default partioFileDefault(0.0, "D:/PennCGGT/CIS6600/autoringTool/outputs/partio/ParticleData_Fluid_1.bgeo");
static PRM_Default colorAttrNameDefault(0.0, "velocity");
static PRM_Default rotationAttrNameDefault(0.0, "v");
static PRM_Default minValDefault(0.0);
static PRM_Default maxValDefault(1.0);
static PRM_Default frameIndexDefault(0.0);


PRM_Template
SOP_PartioEmitter::myTemplateList[] = {
	{PRM_Template(PRM_FILE, 1, &partioFile, &partioFileDefault, 0)},


	PRM_Template()
};

// Here's how we define local variables for the SOP.
enum {
	VAR_PT,		// Point number of the star
	VAR_NPT		// Number of points in the star
};

CH_LocalVariable
SOP_PartioEmitter::myVariables[] = {
	{ "PT",	VAR_PT, 0 },		// The table provides a mapping
	{ "NPT",	VAR_NPT, 0 },		// from text string to integer token
	{ 0, 0, 0 },
};

bool
SOP_PartioEmitter::evalVariableValue(fpreal& val, int index, int thread)
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
SOP_PartioEmitter::myConstructor(OP_Network* net,
	const char* name,
	OP_Operator* op)
{
	return new SOP_PartioEmitter(net, name, op);
}

SOP_PartioEmitter::SOP_PartioEmitter(OP_Network* net,
	const char* name,
	OP_Operator* op)
	: SOP_Node(net, name, op)
{
	myCurrPoint = -1;
}

SOP_PartioEmitter::~SOP_PartioEmitter()
{
}

unsigned
SOP_PartioEmitter::disableParms()
{
	unsigned changes = 0;
	return changes;
}

OP_ERROR
SOP_PartioEmitter::cookMySop(OP_Context& context)
{
	fpreal	now = context.getTime();
	
	// Check to see that there hasn't been a critical error in cooking the SOP.
	if (error() < UT_ERROR_ABORT)
	{
		UT_Interrupt* boss;
		boss = UTgetInterrupt();

		gdp->clearAndDestroy();

		// Start the interrupt server
		if (boss->opStart("Building"))
		{
			// PUT YOUR CODE HERE



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

bool
SOP_PartioEmitter::readParticles(const std::string& fileName, const std::string& colorAttrName, const std::string& rotationAttrName) {
	if (m_partioData != nullptr)
		m_partioData->release();

	m_partioData = Partio::read(fileName.c_str());
	if (!m_partioData)
		return false;

	m_userColorAttr.attributeIndex = -1;
	m_userRotationAttr.attributeIndex = -1;
	m_velAttr.attributeIndex = -1;
	m_idAttr.attributeIndex = -1;
	for (int i = 0; i < m_partioData->numAttributes(); i++)
	{
		Partio::ParticleAttribute attr;
		m_partioData->attributeInfo(i, attr);
		if (attr.name == "position")
			m_posAttr = attr;
		else if (attr.name == "velocity")
			m_velAttr = attr;
		else if (attr.name == "id")
			m_idAttr = attr;

		if (attr.name == colorAttrName)
			m_userColorAttr = attr;
		if (attr.name == rotationAttrName)
			m_userRotationAttr = attr;
	}


	return true;
}