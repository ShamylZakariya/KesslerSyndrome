//
//  Planet.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/6/17.
//

#ifndef Planet_hpp
#define Planet_hpp

#include "Terrain.hpp"
#include "Xml.hpp"
#include "PlanetGenerator.hpp"

namespace game {

    SMART_PTR(Planet);

    class Planet : public elements::terrain::TerrainObject {
    public:

        /**
         Description to build a planet
         */
        struct config {

            int seed;
            double radius;
            double surfaceSolidity;
            double surfaceRoughness;

            config() :
                    seed(12345),
                    radius(500),
                    surfaceSolidity(0.5),
                    surfaceRoughness(0.1)
            {
            }

            static config parse(core::util::xml::XmlMultiTree node);

        };

        static PlanetRef create(string name, elements::terrain::WorldRef world, core::util::xml::XmlMultiTree planetNode, int drawLayer);

        static PlanetRef create(string name, elements::terrain::WorldRef world, const config &surfaceConfig, const config &coreConfig, const vector<planet_generation::params::perimeter_attachment_params> &attachments, dvec2 origin, double partitionSize, int drawLayer);

    private:

        Planet(string name, elements::terrain::WorldRef world, const map<size_t,vector<elements::terrain::AttachmentRef>> &attachmentsByBatchId, int drawLayer);

    public:
        
        virtual ~Planet();

        // Object
        void onReady(core::StageRef stage) override;

        // Planet
        
        // get the origin of the planet in world space
        dvec2 getOrigin() const { return _origin; }
        
        // get all attachments as map of attachment id to vector of AttachmentRef
        const map<size_t,vector<elements::terrain::AttachmentRef>> &getAttachments() const { return _attachmentsByBatchId; }
        
        // get all attachments for a given id
        vector<elements::terrain::AttachmentRef> getAttachmentsByBatchId(size_t batchId) const;

        // get the parameters used to generate the surface geometry
        const config &getSurfaceConfig() const { return _surfaceConfig; }
        
        // get the parameters used to generate the core geometry
        const config &getCoreConfig() const { return _coreConfig; }
        
    private:

        dvec2 _origin;
        map<size_t,vector<elements::terrain::AttachmentRef>> _attachmentsByBatchId;
        config _surfaceConfig, _coreConfig;
    };


}

#endif /* Planet_hpp */
