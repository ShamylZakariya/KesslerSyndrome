//
//  PerlinWorldTest.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/25/17.
//

#ifndef PerlinWorldTest_hpp
#define PerlinWorldTest_hpp

#include <cinder/Rand.h>

#include "Core.hpp"

#include "Terrain.hpp"
#include "TerrainCutRecorder.hpp"


using namespace ci;
using namespace core;

class PerlinWorldTestScenario : public Scenario {
public:

    PerlinWorldTestScenario();

    virtual ~PerlinWorldTestScenario();

    virtual void setup() override;

    virtual void cleanup() override;

    virtual void clear(const render_state &state) override;

    virtual void draw(const render_state &state) override;

    virtual bool keyDown(const ci::app::KeyEvent &event) override;

    void reset();

private:

    struct segment {
        dvec2 a, b;
        ci::Color color;
    };

    struct polyline {
        ci::PolyLine2 pl;
        ci::Color color;
    };

    vector <polyline> marchToPerimeters(ci::Channel8u &iso, size_t expectedContourCount) const;
    vector <segment> testMarch(ci::Channel8u &iso) const;
    void onCutPerformed(dvec2 a, dvec2 b, double radius);

private:

    float _surfaceSolidity, _surfaceRoughness;
    int32_t _seed;

    vector <segment> _marchSegments;
    vector <polyline> _marchedPolylines;
    terrain::TerrainObjectRef _terrain;
        
    shared_ptr<TerrainCutRecorder> _terrainCutRecorder;
    bool _recordCuts;
    bool _playingBackRecordedCuts;

};

#endif /* PerlinWorldTest_hpp */
