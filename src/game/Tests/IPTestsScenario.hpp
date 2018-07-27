//
//  IPTestsScenario.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 2/22/18.
//

#ifndef IPTestsScenario_hpp
#define IPTestsScenario_hpp

#include <cinder/Rand.h>

#include "core/Core.hpp"

class IPTestsScenario : public core::Scenario {
public:
    
    IPTestsScenario();
    
    virtual ~IPTestsScenario();
    
    virtual void setup() override;
    
    virtual void cleanup() override;
        
    virtual void clear(const core::render_state &state) override;
    
    virtual void draw(const core::render_state &state) override;
    
    virtual void drawScreen(const core::render_state &state) override;
        
    void reset();

private:

    Channel8u testRemap(int width, int height);
    Channel8u testDilate(int width, int height);
    Channel8u testErode(int width, int height);
    Channel8u testThreshold(int width, int height);
    Channel8u testPerlinNoise(int width, int height);
    Channel8u testPerlinNoise2(int width, int height);
    Channel8u testBlur(int width, int height);

private:
    
    vector<Channel8u> _channels;
    vector<gl::Texture2dRef> _channelTextures;
    int32_t _seed;
        
};
#endif /* IPTestsScenario_hpp */
