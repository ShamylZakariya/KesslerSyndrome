//
//  IPTestsScenario.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 2/22/18.
//

#include "game/Tests/IPTestsScenario.hpp"
#include "core/util/ImageProcessing.hpp"
#include "elements/Components/DevComponents.hpp"

using namespace core;
using namespace elements;

IPTestsScenario::IPTestsScenario() :
        _seed(12345)
{
}

IPTestsScenario::~IPTestsScenario() {
}

void IPTestsScenario::setup() {
    setStage(make_shared<Stage>("Image Processing Tests"));
    
    auto viewportController = make_shared<ViewportController>(getMainViewport<Viewport>());

    getStage()->addObject(Object::with("ViewportControlComponent", {
        viewportController,
        make_shared<MouseViewportControlComponent>(viewportController)
    }));
    
    auto grid = WorldCartesianGridDrawComponent::create(1);
    grid->setFillColor(ColorA(0.2, 0.22, 0.25, 1.0));
    grid->setGridColor(ColorA(1, 1, 1, 0.1));
    getStage()->addObject(Object::with("Grid", {grid}));

    if ((true)) {
        int s = 256;
        {
            StopWatch suite("IP Suite");
            _channels = vector<Channel8u> {
                testBlur(s,s),
                testRemap(s,s),
                testDilate(s,s),
                testErode(s,s),
                testThreshold(s, s),
                testPerlinNoise(s, s),
                testPerlinNoise2(s, s)
            };
        }
        
        const auto fmt = gl::Texture2d::Format().mipmap(false).minFilter(GL_NEAREST).magFilter(GL_NEAREST);
        for(Channel8u channel : _channels) {
            _channelTextures.push_back(gl::Texture2d::create(channel, fmt));
        }
    }
    
    
    getStage()->addObject(Object::with("InputDelegation",{
        elements::KeyboardDelegateComponent::create(0)->onPress([&](int keyCode)->bool{
            switch(keyCode) {

                case app::KeyEvent::KEY_r:
                    this->reset();
                    return true;

                case app::KeyEvent::KEY_s:
                    _seed++;
                    CI_LOG_D("_seed: " << _seed);
                    reset();
                    return true;

                default:
                    return false;
            }
        })
    }));

    
}

void IPTestsScenario::cleanup() {
    setStage(nullptr);
    _channels.clear();
    _channelTextures.clear();
}

void IPTestsScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.2, 0.2));
}

void IPTestsScenario::draw(const render_state &state) {
    Scenario::draw(state);
    
    for (auto tex : _channelTextures) {
        gl::color(ColorA(1, 1, 1, 1));
        gl::draw(tex);
        gl::translate(vec3(tex->getWidth() + 8, 0, 0));
    }
}

void IPTestsScenario::drawScreen(const render_state &state) {

    //
    // NOTE: we're in screen space, with coordinate system origin at top left
    //
    
    float fps = core::App::get()->getAverageFps();
    float sps = core::App::get()->getAverageSps();
    string info = core::strings::format("%.1f %.1f", fps, sps);
    gl::drawString(info, vec2(10, 10), Color(1, 1, 1));
    
    auto viewport = getMainViewport<Viewport>();
    Viewport::look look = viewport->getLook();
    double scale = viewport->getScale();
    
    stringstream ss;
    ss << std::setprecision(3) << "look (" << look.world.x << ", " << look.world.y << ") scale: " << scale << " up: (" << look.up.x << ", " << look.up.y << ")";
    gl::drawString(ss.str(), vec2(10, 40), Color(1, 1, 1));

}

void IPTestsScenario::reset() {
    cleanup();
    setup();
}

Channel8u IPTestsScenario::testRemap(int width, int height) {
    using namespace core::util;
    Channel8u channel = Channel8u(width, height);
    
    StopWatch sw("remap");
    ip::in_place::fill(channel, 128);
    ip::in_place::fill(channel, Area(20,20, 60,60), 200);
    ip::in_place::fill(channel, Area(20,height-60, 60,height-20), 64);
    ip::in_place::remap(channel, 200, 255, 0); // make 200 become 255, everything else 0
    
    return channel;
}

Channel8u IPTestsScenario::testDilate(int width, int height) {
    using namespace core::util;
    Channel8u channel = Channel8u(width, height);
    
    StopWatch sw("dilate");
    ip::in_place::fill(channel, 0);
    ip::in_place::fill(channel, Area(width/2 - 20, height/2 - 20, width/2 + 20, height/2+20), 255);
    channel = ip::dilate(channel, 24);
    
    return channel;
}

Channel8u IPTestsScenario::testErode(int width, int height) {
    using namespace core::util;
    Channel8u channel = Channel8u(width, height);
    
    StopWatch sw("erode");
    ip::in_place::fill(channel, 0);
    ip::in_place::fill(channel, Area(width/2 - 20, height/2 - 20, width/2 + 20, height/2+20), 255);
    channel = ip::erode(channel, 24);

    return channel;
}

Channel8u IPTestsScenario::testThreshold(int width, int height) {
    using namespace core::util;
    Channel8u channel = Channel8u(width, height);
    
    StopWatch sw("erode");
    ip::in_place::fill(channel, 0);
    ip::in_place::fill(channel, Area(width/2 - 20, height/2 - 20, width/2 + 20, height/2+20), 255);
    ip::in_place::fill(channel, Area(20,20, 60,60), 128);
    ip::in_place::fill(channel, Area(20,height-60, 60,height-20), 64);
    ip::in_place::threshold(channel, 100, 255, 0);
    
    return channel;
}

Channel8u IPTestsScenario::testPerlinNoise(int width, int height) {
    using namespace core::util;
    Channel8u channel = Channel8u(width, height);
    
    Perlin pn(8,123);
    double frequency = 8.0 / width;
    StopWatch sw("perlin");
    ip::in_place::perlin(channel, pn, frequency);
    
    return channel;
}

Channel8u IPTestsScenario::testPerlinNoise2(int width, int height) {
    using namespace core::util;
    Channel8u channel = Channel8u(width, height);
    
    Perlin pn(8,123);
    double frequency = 8.0 / width;
    StopWatch sw("perlin_abs_thresh");
    ip::in_place::perlin_abs_thresh(channel, pn, frequency, 12);
    
    return channel;
}

Channel8u IPTestsScenario::testBlur(int width, int height) {
    using namespace core::util;
    Channel8u channel = Channel8u(width, height);
    
    Perlin pn(8,123);
    double frequency = 8.0 / width;
    ip::in_place::perlin(channel, pn, frequency);
    ip::in_place::threshold(channel);
    
    StopWatch sw("blur");
    channel = ip::blur(channel, 24);
    
    
    return channel;
}
