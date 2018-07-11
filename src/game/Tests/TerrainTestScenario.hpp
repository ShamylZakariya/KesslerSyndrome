//
//  TerrainTestScenario.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 3/5/17.
//
//

#ifndef IslandTestScenario_hpp
#define IslandTestScenario_hpp

#include "core/Core.hpp"
#include "elements/Terrain/Terrain.hpp"
#include "elements/Components/ViewportController.hpp"

class TerrainTestScenario : public core::Scenario {
public:

    TerrainTestScenario();

    virtual ~TerrainTestScenario();

    virtual void setup() override;

    virtual void cleanup() override;

    virtual void clear(const core::render_state &state) override;

    virtual void drawScreen(const core::render_state &state) override;

    virtual bool keyDown(const app::KeyEvent &event) override;

    void reset();

private:

    elements::terrain::WorldRef testDistantTerrain();

    elements::terrain::WorldRef testBasicTerrain();

    elements::terrain::WorldRef testComplexTerrain();

    elements::terrain::WorldRef testSimpleAnchors();

    elements::terrain::WorldRef testComplexAnchors();

    elements::terrain::WorldRef testSimplePartitionedTerrain();

    elements::terrain::WorldRef testComplexPartitionedTerrainWithAnchors();

    elements::terrain::WorldRef testSimpleSvgLoad();

    elements::terrain::WorldRef testComplexSvgLoad();

    elements::terrain::WorldRef testFail();

    void timeSpatialIndex();

private:

    elements::terrain::TerrainObjectRef _terrain;
    elements::ViewportControllerRef _viewportController;
};

#endif /* IslandTestScenario_hpp */
