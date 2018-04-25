//
//  MultiViewportTestScenario.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/19/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef MultiViewportTestScenario_hpp
#define MultiViewportTestScenario_hpp

#include "Core.hpp"

using namespace ci;
using namespace core;

class MultiViewportTestScenario : public Scenario {
public:
    
    MultiViewportTestScenario();
    
    ~MultiViewportTestScenario();
    
    void setup() override;
    void cleanup() override;
    void update(const time_state &time) override;
    void draw(const render_state &state) override;
    void drawScreen(const render_state &state) override;
    
    void reset();
    
private:
    
    double _scale;
        
};
#endif /* MultiViewportTestScenario_hpp */
