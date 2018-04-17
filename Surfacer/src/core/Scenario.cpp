//
//  Scenario.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#include "Scenario.hpp"

#include "Stage.hpp"
#include "Strings.hpp"

using namespace ci;
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

    }

    /*
        ViewportRef _viewport;
        shared_ptr<ScreenViewport> _screenViewport;
        ViewportControllerRef _viewportController;
        time_state _time, _stepTime;
        render_state _renderState, _screenRenderState;
        StageRef _stage;
        int _width, _height;
     */

    Scenario::Scenario() :
            InputListener(numeric_limits<int>::max()), // scenario should always be last to receive input after in-game input components
            _viewport(make_shared<Viewport>()),
            _screenViewport(make_shared<ScreenViewport>()),
            _viewportController(make_shared<ViewportController>(_viewport)),
            _time(app::getElapsedSeconds(), 1.0 / 60.0, 1, 0),
            _stepTime(app::getElapsedSeconds(), 1.0 / 60.0, 1, 0),
            _renderState(_viewport, RenderMode::GAME, 0, 0, 0, 0),
            _screenRenderState(_screenViewport, RenderMode::GAME, 0, 0, 0, 0),
            _width(app::getWindowWidth()),
            _height(app::getWindowHeight()) {
        _viewport->onMotion.connect(this, &Scenario::onViewportMoved);
        _viewport->onBoundsChanged.connect(this, &Scenario::onViewportBoundsChanged);
        setListening(true);
    }

    Scenario::~Scenario() {
    }

    bool Scenario::isListening() const {
        return InputListener::isListening() && getStage();
    }

    void Scenario::resize(ivec2 size) {
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

    void Scenario::setViewportController(ViewportControllerRef vp) {
        _viewportController = vp;
        _viewportController->setViewport(_viewport);
    }

    void Scenario::setRenderMode(RenderMode::mode mode) {
        _screenRenderState.mode = _renderState.mode = mode;
        app::console() << "Scenario[" << this << "]::setRenderMode: " << RenderMode::toString(getRenderMode()) << endl;
    }

    void Scenario::screenshot(const ci::fs::path &folderPath, const string &namingPrefix, const string format) {
        size_t index = 0;
        ci::fs::path fullPath;

        do {
            fullPath = folderPath / (namingPrefix + str(index++) + "." + format);
        } while (ci::fs::exists(fullPath));

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

    void Scenario::dispatchResize(const ivec2 &size) {
        _width = size.x;
        _height = size.y;

        gl::viewport(0, 0, size.x, size.y);
        _viewport->setSize(size.x, size.y);
        _screenViewport->setSize(size.x, size.y);

        if (_stage) {
            _stage->resize(size);
        }

        resize(size);
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

        _viewportController->update(_time);
    }

    void Scenario::dispatchDraw() {
        _screenRenderState.frame = _renderState.frame = app::getElapsedFrames();
        _screenRenderState.pass = _renderState.pass = 0;
        _screenRenderState.time = _renderState.time = _time.time;
        _screenRenderState.deltaT = _renderState.deltaT = _time.deltaT;

        clear(_renderState);

        if (_stage) {
            gl::ScopedMatrices sm;
            gl::setMatricesWindow(_width, _height, false);
            _viewport->set();

            _stage->draw(_renderState);
            draw(_renderState);
        }

        gl::ScopedMatrices sm;
        gl::setMatricesWindow(_width, _height, true);

        drawScreen(_screenRenderState);

        if (_stage) {
            _stage->drawScreen(_screenRenderState);
        }
    }

    void Scenario::onViewportMoved(const Viewport &vp) {
        // forward Viewport::onMotion to Stage::signals.onViewportMotion
        if (_stage) {
            _stage->signals.onViewportMotion(vp);
        }
    }

    void Scenario::onViewportBoundsChanged(const Viewport &vp) {
        // forward Viewport::onBoundsChanged to Stage::signals.onViewportBoundsChanged
        if (_stage) {
            _stage->signals.onViewportBoundsChanged(vp);
        }
    }


}
