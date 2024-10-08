#ifndef __SOP_FUILDSIMULATOR_h__
#define __SOP_FUILDSIMULATOR_h__

//#include <GEO/GEO_Point.h>
//
#include <SOP/SOP_Node.h>
#include <OP/OP_AutoLockInputs.h>
#include "SPlisHSPlasH/Simulation.h"
#include "SPlisHSPlasH/Utilities/SceneLoader.h"
#include "Simulator/SimulatorBase.h"



namespace HDK_Sample {
    class SOP_FUILDSIMULATOR : public SOP_Node
    {
    public:
        static OP_Node* myConstructor(OP_Network*, const char*,
            OP_Operator*);

        /// Stores the description of the interface of the SOP in Houdini.
        /// Each parm template refers to a parameter.
        static PRM_Template		 myTemplateList[];

        /// This optional data stores the list of local variables.
        static CH_LocalVariable	 myVariables[];
        bool lastCheckboxState = false;

        void populateParameters(fpreal t, OP_AutoLockInputs inputs);
        UT_String getParameters(GA_ROHandleS paraHandle);
        static int simulateFluid(void* data, int index, float time, const PRM_Template* tplate);
		void drawParticles(int frame, std::vector<std::vector<std::vector<Vector3r>>>& particles);


        std::unique_ptr<SPH::Simulation> mySim;
        std::unique_ptr<Utilities::SceneLoader> mySceneLoader;
		std::unique_ptr<SPH::SimulatorBase> mySimulator;

		
        std::string mySceneFile;
        std::unique_ptr<std::vector<Vector3r>> my_pos;
        std::unique_ptr<std::vector<Vector3r>> my_vel;
        std::unique_ptr<std::vector<Vector3r>> my_angVel;
		// a vessel to store particle info in each frame
		// usd unique_ptr to avoid memory leak
		// a vector for frame, a vector for particles in each frame, a vector for Vector3r attributes in each particle
		std::unique_ptr<std::vector<std::vector<std::vector<Vector3r>>>> my_particles;

        // output patio file path as parameter in detail string
        UT_String myOutputPath;
        float my_stopAt;


        fpreal lastCookTime;

    protected:

        SOP_FUILDSIMULATOR(OP_Network* net, const char* name, OP_Operator* op);
        virtual ~SOP_FUILDSIMULATOR();

        /// Disable parameters according to other parameters.
        virtual unsigned		 disableParms();


        /// cookMySop does the actual work of the SOP computing, in this
        /// case, a LSYSTEM
        virtual OP_ERROR		 cookMySop(OP_Context& context);

        /// This function is used to lookup local variables that you have
        /// defined specific to your SOP.
        virtual bool		 evalVariableValue(
            fpreal& val,
            int index,
            int thread);
        // Add virtual overload that delegates to the super class to avoid
        // shadow warnings.
        virtual bool		 evalVariableValue(
            UT_String& v,
            int i,
            int thread)
        {
            return evalVariableValue(v, i, thread);
        }

    private:
        /// The following list of accessors simplify evaluating the parameters
        /// of the SOP.

        // NOTE : You can declare local variables here like this  
        int		myCurrPoint;
        int		myTotalPoints;
    };
} // End HDK_Sample namespace

#endif