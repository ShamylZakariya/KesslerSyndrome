//
//  PlanetGenerator.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 2/1/18.
//

#ifndef PlanetGenerator_hpp
#define PlanetGenerator_hpp


#include "Core.hpp"
#include "TerrainWorld.hpp"

namespace game { namespace planet_generation {
    
    /**
     Parameters to tune planet generation
    */
    struct params {
        
        // size of map
        int size;

        // world is generated by default 1 to 1 mapping to pixels of the generated maps. Scale and or translate
        // to move the result geoemtry to a more useful position. See params::defaultCenteringTransform
        dmat4 transform;
        
        struct generation_params {
            // random seed to start with
            int seed;
            
            // number of perlin noise octaves
            int noiseOctaves;
            
            // scale frequency of perlin noise sampling
            double noiseFrequencyScale;
            
            // Value of zero results in a very wispy planet surface, and 1 results in a very solid surface
            double surfaceSolidity;

            // Value of zero results in smooth planet surface, value of 1 roughens. Range [0,1]
            double surfaceRoughness;

            double vignetteStart;
            
            double vignetteEnd;
            

            generation_params():
            seed(12345),
            noiseOctaves(4),
            noiseFrequencyScale(1),
            surfaceSolidity(1),
            vignetteStart(0.9),
            vignetteEnd(1),
            surfaceRoughness(0.15)
            {}
        };

        struct terrain_params : public generation_params {
            // if true, terrain will be generated
            bool enabled;
            
            // if > 0 the terrain will be partitioned, which can improve draw/cut performance
            int partitionSize;
            
            // if true, terrain (not anchors) will be pruned of all but the biggest solid geometry
            bool pruneFloaters;
            
            // material to apply to terrain
            elements::terrain::material material;
            
            terrain_params(bool enabled=true):
            enabled(enabled),
            partitionSize(0),
            pruneFloaters(true)
            {}

        } terrain;
        
        
        struct anchor_params : public generation_params {
            // if true, anchors will be generated
            bool enabled;
            
            // material to apply to anchors
            elements::terrain::material material;
            
            anchor_params(bool enabled=true):
            enabled(enabled)
            {}
        } anchors;
        
        
        struct perimeter_attachment_params {
            
            // scale probability by the closeness of the dot of surface normal to local up
            double normalToUpDotTarget;
            double normalToUpDotRange;
            
            // dice roll probability of a valid surface point getting a planting. Range [0,+1]
            double probability;
            
            // number of possible plantings per unit distance
            double density;
            
            // if non-zero, attachment generation will end when maxCount is reached
            size_t maxCount;
            
            // if true, hole contours will be included as well as the primary outer contour
            bool includeHoleContours;
            
            // id to query later to get the generated attachments
            size_t batchId;

            perimeter_attachment_params(size_t batchId):
            normalToUpDotTarget(1),
            normalToUpDotRange(1),
            probability(1),
            density(1),
            maxCount(0),
            includeHoleContours(true),
            batchId(batchId)
            {}
            
            perimeter_attachment_params(size_t batchId, double normalToUpDotTarget, double normalToUpDotRange, double probability, double density, size_t maxCount, bool includeHoleContours):
            normalToUpDotTarget(normalToUpDotTarget),
            normalToUpDotRange(normalToUpDotRange),
            probability(probability),
            density(density),
            maxCount(maxCount),
            includeHoleContours(includeHoleContours),
            batchId(batchId)
            {}

        };
        
        // each attachment_params added will cause an attachment generating pass on the generated terrain;
        vector<perimeter_attachment_params> attachments;
        
        params(int size=512):
        size(size)
        {}
        
        /**
         apply a default centering transform that centers the generated geometry at the origin, with an optional scaling factor
         */
        params &defaultCenteringTransform(double scale = 1) {
            this->transform = glm::scale(dvec3(scale, scale, 1)) * glm::translate(dvec3(-size/2, -size/2, 0));
            return *this;
        }
        
        // get the origin of whatever planet geometry will be created
        dvec2 origin() const {
            return transform * dvec2(size/2, size/2);
        }
        
        // return the inner radius of terrain geometry in world coords
        double terrainInnerRadius() const {
            dvec2 bitmapCoord(terrain.vignetteStart * size/2 + size/2, size/2);
            dvec2 worldCoord = transform * bitmapCoord;
            return distance(worldCoord, origin());
        }

        // return the outer radius of terrain geometry in world coords
        double terrainOuterRadius() const {
            dvec2 bitmapCoord(terrain.vignetteEnd * size/2 + size/2, size/2);
            dvec2 worldCoord = transform * bitmapCoord;
            return distance(worldCoord, origin());
        }

        // return the inner radius of anchor geometry in world coords
        double anchorInnerRadius() const {
            dvec2 bitmapCoord(anchors.vignetteStart * size/2 + size/2, size/2);
            dvec2 worldCoord = transform * bitmapCoord;
            return distance(worldCoord, origin());
        }
        
        // return the outer radius of anchor geometry in world coords
        double anchorOuterRadius() const {
            dvec2 bitmapCoord(anchors.vignetteEnd * size/2 + size/2, size/2);
            dvec2 worldCoord = transform * bitmapCoord;
            return distance(worldCoord, origin());
        }

    };
    
    namespace detail {

        /**
         Generate a map given the provided parameters
         */
        Channel8u generate_map(const params::generation_params &p, int size);

        /**
         Generate just terrain map from given generation parameters,
         and populate terrain::Shape vector with the marched/triangulated geometry.
         return terrain map
         */
        ci::Channel8u generate_shapes(const params &p, vector <elements::terrain::ShapeRef> &shapes);

        /**
         Generate just anchors and anchor map from given generation parameters,
         and populate terrain::Shape vector with the marched/triangulated geometry.
         return terrain map
         */
        ci::Channel8u generate_anchors(const params &p, vector <elements::terrain::AnchorRef> &anchors);

    }
    
    struct result {
        elements::terrain::WorldRef world;
        ci::Channel8u terrainMap;
        ci::Channel8u anchorMap;
        map<size_t,vector<elements::terrain::AttachmentRef>> attachmentsByBatchId;
    };

    /**
     Generate a terrin::World with given generation parameters; generates both terrain and anchor geometry.
     */
    result generate(const params &params, elements::terrain::WorldRef world);

    /**
     Generate a terrin::World with given generation parameters; generates both terrain and anchor geometry.
     */
    result generate(const params &params, core::SpaceAccessRef space);
    
}} // end namespace game::planet_generation

#endif /* PlanetGenerator_hpp */
