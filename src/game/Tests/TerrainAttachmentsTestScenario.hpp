//
//  TerrainAttachmentsTestScenario.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 2/8/18.
//

#ifndef TerrainAttachmentsTestScenario_hpp
#define TerrainAttachmentsTestScenario_hpp

#include "Terrain.hpp"
#include "Scenario.hpp"

class TerrainAttachmentsTestScenario : public core::Scenario {
public:
    
    TerrainAttachmentsTestScenario();
    
    virtual ~TerrainAttachmentsTestScenario();
    
    virtual void setup() override;
    
    virtual void cleanup() override;
    
    virtual void clear(const core::render_state &state) override;
    
    virtual void drawScreen(const core::render_state &state) override;
    
    virtual bool keyDown(const ci::app::KeyEvent &event) override;
    
    void reset();
    
private:
    
    elements::terrain::WorldRef testBasicTerrain();
    elements::terrain::WorldRef testBasicAttachmentAdapter();
    
private:
    
    elements::terrain::TerrainObjectRef _terrain;
};

#endif /* TerrainAttachmentsTestScenario_hpp */
