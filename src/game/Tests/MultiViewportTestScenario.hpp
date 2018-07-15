//
//  MultiViewportTestScenario.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 4/19/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef MultiViewportTestScenario_hpp
#define MultiViewportTestScenario_hpp

#include "core/Core.hpp"

class MultiViewportTestScenario : public core::Scenario {
public:
    
    MultiViewportTestScenario();
    
    ~MultiViewportTestScenario();
    
    void setup() override;
    void cleanup() override;
    void update(const core::time_state &time) override;    
    void reset();
    
private:
    
    double _scale;
        
};
#endif /* MultiViewportTestScenario_hpp */
