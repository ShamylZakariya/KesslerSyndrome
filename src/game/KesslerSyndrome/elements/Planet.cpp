//
//  Planet.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/6/17.
//

#include "game/KesslerSyndrome/elements/Planet.hpp"

#include "TerrainDetail.hpp"
#include "GameConstants.hpp"

using namespace core;
using namespace elements;
using elements::terrain::dpolygon2;

namespace game {

    namespace {
        
        const int MAP_RESOLUTION = 512;
        int batchId = 0;
    
        planet_generation::params create_planet_generation_params(const Planet::config &surfaceConfig, const Planet::config &coreConfig, dvec2 worldOrigin, double worldRadius) {
            
            planet_generation::params params;
            params.size = MAP_RESOLUTION;

            //
            // compute a transform to put this at origin with requested radius
            //
            
            double halfSize = params.size / 2;
            params.transform = glm::scale( dvec3(worldRadius / halfSize, worldRadius / halfSize, 1)) * glm::translate(dvec3(worldOrigin.x - halfSize, worldOrigin.y - halfSize, 0));

            params.terrain.enabled = true;
            params.terrain.seed = surfaceConfig.seed;
            params.terrain.surfaceSolidity = surfaceConfig.surfaceSolidity;
            params.terrain.surfaceRoughness = surfaceConfig.surfaceRoughness;
            params.terrain.vignetteStart = 0.8;
            params.terrain.vignetteEnd = 0.99;
            params.terrain.pruneFloaters = true;

            params.anchors.enabled = true;
            params.anchors.seed = coreConfig.seed;
            params.anchors.surfaceSolidity = coreConfig.surfaceSolidity;
            params.anchors.surfaceRoughness = coreConfig.surfaceRoughness;
            params.anchors.vignetteStart = 0.3;
            params.anchors.vignetteEnd = 0.7;
            
            return params;
        }
        
        planet_generation::params::perimeter_attachment_params parseAttachmentGenerator(const core::util::xml::XmlMultiTree &node) {
            planet_generation::params::perimeter_attachment_params p(0);
            p.batchId = util::xml::readNumericAttribute<size_t>(node, "batchId", batchId++);
            p.normalToUpDotTarget = util::xml::readNumericAttribute<double>(node, "normalToUpDotTarget", 1);
            p.normalToUpDotRange = util::xml::readNumericAttribute<double>(node, "normalToUpDotRange", 0.75);
            p.probability = util::xml::readNumericAttribute<double>(node, "probability", 0.5);
            p.density = util::xml::readNumericAttribute<size_t>(node, "density", 1);
            p.includeHoleContours = util::xml::readBoolAttribute(node, "includeHoleContours", true);
            return p;
        }
        
        vector<planet_generation::params::perimeter_attachment_params> parseAttachmentGenerators(const core::util::xml::XmlMultiTree &node) {
            vector<planet_generation::params::perimeter_attachment_params> params;
            for (size_t i = 0;;i++) {
                auto generator = node.getChild("generator", i);
                if (!generator) {
                    break;
                }
                params.push_back(parseAttachmentGenerator(generator));
            }
            return params;
        }
        
    }

    Planet::config Planet::config::parse(core::util::xml::XmlMultiTree configNode) {
        config c;
        c.seed = util::xml::readNumericAttribute<int>(configNode, "seed", c.seed);
        c.radius = util::xml::readNumericAttribute<double>(configNode, "radius", c.radius);
        c.surfaceSolidity = util::xml::readNumericAttribute<double>(configNode, "surfaceSolidity", c.surfaceSolidity);
        c.surfaceRoughness = util::xml::readNumericAttribute<double>(configNode, "surfaceRoughness", c.surfaceRoughness);

        return c;
    }

    PlanetRef Planet::create(string name, terrain::WorldRef world, core::util::xml::XmlMultiTree planetNode, int drawLayer) {
        dvec2 origin = util::xml::readPointAttribute(planetNode, "origin", dvec2(0, 0));
        double partitionSize = util::xml::readNumericAttribute<double>(planetNode, "partitionSize", 250);
        config surfaceConfig = config::parse(planetNode.getChild("surface"));
        config coreConfig = config::parse(planetNode.getChild("core"));
        auto attachmentGenerators = parseAttachmentGenerators(planetNode.getChild("attachmentGenerators"));

        return create(name, world, surfaceConfig, coreConfig, attachmentGenerators, origin, partitionSize, drawLayer);
    }

    PlanetRef Planet::create(string name, terrain::WorldRef world, const config &surfaceConfig, const config &coreConfig, const vector<planet_generation::params::perimeter_attachment_params> &attachments, dvec2 origin, double partitionSize, int drawLayer) {
        
        planet_generation::params params = create_planet_generation_params(surfaceConfig, coreConfig, origin, surfaceConfig.radius);
        
        // this is a bit hokey; we need to copy our materials back to the params for generation
        // TODO: a refactoring would let us skip this ugliness and let planet_generation create the World itself
        params.terrain.material = world->getWorldMaterial();
        params.anchors.material = world->getAnchorMaterial();
        params.terrain.partitionSize = partitionSize;
        params.attachments = attachments;

        auto result = planet_generation::generate(params, world);

        // finally create the Planet
        auto planet = PlanetRef(new Planet(name, result.world, result.attachmentsByBatchId, drawLayer));

        planet->_surfaceConfig = surfaceConfig;
        planet->_coreConfig = coreConfig;

        return planet;
    }

    Planet::Planet(string name, terrain::WorldRef world, const map<size_t,vector<terrain::AttachmentRef>> &attachmentsByBatchId, int drawLayer) :
            TerrainObject(name, world, drawLayer),
            _attachmentsByBatchId(attachmentsByBatchId)
    {
    }

    Planet::~Planet() {
    }

    void Planet::onReady(core::StageRef stage) {
        TerrainObject::onReady(stage);
    }
    
    vector<terrain::AttachmentRef> Planet::getAttachmentsByBatchId(size_t batchId) const {
        const auto &pos = _attachmentsByBatchId.find(batchId);
        if (pos != _attachmentsByBatchId.end()) {
            return pos->second;
        }
        
        return vector<terrain::AttachmentRef>();
    }

}
