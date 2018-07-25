//
//  Scenario.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#include "core/Scenario.hpp"
#include "core/Stage.hpp"
#include "core/Strings.hpp"

namespace core {

    namespace {

        const seconds_t STEP_INTERVAL(1.0 / 60.0);

        void update_time(time_state &time) {
            const seconds_t
                    Now = time_state::now(),
                    Elapsed = Now - time.time;

            //
            // We're using a variable timestep, but I'm damping its changes heavily
            //

            time.time = Now;
            time.deltaT = lrp<seconds_t>(0.05, time.deltaT, Elapsed);
            time.step++;
        }
        
        ViewportComposerRef create_stage_viewport_composer() {
            auto viewport = make_shared<Viewport>();
            auto compositor = make_shared<ViewportCompositor>(viewport);
            
            // set up an appropriate default blend mode for compositing a solid scene render into the context
            compositor->setBlend(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);

            return make_shared<ViewportComposer>(viewport, compositor);
        }

        ViewportComposerRef create_screen_viewport_composer() {
            auto viewport = make_shared<ScreenViewport>();
            auto compositor = make_shared<ViewportCompositor>(viewport);
            
            // set up an appropriate blend mode for UI compositing atop the scene
            compositor->setBlend(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

            return make_shared<ViewportComposer>(viewport, compositor);
        }

    }
    
#pragma mark - ViewportComposor
    
    ViewportComposer::ViewportComposer(const BaseViewportRef &viewport, const BaseCompositorRef &compositor):
    _viewports({viewport}),
    _compositor(compositor)
    {}

    ViewportComposer::ViewportComposer(const vector<BaseViewportRef> &viewports, const BaseCompositorRef &compositor):
    _viewports(viewports),
    _compositor(compositor)
    {}
    
    void ViewportComposer::onScenarioResized(int width, int height) {
        for (const auto &viewport : _viewports) {
            viewport->setSize(width, height);
        }
    }
    
    void ViewportComposer::update(const time_state &time) {
        _compositor->update(time);
    }
    
#pragma mark - Scenario

    /*
     ViewportComposerRef _viewportComposer;
     ViewportComposerRef _screenViewportComposer;
     
     time_state _time, _stepTime;
     render_state _renderState, _screenRenderState;
     StageRef _stage;
     int _width, _height;
     */

    Scenario::Scenario() :
            InputListener(numeric_limits<int>::max()), // scenario should always be last to receive input after in-game input components
            _viewportComposer(create_stage_viewport_composer()),
            _screenViewportComposer(create_screen_viewport_composer()),
            _time(app::getElapsedSeconds(), 1.0 / 60.0, 1, 0),
            _stepTime(app::getElapsedSeconds(), 1.0 / 60.0, 1, 0),
            _renderState(0, 0, 0, 0),
            _screenRenderState(0, 0, 0, 0),
            _width(app::getWindowWidth()),
            _height(app::getWindowHeight())
    {
        setListening(true);
    }

    Scenario::~Scenario() {
    }

    bool Scenario::isListening() const {
        return InputListener::isListening() && getStage();
    }

    void Scenario::windowResized(ivec2 size) {
    }

    void Scenario::step(const time_state &time) {
    }

    void Scenario::update(const time_state &time) {
    }

    void Scenario::clear(const render_state &state) {
    }

    void Scenario::draw(const render_state &state) {
    }

    void Scenario::drawScreen(const render_state &state) {
    }

    void Scenario::setViewportComposer(const ViewportComposerRef &composer) {
        _viewportComposer = composer;
        _viewportComposer->onScenarioResized(_width, _height);
    }
    
    void Scenario::setScreenViewportComposer(const ViewportComposerRef &composer) {
        _screenViewportComposer = composer;
        _screenViewportComposer->onScenarioResized(_width, _height);
    }
    
    void Scenario::setRenderStateGizmoMask(size_t gizmoMask) {
        _renderState.gizmoMask = _screenRenderState.gizmoMask = gizmoMask;
    }
    
    void Scenario::addRenderStateGizmo(size_t gizmoBit) {
        _renderState.gizmoMask |= gizmoBit;
        _screenRenderState.gizmoMask |= gizmoBit;
    }
    
    void Scenario::removeRenderStateGizmo(size_t gizmoBit) {
        size_t mask = _renderState.gizmoMask;
        _renderState.gizmoMask = _screenRenderState.gizmoMask = mask & ~gizmoBit;
    }

    void Scenario::screenshot(const fs::path &folderPath, const string &namingPrefix, const string format) {
        size_t index = 0;
        fs::path fullPath;

        do {
            fullPath = folderPath / (namingPrefix + str(index++) + "." + format);
        } while (fs::exists(fullPath));

        Surface s = app::copyWindowSurface();
        writeImage(fullPath.string(), s, ImageTarget::Options(), format);
    }

    void Scenario::setStage(StageRef stage) {
        if (_stage) {
            _stage->removeFromScenario();
        }
        _stage = stage;
        if (_stage) {
            _stage->addedToScenario(shared_from_this());
        }
    }

    void Scenario::dispatchSetup() {
        setup();
    }

    void Scenario::dispatchCleanup() {
        cleanup();
    }

    void Scenario::dispatchWindowResize(const ivec2 &size) {
        _width = size.x;
        _height = size.y;

        gl::viewport(0, 0, size.x, size.y);
        _viewportComposer->onScenarioResized(size.x, size.y);
        _screenViewportComposer->onScenarioResized(size.x, size.y);

        if (_stage) {
            _stage->resize(size);
        }

        windowResized(size);
    }

    void Scenario::dispatchStep() {
        update_time(_stepTime);

        // we don't wantphysics steps to vary wildly from the target
        _stepTime.deltaT = clamp<seconds_t>(_stepTime.deltaT, STEP_INTERVAL * 0.9, STEP_INTERVAL * 1.1);

        step(_stepTime);

        if (_stage) {
            _stage->step(_stepTime);
        }
    }

    void Scenario::dispatchUpdate() {
        update_time(_time);

        update(_time);

        if (_stage) {
            _stage->update(_time);
        }

        _viewportComposer->update(_time);
        _screenViewportComposer->update(_time);
    }

    void Scenario::dispatchDraw() {

        //
        // establish render state
        //

        _screenRenderState.frame = _renderState.frame = app::getElapsedFrames();
        _screenRenderState.time = _renderState.time = _time.time;
        _screenRenderState.deltaT = _renderState.deltaT = _time.deltaT;

        //
        //  Dispatch stage rendering
        //

        for (const auto &viewport : _viewportComposer->getViewports()) {
            _renderState.viewport = viewport;
            dispatchSceneDraw(_renderState);
        }

        //
        //  Dispatch screen rendering
        //

        for (const auto &viewport : _screenViewportComposer->getViewports()) {
            _screenRenderState.viewport = viewport;
            dispatchScreenDraw(_screenRenderState);
        }

        //
        //  Now draw to screen
        //
        
        gl::clear(ColorA(0,0,0,1));
        _viewportComposer->getCompositor()->composite(_renderState, _width, _height);
        _screenViewportComposer->getCompositor()->composite(_screenRenderState, _width, _height);
    }
    
    void Scenario::dispatchSceneDraw(const render_state &renderState) {
        gl::ScopedFramebuffer sfbo(renderState.viewport->getFbo());
       
        clear(_renderState);

        gl::ScopedMatrices sm;
        gl::setMatricesWindow(_width, _height, false);
        renderState.viewport->set();
        
        if (_stage) {
            _stage->prepareToDraw(_renderState);
            _stage->draw(_renderState);
        }
        
        draw(_renderState);
    }
    
    void Scenario::dispatchScreenDraw(const render_state &renderState) {
        gl::ScopedFramebuffer sfbo(renderState.viewport->getFbo());

        gl::clear(ColorA(0,0,0,0));
                
        gl::ScopedMatrices sm;
        gl::setMatricesWindow(_width, _height, true);
        renderState.viewport->set();

        gl::ScopedBlendAlpha sba;

        if (_stage) {
            _stage->drawScreen(_screenRenderState);
        }

        drawScreen(_screenRenderState);
    }


}
