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

#include "core/Common.hpp"
#include "core/MathHelpers.hpp"

namespace core {

    class InputListener;

    SMART_PTR(InputDispatcher);

    namespace ScreenCoordinateSystem {
        enum origin {

            TopLeft,
            BottomLeft

        };
    }


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

        ci::app::MouseEvent getLastMouseEvent() const {
            return _lastMouseEvent;
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

        ivec2 _mouseDelta(const ci::app::MouseEvent &event);

        bool _mouseDown(ci::app::MouseEvent event);

        bool _mouseUp(ci::app::MouseEvent event);

        bool _mouseWheel(ci::app::MouseEvent event);

        bool _mouseMove(ci::app::MouseEvent event);

        bool _mouseDrag(ci::app::MouseEvent event);

        bool _keyDown(ci::app::KeyEvent event);

        bool _keyUp(ci::app::KeyEvent event);

    private:

        static InputDispatcherRef _sInstance;
        static ScreenCoordinateSystem::origin _screenOrigin;

        std::vector<InputListener *> _listeners;
        std::set<int> _keyPressState, _keysPressed, _keysReleased;
        ci::app::MouseEvent _lastMouseEvent;
        ci::app::KeyEvent _lastKeyEvent;
        ivec2 *_lastMousePosition;
        cinder::signals::Connection _mouseDownId, _mouseUpId, _mouseWheelId, _mouseMoveId, _mouseDragId, _keyDownId, _keyUpId;
        bool _mouseHidden, _mouseLeftDown, _mouseMiddleDown, _mouseRightDown;

    };

    /**
     @class InputListener
     Interface for classes which want to be notified of user input

     Derive your class from InputListener and then when you want
     to start listening to input, call InputListener::takeFocus() to push
     your listener to the top of the input listener stack.

     Your InputListener will automatically detach during destruction.

     Each of the input handling functions (mouseDown, mouseUp, etc ) return a boolean. Return true if your listener
     consumed the event. If so, the event will NOT be passed on to the next listener in the stack. If you ignore
     the event return false to let it pass on to the next listener.
     */
    class InputListener {
    public:

        friend class InputDispatcher;

        /**
            To use an InputListener, you mush get the singleton InputDispatcher and push your listener. The listener
            will become the first listener, and will receive input events first, before other listeners in the stack.
         */
        InputListener();

        InputListener(int dispatchReceiptIndex);

        virtual ~InputListener();

        void setDispatchReceiptIndex(int newIndex);

        int getDispatchReceiptIndex() const {
            return _dispatchReceiptIndex;
        }

        void setListening(bool listening) {
            _listening = listening;
        }

        virtual bool isListening() const {
            return _listening;
        }

        virtual bool mouseDown(const ci::app::MouseEvent &event) {
            return false;
        }

        virtual bool mouseUp(const ci::app::MouseEvent &event) {
            return false;
        }

        virtual bool mouseWheel(const ci::app::MouseEvent &event) {
            return false;
        }

        virtual bool mouseMove(const ci::app::MouseEvent &event, const ivec2 &delta) {
            return false;
        }

        virtual bool mouseDrag(const ci::app::MouseEvent &event, const ivec2 &delta) {
            return false;
        }

        virtual bool keyDown(const ci::app::KeyEvent &event) {
            return false;
        }

        virtual bool keyUp(const ci::app::KeyEvent &event) {
            return false;
        }

        /**
            Convenience function to query if a key is currently pressed
         */
        virtual bool isKeyDown(int keyCode) const {
            return InputDispatcher::get()->isKeyDown(keyCode);
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
