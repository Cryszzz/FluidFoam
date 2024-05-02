
#include <SOP/SOP_Node.h>
#include <UT/UT_Math.h>
#include <UT/UT_Interrupt.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPoly.h>
#include <GU/GU_PrimTube.h>
#include <GU/GU_PrimSphere.h>
#include <CH/CH_LocalVariable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <UT/UT_Map.h>
#include <GU/GU_Detail.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include "Visualizer.h"
using namespace HDK_Sample;
namespace fs = std::filesystem;

#include <cstdlib> // For std::system



// Declare parameters' name for the SOP
static PRM_Name partioFile("partioFile", "Partio File Path");
static PRM_Name colorAttrName("colorAttrName", "Color Attribute Name");
static PRM_Name rotationAttrName("rotationAttrName", "Rotation Attribute Name");
static PRM_Name minVal("minVal", "Min Value");
static PRM_Name maxVal("maxVal", "Max Value");
static PRM_Name frameIndex("frameIndex", "Frame Index");

// Declare parameters' default value for the SOP
static PRM_Default partioFileDefault(0.0, "");
static PRM_Default colorAttrNameDefault(0.0, "");
static PRM_Default rotationAttrNameDefault(0.0, "");
static PRM_Default minValDefault(0.0);
static PRM_Default maxValDefault(1.0);
static PRM_Default frameIndexDefault(1.0);


PRM_Template
SOP_VISUALIZER::myTemplateList[] = {
	{PRM_Template(PRM_FILE, 1, &partioFile, &partioFileDefault, 0)},
	{PRM_Template(PRM_STRING, 1, &colorAttrName, &colorAttrNameDefault, 0)},
	{PRM_Template(PRM_STRING, 1, &rotationAttrName, &rotationAttrNameDefault, 0)},
	{PRM_Template(PRM_FLT,	PRM_Template::PRM_EXPORT_MIN, 1, &minVal, &minValDefault, 0)},
	{PRM_Template(PRM_FLT, PRM_Template::PRM_EXPORT_MIN, 1, &maxVal, &maxValDefault, 0)},
	{PRM_Template(PRM_FLT, PRM_Template::PRM_EXPORT_MIN, 1, &frameIndex, &frameIndexDefault, 0)},


	PRM_Template()
};

// Here's how we define local variables for the SOP.
enum {
	VAR_PT,		// Point number of the star
	VAR_NPT		// Number of points in the star
};

CH_LocalVariable
SOP_VISUALIZER::myVariables[] = {
	{ "PT",	VAR_PT, 0 },		// The table provides a mapping
	{ "NPT",	VAR_NPT, 0 },		// from text string to integer token
	{ 0, 0, 0 },
};

bool
SOP_VISUALIZER::evalVariableValue(fpreal& val, int index, int thread)
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
SOP_VISUALIZER::myConstructor(OP_Network* net,
	const char* name,
	OP_Operator* op)
{
	return new SOP_VISUALIZER(net, name, op);
}

SOP_VISUALIZER::SOP_VISUALIZER(OP_Network* net,
	const char* name,
	OP_Operator* op)
	: SOP_Node(net, name, op)
{
	myCurrPoint = -1;
}

SOP_VISUALIZER::~SOP_VISUALIZER()
{
}

unsigned
SOP_VISUALIZER::disableParms()
{
	unsigned changes = 0;
	return changes;
}

// get parameter from the handle
UT_String SOP_VISUALIZER::getParameters(GA_ROHandleS paraHandle) {
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

OP_ERROR
SOP_VISUALIZER::cookMySop(OP_Context& context)
{
	std::cout << "Cooking SOP" << std::endl;
	flags().setTimeDep(true);
	//duplicatePointSource(0, context);
	fpreal	now = context.getTime();
	//fpreal fps = CHgetManager()->getFPS();
	//int currentFrame = static_cast<int>(round(now * fps));
	int currentFrame = context.getFrame();
	std::cout << "Current frame: " << currentFrame << std::endl;
	// Get the frame index from the parameter
	int frameIndex = currentFrame;

	

	// Check to see that there hasn't been a critical error in cooking the SOP.
	if (error() < UT_ERROR_ABORT)
	{
		std::cout << "no error" << std::endl;
		UT_Interrupt* boss;
		boss = UTgetInterrupt();

		gdp->clearAndDestroy();

		// Start the interrupt server
		if (boss->opStart("Building"))
		{
			// Get the file path from the parameter
			UT_String baseFilePath;
			evalString(baseFilePath, "partioFile", 0, now);

			// get the patio file path from the detail list
			UT_String PATIO_FILE_PATH;
			GA_ROHandleS file_path_handle(gdp->findAttribute(GA_ATTRIB_DETAIL, "fluid_patio_file_path"));
			if (file_path_handle.isValid()) {
				UT_StringHolder value = file_path_handle.get(GA_Offset(0));
				std::cout << "File path: " << value.toStdString() << std::endl;
			}
			else {
				file_path_handle = GA_RWHandleS(gdp->addStringTuple(GA_ATTRIB_DETAIL, "fluid_patio_file_path", 1));
			}
			if (file_path_handle.isValid()) {
				//file_path_handle.set(GA_Offset(0), "myOutputPath");
			}
			//std::cout << "Base file path: " << PATIO_FILE_PATH.toStdString() << std::endl;


			// Construct the dynamic file path using current frame
			// check if the file exists
			UT_String partioFilePath;
			partioFilePath.sprintf("%s_%d.bgeo", baseFilePath.toStdString().c_str(), frameIndex);

			std::cout << "Partio file path: " << partioFilePath.toStdString() << std::endl;
			// Get the color attribute name from the parameter
			UT_String colorAttrName;
			evalString(colorAttrName, "colorAttrName", 0, now);

			// Get the rotation attribute name from the parameter
			UT_String rotationAttrName;
			evalString(rotationAttrName, "rotationAttrName", 0, now);

			// Get the min value from the parameter
			float minVal;
			minVal = MINVAL(now);

			// Get the max value from the parameter
			float maxVal;
			maxVal = MAXVAL(now);

			m_partioData = Partio::create();

			myLastPartioFilePath = partioFilePath;
			if (!readParticles(myLastPartioFilePath.toStdString(), colorAttrName.toStdString(), rotationAttrName.toStdString()))
			{
				addError(SOP_ERR_INVALID_SRC, "Failed to read partio file");
				return error();

			}
			
			float sphereRadius = 0.1f;
			// Create a sphere for each particle
			////// render the particles using the partio data and houdini particles attributes //////
			unsigned int numParticles = m_partioData->numParticles();
			std::cout << "Number of particles: " << numParticles << std::endl;
			//UTdebugFormat("Number of attributes: {}", m_partioData->numAttributes());
			for (unsigned int i = 0; i < numParticles; i++) {
				UT_Vector3 posH;
				if (m_posAttr.attributeIndex != 0xffffffff) {
					const float* pos = m_partioData->data<float>(m_posAttr, i);   // This line is causing the error
					posH = UT_Vector3(pos[0], pos[1], pos[2]);
				}

				GA_Offset ptoff = gdp->appendPointOffset();
				
				gdp->setPos3(ptoff, posH);
				//std::cout << "pos: " << pos[0] << " " << pos[1] << " " << pos[2] << std::endl;

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

bool
SOP_VISUALIZER::readParticles(const std::string& fileName, const std::string& colorAttrName, const std::string& rotationAttrName) {
	std::cout << "Reading Partio data from file: " << fileName << std::endl;
	

	if (m_partioData != nullptr)
		m_partioData->release();

	std::cout << "partioData released" << std::endl;

	m_partioData = Partio::read(fileName.c_str());
	if (!m_partioData) {
		std::cerr << "Failed to read Partio data from file: " << fileName << std::endl;
		return false;
	}

	m_userColorAttr.attributeIndex = -1;
	m_userRotationAttr.attributeIndex = -1;
	m_velAttr.attributeIndex = -1;
	m_idAttr.attributeIndex = -1;
	for (int i = 0; i < m_partioData->numAttributes(); i++)
	{
		
		Partio::ParticleAttribute attr;
		m_partioData->attributeInfo(i, attr);
		if (attr.name == "position") {
			m_posAttr = attr;
			//std::cout << "position attribute found" << std::endl;
		}
		else if (attr.name == "velocity") {
			m_velAttr = attr;
			//std::cout << "velocity attribute found" << std::endl;
		}
		else if (attr.name == "id") {
			m_idAttr = attr;
			//std::cout << "id attribute found" << std::endl;
		}

		if (attr.name == colorAttrName) {
			m_userColorAttr = attr;
			//std::cout << "color attribute found" << std::endl;
		}
		if (attr.name == rotationAttrName) {
			m_userRotationAttr = attr;
			//std::cout << "rotation attribute found" << std::endl;
		}
	}
	std::cout <<  m_partioData->numParticles() << std::endl;

	return true;
}