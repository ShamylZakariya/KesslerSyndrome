//
//  CharacterTestScenario.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 8/6/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef CharacterTestScenario_hpp
#define CharacterTestScenario_hpp

#include "core/Core.hpp"
#include "elements/Terrain/Terrain.hpp"
#include "elements/Components/ViewportController.hpp"

class CharacterTestScenario : public core::Scenario {
public:
    
    CharacterTestScenario();
    
    virtual ~CharacterTestScenario();
    
    virtual void setup() override;
    
    virtual void cleanup() override;
    
    virtual void clear(const core::render_state &state) override;
    
    virtual void drawScreen(const core::render_state &state) override;
    
    void reset();
    
private:
    
    elements::terrain::WorldRef loadLevelSvg(string levelSvgAsset);
    
private:
    
    elements::ViewportControllerRef _viewportController;

};
#endif /* CharacterTestScenario_hpp */
