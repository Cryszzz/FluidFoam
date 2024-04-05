
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
static PRM_Default partioFileDefault(0.0, "D:/PennCGGT/CIS6600/autoringTool/outputs/partio/ParticleData_Fluid");
static PRM_Default colorAttrNameDefault(0.0, "velocity");
static PRM_Default rotationAttrNameDefault(0.0, "v");
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

	// Get the file path from the parameter
	UT_String baseFilePath;
	evalString(baseFilePath, "partioFile", 0, now);

	// Construct the dynamic file path using current frame
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
				//Create a sphere at each particle position
				GEO_Primitive* prim = GU_PrimSphere::build(gdp, /* divisions= */ 10);
				GU_PrimSphere* sphere = dynamic_cast<GU_PrimSphere*>(prim);
				if (sphere)
				{
					UT_Vector3 center(posH);
					UT_Vector4F spherePos(center, 1.0f);
					// Get a reference to the sphere's vertex points
					GEO_Primitive* prim = static_cast<GEO_Primitive*>(sphere);
					GA_Offset startoff = prim->getPointOffset(0);
					GA_Offset endoff = prim->getPointOffset(prim->getVertexCount() - 1);
					// Translate all points of the sphere to the center position
					for (GA_Offset ptoff = startoff; ptoff <= endoff; ++ptoff) {
						UT_Vector4F posH = gdp->getPos4(ptoff); // Get current position in homogeneous coordinates
						posH += spherePos; // Translate to the new position
						gdp->setPos4(ptoff, posH); // Set the new position
					}					
				}
				else {
					// Handle the error
					//std::cerr << "Failed to create sphere primitive" << std::endl;
				}
				//std::cout << "Finished creating particle geometry" << std::endl;






				//// Assuming that m_velAttr is a valid velocity attribute from Partio
				//if (m_velAttr.attributeIndex != -1) {
				//	const float* vel = m_partioData->data<float>(m_velAttr, i);
				//	// Set velocity attribute for the point
				//	GA_RWHandleV3 velocityHandle(gdp->findAttribute(GA_ATTRIB_POINT, "v"));
				//	if (!velocityHandle.isValid()) {
				//		// Create the velocity attribute if it doesn't exist
				//		std::cout << "velocity attribute not found" << std::endl;
				//		//velocityHandle = GA_RWHandleV3(gdp->addFloatTuple(GA_ATTRIB_POINT, "v", 3));
				//	}
				//	velocityHandle.set(ptoff, UT_Vector3(vel[0], vel[1], vel[2]));
				//	//std::cout << "vel: " << vel[0] << " " << vel[1] << " " << vel[2] << std::endl;
				//}

				//// Assuming that m_idAttr is a valid ID attribute from Partio
				//if (m_idAttr.attributeIndex != -1) {
				//	const int* id = m_partioData->data<int>(m_idAttr, i);
				//	// Set id attribute for the point
				//	GA_RWHandleI idHandle(gdp->findAttribute(GA_ATTRIB_POINT, "id"));
				//	if (!idHandle.isValid()) {
				//		// Create the ID attribute if it doesn't exist
				//		std:: cout << "id attribute not found" << std::endl;
				//		//idHandle = GA_RWHandleI(gdp->addIntTuple(GA_ATTRIB_POINT, "id", 1));
				//	}
				//	idHandle.set(ptoff, *id);
				//	//std::cout << "id: " << *id << std::endl;
				//}
				////const float* color = m_partioData->data<float>(m_userColorAttr, i);

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