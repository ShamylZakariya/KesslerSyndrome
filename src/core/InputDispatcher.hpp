//
//  InputDispatcher.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#ifndef InputDispatcher_hpp
#define InputDispatcher_hpp

#include <cinder/app/App.h>
#include <cinder/Vector.h>
#include <cinder/app/MouseEvent.h>
#include <cinder/app/KeyEvent.h>
#include <cinder/app/FileDropEvent.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#include <OIS.h>
#pragma clang diagnostic pop

#include "core/Common.hpp"
#include "core/MathHelpers.hpp"

namespace core {

    class InputListener;

    SMART_PTR(InputDispatcher);
    SMART_PTR(Gamepad);

    namespace ScreenCoordinateSystem {
        enum origin {
            TopLeft,
            BottomLeft
        };
    }
    
    /**
     Gamepad
     Represents a bog-standard PS3/4 type controller, with twin analog sticks, analog shoulder triggers,
     and ABXY buttons, etc.

     The Gamepad interface is designed for polling, not event subscription.
     
     Get a Gamepad from InputDispatcher::getGamepads()
     */
    class Gamepad : public OIS::JoyStickListener {
    public:
        
        virtual ~Gamepad();
        
        /// get the id of this controller, suitable for distinguishing between N attached controllers
        int getId() const;
        
        /// get the underlying IOS::JoyStick
        OIS::JoyStick *getJoystick() const { return _joystick; }
        
        // Current stick/trigger/button state
        
        dvec2 getLeftStickValue() const { return _currentState.leftStickValue; }
        dvec2 getRightStickValue() const { return _currentState.rightStickValue; }
        
        bool getL1ButtonValue() const { return _currentState.l1Button; }
        double getL2TriggerValue() const { return _currentState.l2Trigger; }

        bool getR1ButtonValue() const { return _currentState.r1Button; }
        double getR2TriggerValue() const { return _currentState.r2Trigger; }
        
        bool getAButtonValue() const { return _currentState.aButton; } // PS Circle Button
        bool getBButtonValue() const { return _currentState.bButton; } // PS X Button
        bool getXButtonValue() const { return _currentState.xButton; } // PS Triangle Button
        bool getYButtonValue() const { return _currentState.yButton; } // PS Square Button

        bool getStartButtonValue() const { return _currentState.startButton; } // PS Options Button
        bool getSelectButtonValue() const { return _currentState.selectButton; } // PS Share Button
        
        // Check if buttons were pressed on this step but not before
        
        bool getL1ButtonWasPressed() const {
            return _currentState.l1Button && !_previousState.l1Button;
        }

        bool getR1ButtonWasPressed() const {
            return _currentState.r1Button && !_previousState.r1Button;
        }
        
        bool getAButtonWasPressed() const {
            return _currentState.aButton && !_previousState.aButton;
        }

        bool getBButtonWasPressed() const {
            return _currentState.bButton && !_previousState.bButton;
        }

        bool getXButtonWasPressed() const {
            return _currentState.xButton && !_previousState.xButton;
        }

        bool getYButtonWasPressed() const {
            return _currentState.yButton && !_previousState.yButton;
        }

        bool getStartButtonWasPressed() const {
            return _currentState.startButton && !_previousState.startButton;
        }

        bool getSelectButtonWasPressed() const {
            return _currentState.selectButton && !_previousState.selectButton;
        }
        
        // Check if buttons were release on this step but not before
        
        bool getL1ButtonWasReleased() const {
            return !_currentState.l1Button && _previousState.l1Button;
        }
        
        bool getR1ButtonWasReleased() const {
            return !_currentState.r1Button && _previousState.r1Button;
        }
        
        bool getAButtonWasReleased() const {
            return !_currentState.aButton && _previousState.aButton;
        }
        
        bool getBButtonWasReleased() const {
            return !_currentState.bButton && _previousState.bButton;
        }
        
        bool getXButtonWasReleased() const {
            return !_currentState.xButton && _previousState.xButton;
        }
        
        bool getYButtonWasReleased() const {
            return !_currentState.yButton && _previousState.yButton;
        }
        
        bool getStartButtonWasReleased() const {
            return !_currentState.startButton && _previousState.startButton;
        }
        
        bool getSelectButtonWasReleased() const {
            return !_currentState.selectButton && _previousState.selectButton;
        }
        
        // OIS::JoyStickListener
        
        bool buttonPressed(const OIS::JoyStickEvent& arg, int button) override;
        bool buttonReleased(const OIS::JoyStickEvent& arg, int button) override;
        bool axisMoved(const OIS::JoyStickEvent& arg, int axis) override;
        bool sliderMoved(const OIS::JoyStickEvent& arg, int index) override;
        bool povMoved(const OIS::JoyStickEvent& arg, int index) override;
        bool vector3Moved(const OIS::JoyStickEvent& arg, int index) override;

    protected:

        friend class InputDispatcher;
        Gamepad(OIS::JoyStick *joystick);
        void update();
        
    protected:
        
        struct state {
            dvec2 leftStickValue, rightStickValue;
            bool l1Button, r1Button, aButton, bButton, xButton, yButton, startButton, selectButton;
            double l2Trigger, r2Trigger;
        };
        
        OIS::JoyStick *_joystick;
        state _currentState, _previousState;
        
    };


    /**
     @class InputDispatcher
     */
    class InputDispatcher : public std::enable_shared_from_this<InputDispatcher> {
    public:

        /**
            Set the coordinate system the singleton InputDIspatcher will use.
            Defaults to ScreenCoordinateSystem::BottomLeft
         */
        static void setScreenCoordinateSystemOrigin(ScreenCoordinateSystem::origin origin) {
            _screenOrigin = origin;
        }

        ScreenCoordinateSystem::origin screenCoordinateSystemOrigin() {
            return _screenOrigin;
        }

        static void set(InputDispatcherRef instance) {
            _sInstance = instance;
        }

        static InputDispatcherRef get() {
            return _sInstance;
        }

    public:

        InputDispatcher(app::WindowRef window);

        virtual ~InputDispatcher();

        /// convenience function to check if the key is currently pressed
        bool isKeyDown(int keyCode) const;
        
        /// convenience function to check if the key was pressed this timestemp
        bool wasKeyPressed(int keyCode) const;
        
        /// convenience function to check if the key was released this timestemp
        bool wasKeyReleased(int keyCode) const;

        /// called by App before dispatching update() to scenario
        void update();
        
        /// called by App after dispatching update() to scenario
        void postUpdate();

        /// hide the user's mouse cursor
        void hideMouse();

        /// reveal the user's mouse cursor
        void unhideMouse();

        bool mouseHidden() const {
            return _mouseHidden;
        }

        bool isShiftDown() const {
            return _lastKeyEvent.isShiftDown();
        }

        bool isAltDown() const {
            return _lastKeyEvent.isAltDown();
        }

        bool isControlDown() const {
            return _lastKeyEvent.isControlDown();
        }

        bool isMetaDown() const {
            return _lastKeyEvent.isMetaDown();
        }

        bool isAccelDown() const {
            return _lastKeyEvent.isAccelDown();
        }

        bool isMouseLeftButtonDown() const {
            return _mouseLeftDown;
        }

        bool isMouseMiddleButtonDown() const {
            return _mouseMiddleDown;
        }

        bool isMouseRightButtonDown() const {
            return _mouseRightDown;
        }

        ivec2 getMousePosition() const {
            return _lastMouseEvent.getPos();
        }

        app::MouseEvent getLastMouseEvent() const {
            return _lastMouseEvent;
        }
        
        /**
         InputDispatcher will create a Gamepad for each attached/detected/supported gamepad.
         Right now this is only known to work with PS4 controllers attached over USB.
         If no controllers are attached this vector will be empty.
         **/
        const vector<GamepadRef> &getGamepads() const {
            return _gamepads;
        }
        
    private:

        friend class InputListener;

        /**
            Push a listener to the top of the input receiver stack.
            This listener will be the first to receive input events.
         */
        void _addListener(InputListener *listener);

        /**
            Remove a listener from the input receiver stack.
            It will no longer receive input events.
         */
        void _removeListener(InputListener *listener);

        /**
            Return the listener (if any) which has the topmost position in the listener stack
         */
        InputListener *_topListener() const;

        void _sortListeners();

        ivec2 _mouseDelta(const app::MouseEvent &event);

        bool _mouseDown(app::MouseEvent event);

        bool _mouseUp(app::MouseEvent event);

        bool _mouseWheel(app::MouseEvent event);

        bool _mouseMove(app::MouseEvent event);

        bool _mouseDrag(app::MouseEvent event);

        bool _keyDown(app::KeyEvent event);

        bool _keyUp(app::KeyEvent event);

    private:

        static InputDispatcherRef _sInstance;
        static ScreenCoordinateSystem::origin _screenOrigin;

        std::vector<InputListener *> _listeners;
        std::set<int> _keyPressState, _keysPressed, _keysReleased;
        app::MouseEvent _lastMouseEvent;
        app::KeyEvent _lastKeyEvent;
        ivec2 *_lastMousePosition;
        cinder::signals::Connection _mouseDownId, _mouseUpId, _mouseWheelId, _mouseMoveId, _mouseDragId, _keyDownId, _keyUpId;
        bool _mouseHidden, _mouseLeftDown, _mouseMiddleDown, _mouseRightDown;
        
        OIS::InputManager *_oisInputManager;
        vector<GamepadRef> _gamepads;
        
    };

    /**
     @class InputListener
     Interface for classes which want to be notified of user input. You will generally do so via InputComponent subclasses.

     Each of the input handling functions (onMouseDown, onMouseUp, onKeyDown, etc ) return a boolean. Return true if your listener
     consumed the event. If so, the event will NOT be passed on to the next listener in the stack. If you ignore
     the event return false to let it pass on to the next listener.
     
     To "poll" events without consuming them, e.g., to monitor a key value during a call to update() in a Component,
     call isKeyDown, getMousePosition, etc. See InputComponent.
     
     Listeners will be notified in order of getDispatchReceiptIndex(), where listeners with a lower index will have the
     opportunity to consume the input before those with a higher value.
     
     Listeners are not listening by default, you must call setListening(true) to start listening to input. This is
     handled automatically by the InputComponent lifecycle in InputComponent::onReady()
     */
    class InputListener {
    public:

        /**
         Create an InputListener with a given dispatchReceiptIndex, where a lower dispatchReceiptIndex will have the
         oppotunity to consume input events earlier than those with a higher dispatchReceiptIndex.
         */
        InputListener(int dispatchReceiptIndex);

        virtual ~InputListener();

        void setDispatchReceiptIndex(int newIndex);

        int getDispatchReceiptIndex() const {
            return _dispatchReceiptIndex;
        }

        /// pass true to "turn on" this InputListener, false to ignore input.
        void setListening(bool listening) {
            _listening = listening;
        }

        virtual bool isListening() const {
            return _listening;
        }

        virtual bool onMouseDown(const app::MouseEvent &event) {
            return false;
        }

        virtual bool onMouseUp(const app::MouseEvent &event) {
            return false;
        }

        virtual bool onMouseWheel(const app::MouseEvent &event) {
            return false;
        }

        virtual bool onMouseMove(const app::MouseEvent &event, const ivec2 &delta) {
            return false;
        }

        virtual bool onMouseDrag(const app::MouseEvent &event, const ivec2 &delta) {
            return false;
        }

        virtual bool onKeyDown(const app::KeyEvent &event) {
            return false;
        }

        virtual bool onKeyUp(const app::KeyEvent &event) {
            return false;
        }

        //
        // The following methods are for polling behavior and aren't affected by isListening()
        //

        bool isKeyDown(int keyCode) const {
            return InputDispatcher::get()->isKeyDown(keyCode);
        }

        bool wasKeyPressed(int keyCode) const {
            return InputDispatcher::get()->wasKeyPressed(keyCode);
        }

        bool wasKeyReleased(int keyCode) const {
            return InputDispatcher::get()->wasKeyReleased(keyCode);
        }

        ivec2 getMousePosition() const {
            return InputDispatcher::get()->getMousePosition();
        }

        bool isMouseLeftButtonDown() const {
            return InputDispatcher::get()->isMouseLeftButtonDown();
        }

        bool isMouseRightButtonDown() const {
            return InputDispatcher::get()->isMouseRightButtonDown();
        }

        bool isMouseMiddleButtonDown() const {
            return InputDispatcher::get()->isMouseMiddleButtonDown();
        }

        bool isShiftDown() const {
            return InputDispatcher::get()->isShiftDown();
        }

        bool isAltDown() const {
            return InputDispatcher::get()->isAltDown();
        }

        bool isControlDown() const {
            return InputDispatcher::get()->isControlDown();
        }

        bool isMetaDown() const {
            return InputDispatcher::get()->isMetaDown();
        }

        bool isAccelDown() const {
            return InputDispatcher::get()->isAccelDown();
        }

    private:

        bool _listening;
        int _dispatchReceiptIndex;

    };


}


#endif /* InputDispatcher_hpp */
