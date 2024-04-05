#ifndef __SOP_VISUALIZER_h__
#define __SOP_VISUALIZER_h__

//#include <GEO/GEO_Point.h>
//
#include <GEO/GEO_AttributeHandle.h>
#include <GEO/GEO_IOTranslator.h>
#include <GEO/GEO_PrimPart.h>
#include <UT/UT_Map.h>
#include <GU/GU_Detail.h>
#include <Partio.h>




namespace HDK_Sample
{
    class SOP_VISUALIZER : public SOP_Node
    {
    public:
        static OP_Node* myConstructor(OP_Network*, const char*,
            OP_Operator*);

        /// Stores the description of the interface of the SOP in Houdini.
        /// Each parm template refers to a parameter.
        static PRM_Template		 myTemplateList[];

        /// This optional data stores the list of local variables.
        static CH_LocalVariable	 myVariables[];

    protected:

        SOP_VISUALIZER(OP_Network* net, const char* name, OP_Operator* op);
        virtual ~SOP_VISUALIZER();

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

        // read the partio file
        bool readParticles(const std::string& fileName, const std::string& colorAttrName, const std::string& rotationAttrName);

    private:
        /// The following list of accessors simplify evaluating the parameters
        /// of the SOP.
        /// 
        /// 

        Partio::ParticlesDataMutable* m_partioData;
        Partio::ParticleAttribute m_posAttr;
        Partio::ParticleAttribute m_velAttr;
        Partio::ParticleAttribute m_idAttr;
        Partio::ParticleAttribute m_userColorAttr;
        Partio::ParticleAttribute m_userRotationAttr;
        std::vector<unsigned int> m_id_to_index;

        UT_String myLastPartioFilePath;

        // get the parameters from current node
        fpreal ATTRIB_COLOR(fpreal t) { return evalFloat("colorAttrName", 0, t); }
        fpreal ATTRIB_ROTATION(fpreal t) { return evalFloat("rotationAttrName", 0, t); }
        fpreal MINVAL(fpreal t) { return evalFloat("minVal", 0, t); }
        fpreal MAXVAL(fpreal t) { return evalFloat("maxVal", 0, t); }
        fpreal FRAME(fpreal t) { return evalFloat("frameIndex", 0, t); }

        //GEO_PrimParticle* mySystem;

        const GU_Detail* mySource;
        GA_Index		 mySourceNum;		// Source point to birth from
        GA_ROHandleV3	 mySourceVel;		// Velocity attrib in source

        //GU_RayIntersect* myCollision;

        GEO_PrimParticle* mySystem;

        fpreal		 myLastCookTime;	// Last cooked time
        GA_RWHandleV3	 myVelocity;		// My velocity attribute
        GA_RWHandleF	 myLife;		// My life attribute

        static int* myOffsets;

        


        ///////////////////////////////////////////////////////////////////////////////////////////////////////////

        /// Member variables are stored in the actual SOP, not with the geometry
        /// In this case these are just used to transfer data to the local 
        /// variable callback.
        /// Another use for local data is a cache to store expensive calculations.

        // NOTE : You can declare local variables here like this  
        int		myCurrPoint;
        int		myTotalPoints;
    };
}

#endif