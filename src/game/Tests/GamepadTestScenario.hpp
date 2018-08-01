//
//  GamepadTestScenario.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 8/1/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef GamepadTestScenario_hpp
#define GamepadTestScenario_hpp

#include <cinder/Rand.h>

#include "core/Core.hpp"

class GamepadTestScenario : public core::Scenario {
public:
    
    GamepadTestScenario();
    
    virtual ~GamepadTestScenario();
    
    virtual void setup() override;
    
    virtual void cleanup() override;
    
    virtual void clear(const core::render_state &state) override;
    
    virtual void draw(const core::render_state &state) override;
    
    virtual void drawScreen(const core::render_state &state) override;
    
    void reset();
    
private:
    
};

#endif /* GamepadTestScenario_hpp */
