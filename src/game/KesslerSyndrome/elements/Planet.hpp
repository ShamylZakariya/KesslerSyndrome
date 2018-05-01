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

    public:

        Planet(string name, elements::terrain::WorldRef world, const map<size_t,vector<elements::terrain::AttachmentRef>> &attachmentsByBatchId, int drawLayer);

        virtual ~Planet();

        // Object
        void onReady(core::StageRef stage) override;

        // Planet
        dvec2 getOrigin() const {
            return _origin;
        }
        
        const map<size_t,vector<elements::terrain::AttachmentRef>> &getAttachments() const { return _attachmentsByBatchId; }
        
        vector<elements::terrain::AttachmentRef> getAttachmentsByBatchId(size_t batchId) const;

    private:

        dvec2 _origin;
        map<size_t,vector<elements::terrain::AttachmentRef>> _attachmentsByBatchId;

    };


    SMART_PTR(CrackGeometry);

    class CrackGeometry {
    public:
        virtual ~CrackGeometry() {
        }

        virtual cpBB getBB() const = 0;

        virtual elements::terrain::dpolygon2 getPolygon() const = 0;

        virtual void debugDrawSkeleton() const {
        }

    protected:

        static elements::terrain::dpolygon2 lineSegmentToPolygon(dvec2 a, dvec2 b, double width);

    };

    class RadialCrackGeometry : public CrackGeometry {
    public:

        RadialCrackGeometry(dvec2 origin, int numSpokes, int numRings, double radius, double thickness, double variance);

        cpBB getBB() const override {
            return _bb;
        }

        elements::terrain::dpolygon2 getPolygon() const override {
            return _polygon;
        }

        void debugDrawSkeleton() const override;

    private:

        double _thickness;
        dvec2 _origin;
        vector<dvec2> _vertices;
        vector<vector<pair<size_t, size_t>>> _spokes, _rings;
        elements::terrain::dpolygon2 _polygon;
        cpBB _bb;

        elements::terrain::dpolygon2 createPolygon() const;

        elements::terrain::dpolygon2 createPolygon(const pair<std::size_t, std::size_t> &segment) const;
    };

    class CrackGeometryDrawComponent : public core::DrawComponent {
    public:

        CrackGeometryDrawComponent(const CrackGeometryRef &crackGeometry);

        ~CrackGeometryDrawComponent() {
        }

        cpBB getBB() const override;

        void draw(const core::render_state &renderState) override;

        core::VisibilityDetermination::style getVisibilityDetermination() const override;

        int getLayer() const override;

    private:
        CrackGeometryRef _crackGeometry;
        TriMeshRef _trimesh;
    };


}

#endif /* Planet_hpp */
