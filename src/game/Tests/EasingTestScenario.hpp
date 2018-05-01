//
//  EasingTestScenario.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 3/4/18.
//

#ifndef EasingTestScenario_hpp
#define EasingTestScenario_hpp

#include <cinder/Rand.h>

#include "Core.hpp"
#include "Svg.hpp"

#include "Terrain.hpp"
#include "ParticleSystem.hpp"

class EasingTestScenario : public core::Scenario {
public:
    
    EasingTestScenario();
    
    virtual ~EasingTestScenario();
    
    virtual void setup() override;
    
    virtual void cleanup() override;
    
    virtual void clear(const core::render_state &state) override;
    
    virtual void draw(const core::render_state &state) override;
    
    virtual void drawScreen(const core::render_state &state) override;
    
    virtual bool keyDown(const app::KeyEvent &event) override;
    
    void reset();
    
private:
        
};

#endif /* EasingTestScenario_hpp */
