//
//  FilterStackTestScenario.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 7/18/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef FilterStackTestScenario_hpp
#define FilterStackTestScenario_hpp

#include "core/Core.hpp"
#include "core/util/Svg.hpp"

using namespace core;

class FilterStackTestScenario : public Scenario {
public:
    
    FilterStackTestScenario();
    
    virtual ~FilterStackTestScenario();
    
    virtual void setup() override;
    
    virtual void cleanup() override;
    
    virtual void clear(const render_state &state) override;
    
    virtual void drawScreen(const render_state &state) override;
        
    void reset();
    
};
#endif /* FilterStackTestScenario_hpp */
