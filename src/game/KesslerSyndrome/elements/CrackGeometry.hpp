//
//  CrackGeometry.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/2/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef CrackGeometry_hpp
#define CrackGeometry_hpp

#include "core/Core.hpp"
#include "elements/Terrain/Terrain.hpp"

namespace game {
    
    SMART_PTR(CrackGeometry);
    
    class CrackGeometry {
    public:
        virtual ~CrackGeometry() {
        }
        
        virtual cpBB getBB() const = 0;
        
        virtual vector<elements::terrain::dpolygon2> getPolygons() const = 0;
        
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
        
        vector<elements::terrain::dpolygon2> getPolygons() const override {
            return _polygons;
        }
        
        void debugDrawSkeleton() const override;
        
    private:

        elements::terrain::dpolygon2 createPolygon() const;
        elements::terrain::dpolygon2 createPolygon(const pair<std::size_t, std::size_t> &segment) const;
        
    private:
        
        double _thickness;
        dvec2 _origin;
        vector<dvec2> _vertices;
        vector<vector<pair<size_t, size_t>>> _spokes, _rings;
        vector<elements::terrain::dpolygon2> _polygons;
        cpBB _bb;
        
    };
    
    class ExplosionCrackGeometry : public CrackGeometry {
    public:
        
        ExplosionCrackGeometry(dvec2 origin, double innerRadius, double outerRadius, int numInnerSlices, int numOuterSlices, double crackThickness, double variance);

        cpBB getBB() const override {
            return _bb;
        }
        
        vector<elements::terrain::dpolygon2> getPolygons() const override {
            return _polygons;
        }
        
        void debugDrawSkeleton() const override;

    private:
        
        void createPolygon();
        
    private:
        
        double _innerRadius, _outerRadius, _crackThickness, _variance;
        int _numOuterSlices, _numInnerSlices;
        dvec2 _origin;
        vector<pair<dvec2,dvec2>> _segments;
        vector<dvec2> _hole;
        vector<elements::terrain::dpolygon2> _polygons;
        cpBB _bb;
        
    };
    
    class CrackGeometryDrawComponent : public core::DrawComponent {
    public:
        
        CrackGeometryDrawComponent(const CrackGeometryRef &crackGeometry);
        
        ~CrackGeometryDrawComponent() {
        }
        
        cpBB getBB() const override;
        
        void draw(const core::render_state &renderState) override;
        
    private:
        CrackGeometryRef _crackGeometry;
        TriMeshRef _trimesh;
    };
    
}

#endif /* CrackGeometry_hpp */
