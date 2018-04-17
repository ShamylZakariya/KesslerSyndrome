//
//  Scenario.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#ifndef Scenario_hpp
#define Scenario_hpp

#include "InputDispatcher.hpp"
#include "Viewport.hpp"
#include "ViewportController.hpp"
#include "RenderState.hpp"
#include "TimeState.hpp"

namespace core {

    SMART_PTR(Stage)

    SMART_PTR(Scenario)

    class Scenario : public InputListener, public signals::receiver, public enable_shared_from_this<Scenario> {
    public:

        Scenario();

        virtual ~Scenario();

        bool isListening() const override;

        virtual void setup() {
        }

        virtual void cleanup() {
        }

        virtual void resize(ivec2 size);

        virtual void step(const time_state &time);

        virtual void update(const time_state &time);

        /**
         Clear the screen. No drawing should be done in here.
         */
        virtual void clear(const render_state &state);

        /**
         performs draw with viewport set with its current scale, look, etc.
         */
        virtual void draw(const render_state &state);

        /**
         performs draw with a top-left 1-to-1 ortho projection suitable for UI.
         Note, there's no ViewportRef available in this pass since it's screen-direct.
         */
        virtual void drawScreen(const render_state &state);

        const ViewportRef &getViewport() const {
            return _viewport;
        }

        // time state used for animation
        const time_state &getTime() const {
            return _time;
        }

        // time state used for physics
        const time_state &getStepTime() const {
            return _stepTime;
        }

        const render_state &getRenderState() const {
            return _renderState;
        }

        void setRenderMode(RenderMode::mode mode);

        RenderMode::mode getRenderMode() const {
            return _renderState.mode;
        }

        /**
         Save a screenshot as PNG to @a path
         */
        void screenshot(const ci::fs::path &folderPath, const std::string &namingPrefix, const std::string format = "png");

        void setStage(StageRef stage);

        StageRef getStage() const {
            return _stage;
        }

    public:

        virtual void dispatchSetup();

        virtual void dispatchCleanup();

        virtual void dispatchResize(const ivec2 &size);

        virtual void dispatchStep();

        virtual void dispatchUpdate();

        virtual void dispatchDraw();

    private:

        ViewportRef _viewport;
        ScreenViewportRef _screenViewport;
        time_state _time, _stepTime;
        render_state _renderState, _screenRenderState;
        StageRef _stage;
        int _width, _height;

    };

}
#endif /* Scenario_hpp */
