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
#include "Compositor.hpp"

namespace core {

    SMART_PTR(Stage)
    SMART_PTR(Scenario)
    SMART_PTR(ViewportComposer)
    
    
    /**
     ViewportComposer acts as manager for a set of viewports to be used to render the contents of a Scenario.
     ViewportComposer also owns the Compositor used to draw the contents of the viewports to screen.
     Also, the ViewportComposer is responsible for resizing the viewports when the owning scenario is resized.
     Special multi-viewport setups might prefer to have each viewport be resized to a fraction of Scenario window
     size.
     */
    class ViewportComposer {
    public:

        ViewportComposer(){}
        ViewportComposer(const BaseViewportRef &viewport, const BaseCompositorRef &compositor);
        ViewportComposer(const vector<BaseViewportRef> &viewports, const BaseCompositorRef &compositor);
        
        const vector<BaseViewportRef> &getViewports() const { return _viewports; }
        const BaseCompositorRef &getCompositor() const { return _compositor; }


        virtual void onScenarioResized(int width, int height);
        virtual void update(const time_state &time);
        
    protected:
        
        vector<BaseViewportRef> _viewports;
        BaseCompositorRef _compositor;
        
    };
    

    class Scenario : public InputListener, public signals::receiver, public enable_shared_from_this<Scenario> {
    public:

        Scenario();

        virtual ~Scenario();

        bool isListening() const override;

        virtual void setup() {}

        virtual void cleanup() {}

        /**
         Called when the application window has resized.
         */
        virtual void windowResized(ivec2 size);

        /**
         Perform fixed-timestep animations (i.e. rigid body physics) here
         */
        virtual void step(const time_state &time);

        /**
         Perform time-based animations which don't require a fixed time step.
         E.g., do your rigid body physics updates in step(), do your time-based animations here.
         */
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
         */
        virtual void drawScreen(const render_state &state);

        /**
         Assign a ViewportComposer to the Scenario which renders the stage contents.
         Scenario has a default ViewportComposer for stage rendering with a single Viewport and ViewportCompositor
         */
        virtual void setViewportComposer(const ViewportComposerRef &composer);
        
        /**
         Get the ViewportComposer used to render the stage
         */
        const ViewportComposerRef &getViewportComposer() const { return _viewportComposer; }
        
        /**
         Get the zeroth camera in the ViewportComposer
         */
        template<typename T>
        shared_ptr<T> getMainViewport() const { return static_pointer_cast<T>(_viewportComposer->getViewports().front()); }

        /**
         Assign a ViewportComposer to the Scenario which renders Screen-space contents.
         Scenario has a default ViewportComposer for screen rendering with a single ScreenViewport and ViewportCompositor
         */
        virtual void setScreenViewportComposer(const ViewportComposerRef &composer);
        
        /**
         Get the ViewportComposer used to render the screen-space contents.
         */
        const ViewportComposerRef &getScreenViewportComposer() const { return _screenViewportComposer; }
        
        /**
         Get the zeroth camera in the screen ViewportComposer
         */
        template<typename T>
        shared_ptr<T> getMainScreenViewport() const { return static_pointer_cast<T>(_screenViewportComposer->getViewports().front()); }
        
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

        virtual void dispatchWindowResize(const ivec2 &size);

        virtual void dispatchStep();

        virtual void dispatchUpdate();

        virtual void dispatchDraw();
        
        virtual void dispatchSceneDraw(const render_state &renderState);

        virtual void dispatchScreenDraw(const render_state &renderState);

    private:

        ViewportComposerRef _viewportComposer;
        ViewportComposerRef _screenViewportComposer;
        
        time_state _time, _stepTime;
        render_state _renderState, _screenRenderState;
        StageRef _stage;
        int _width, _height;

    };

}
#endif /* Scenario_hpp */
