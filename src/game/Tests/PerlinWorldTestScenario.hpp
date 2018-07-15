//
//  PerlinWorldTest.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/25/17.
//

#ifndef PerlinWorldTest_hpp
#define PerlinWorldTest_hpp

#include <cinder/Rand.h>

#include "core/Core.hpp"
#include "elements/Terrain/Terrain.hpp"
#include "game/Tests/util/TerrainCutRecorder.hpp"

class PerlinWorldTestScenario : public core::Scenario {
public:

    PerlinWorldTestScenario();

    virtual ~PerlinWorldTestScenario();

    virtual void setup() override;

    virtual void cleanup() override;

    virtual void clear(const core::render_state &state) override;

    virtual void draw(const core::render_state &state) override;

    virtual bool keyDown(const app::KeyEvent &event) override;

    void reset();

private:

    struct segment {
        dvec2 a, b;
        Color color;
    };

    struct polyline {
        PolyLine2 pl;
        Color color;
    };

    vector <polyline> marchToPerimeters(Channel8u &iso, size_t expectedContourCount) const;
    vector <segment> testMarch(Channel8u &iso) const;
    void onCutPerformed(dvec2 a, dvec2 b, double radius);

private:

    float _surfaceSolidity, _surfaceRoughness;
    int32_t _seed;

    vector <segment> _marchSegments;
    vector <polyline> _marchedPolylines;
    elements::terrain::TerrainObjectRef _terrain;
        
    shared_ptr<TerrainCutRecorder> _terrainCutRecorder;
    bool _recordCuts;
    bool _playingBackRecordedCuts;

};

#endif /* PerlinWorldTest_hpp */
