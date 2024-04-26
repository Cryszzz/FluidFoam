
//#include <RE/RE_EGLServer.h>

#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <UT/UT_DSOVersion.h>
#include <limits.h>
#include "Plugin.h"

using namespace HDK_Sample;
//
// Help is stored in a "wiki" style text file. 
//
// See the sample_install.sh file for an example.
//
// NOTE : Follow this tutorial if you have any problems setting up your visual studio 2008 for Houdini 
//  http://www.apileofgrains.nl/setting-up-the-hdk-for-houdini-12-with-visual-studio-2008/


///
/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case we add ourselves
/// to the specified operator table.
///
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(
	    new OP_Operator("CusSPHSimulator",			// Internal name
			    "Simulator",			// UI name
				 SOP_SIMULATOR::myConstructor,	// How to build the SOP
				 SOP_SIMULATOR::myTemplateList,	// My parameters
			     0,				// Min # of sources
			     0,				// Max # of sources
				 SOP_SIMULATOR::myVariables,	// Local variables
			     OP_FLAG_GENERATOR)		// Flag it as generator
	    );
	table->addOperator(
		new OP_Operator("CusFoamGenerator",			// Internal name
			"FoamGenerator",			// UI name
			SOP_FOAMGENERATOR::myConstructor,	// How to build the SOP
			SOP_FOAMGENERATOR::myTemplateList,	// My parameters
			0,				// Min # of sources
			0,				// Max # of sources
			SOP_FOAMGENERATOR::myVariables,	// Local variables
			OP_FLAG_GENERATOR)		// Flag it as generator
	);
	table->addOperator(
		new OP_Operator("CusFluidConfiguration",			// Internal name
			"FluidConfiguration",			// UI name
			SOP_FLUIDCONFIGURATION::myConstructor,	// How to build the SOP
			SOP_FLUIDCONFIGURATION::myTemplateList,	// My parameters
			0,				// Min # of sources
			0,				// Max # of sources
			SOP_FLUIDCONFIGURATION::myVariables,	// Local variables
			OP_FLAG_GENERATOR)		// Flag it as generator
	);
	table->addOperator(
		new OP_Operator("CusRigidBody",			// Internal name
			"RigidBody",			// UI name
			SOP_RIGIDBODY::myConstructor,	// How to build the SOP
			SOP_RIGIDBODY::myTemplateList,	// My parameters
			0,				// Min # of sources
			0,				// Max # of sources
			SOP_RIGIDBODY::myVariables,	// Local variables
			OP_FLAG_GENERATOR)		// Flag it as generator
	);
	table->addOperator(
		new OP_Operator("CusVisualizer",			// Internal name
			"MyVisualizer",			// UI name
			SOP_VISUALIZER::myConstructor,	// How to build the SOP
			SOP_VISUALIZER::myTemplateList,	// My parameters
			0,				// Min # of sources
			10,				// Max # of sources
			SOP_VISUALIZER::myVariables,	// Local variables
			OP_FLAG_GENERATOR)		// Flag it as generator
	);
}



