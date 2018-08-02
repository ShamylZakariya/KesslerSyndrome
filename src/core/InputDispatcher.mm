//
//  InputDispatcher.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#include "core/InputDispatcher.hpp"

#import <AppKit/AppKit.h>

#import "core/util/Xml.hpp"

namespace core {
    
    namespace {
        
        app::MouseEvent TranslateMouseEvent(const app::MouseEvent &e, ScreenCoordinateSystem::origin origin) {
            // cinder dispatches mouse events for a top-left system
            if (origin == ScreenCoordinateSystem::TopLeft) return e;
            
            using namespace app;
            
            //
            //	MouseEvent is read only, so we have to reconstruct :(
            //
            
            int initiator = 0;
            unsigned int modifiers = 0;
            
            if (e.isLeft()) initiator |= MouseEvent::LEFT_DOWN;
            if (e.isRight()) initiator |= MouseEvent::RIGHT_DOWN;
            if (e.isMiddle()) initiator |= MouseEvent::MIDDLE_DOWN;
            
            if (e.isLeftDown()) modifiers |= MouseEvent::LEFT_DOWN;
            if (e.isRightDown()) modifiers |= MouseEvent::RIGHT_DOWN;
            if (e.isMiddleDown()) modifiers |= MouseEvent::MIDDLE_DOWN;
            
            if (e.isShiftDown()) modifiers |= MouseEvent::SHIFT_DOWN;
            if (e.isAltDown()) modifiers |= MouseEvent::ALT_DOWN;
            if (e.isControlDown()) modifiers |= MouseEvent::CTRL_DOWN;
            if (e.isMetaDown()) modifiers |= MouseEvent::META_DOWN;
            if (e.isAccelDown()) modifiers |= MouseEvent::ACCEL_DOWN;
            
            return app::MouseEvent(e.getWindow(),
                                       initiator,
                                       e.getX(),
                                       app::getWindowHeight() - e.getY(),
                                       modifiers,
                                       e.getWheelIncrement(),
                                       e.getNativeModifiers()
                                       );
        }
        
        bool inputListenerDispatchIndexSorter(InputListener *a, InputListener *b) {
            return a->getDispatchReceiptIndex() < b->getDispatchReceiptIndex();
        }
        
        NSWindow* get_native_window(app::WindowRef window) {
            NSView *view = (NSView*)window->getNative();
            return [view window];
        }
        
        OIS::ParamList get_native_window_paramlist(app::WindowRef window) {
            NSWindow *nativeWindow = get_native_window(window);
            OIS::ParamList pl;
            pl.insert(std::make_pair(std::string("WINDOW"), std::to_string((size_t)nativeWindow)));
            return pl;
        }
        
    }
    
#pragma mark - Gamepad
   
    Gamepad::~Gamepad()
    {
        _joystick->setEventCallback(nullptr);
    }
    
    int Gamepad::getId() const {
        return _joystick->getID();
    }
    
    string Gamepad::getVendor() const {
        return _joystick->vendor();
    }
    
    bool Gamepad::buttonPressed(const OIS::JoyStickEvent& arg, int button) {
        const auto component = _deviceButtonToComponentsMap[button];
        
        if (_logEvents) {
            CI_LOG_D("buttonPressed device: " << getId() << " device button: " << button << " component: " << component);
        }

        // bail if we have a recognized input
        if (!componentIsButton(component)) { return false; }
        
        _currentState[component] = true;
        
        return true;
    }

    bool Gamepad::buttonReleased(const OIS::JoyStickEvent& arg, int button) {
        const auto component = _deviceButtonToComponentsMap[button];

        if (_logEvents) {
            CI_LOG_D("buttonRelease device: " << getId() << " device button: " << button << " component: " << component);
        }

        // bail if we have a recognized input
        if (!componentIsButton(component)) { return false; }

        _currentState[component] = false;
        
        return true;
    }

    bool Gamepad::axisMoved(const OIS::JoyStickEvent& arg, int axis) {
        const auto map = _deviceAxisToComponentsMap[axis];
        double relativeValue = 0;
        if (componentIsAxis(map.component)) {

            double value = arg.state.mAxes[axis].abs;

            //
            //  Handle the Dead Zone
            //

            const double Range = static_cast<double>(OIS::JoyStick::MAX_AXIS);
            if (map.positiveUnitRange) {
                if (value < -Range + _deviceAxisDeadZone ) {
                    value = -Range;
                }
            } else {
                if (value > -_deviceAxisDeadZone && value < _deviceAxisDeadZone) {
                    value = 0;
                }
            }

            //
            //  Compute normalize value
            //

            relativeValue = value / Range;
            
            if (map.positiveUnitRange) {
                relativeValue = (relativeValue * 0.5) + 0.5;
            }
            
            if (map.invert) {
                relativeValue *= -1;
            }
        
            _currentState[map.component] = relativeValue;
        }

        if (_logEvents) {
            CI_LOG_D("axisMoved device: " << getId() << " device axis: " << axis << " component: " << map.component << " relativeValue: " << relativeValue);
        }
        
        return true;
    }
    
    bool Gamepad::sliderMoved(const OIS::JoyStickEvent& arg, int index) {
        if (_logEvents) {
            CI_LOG_D("sliderMoved device: " << getId() << " device index:" << index);
        }
        return true;
    }
    bool Gamepad::povMoved(const OIS::JoyStickEvent& arg, int index) {
        if (_logEvents) {
            CI_LOG_D("povMoved device: " << getId() << " device index:" << index);
        }
        return true;
    }
    bool Gamepad::vector3Moved(const OIS::JoyStickEvent& arg, int index) {
        if (_logEvents) {
            CI_LOG_D("vector3Moved device: " << getId() << " device index:" << index);
        }
        return true;
    }

    Gamepad::Components Gamepad::stringToComponents(const string &componentString) {
        if (componentString == "LeftStickX") return LeftStickX;
        if (componentString == "LeftStickY") return LeftStickY;
        if (componentString == "RightStickX") return RightStickX;
        if (componentString == "RightStickY") return RightStickY;

        if (componentString == "DPadX") return DPadX;
        if (componentString == "DPadY") return DPadY;

        if (componentString == "LeftTrigger") return LeftTrigger;
        if (componentString == "RightTrigger") return RightTrigger;
        
        if (componentString == "LeftShoulderButton") return LeftShoulderButton;
        if (componentString == "LeftShoulderButton2") return LeftShoulderButton2;
        if (componentString == "RightShoulderButton") return RightShoulderButton;
        if (componentString == "RightShoulderButton2") return RightShoulderButton2;
        if (componentString == "LeftStickButton") return LeftStickButton;
        if (componentString == "RightStickButton") return RightStickButton;

        if (componentString == "AButton") return AButton;
        if (componentString == "BButton") return BButton;
        if (componentString == "XButton") return XButton;
        if (componentString == "YButton") return YButton;
        if (componentString == "StartButton") return StartButton;
        if (componentString == "SelectButton") return SelectButton;

        return Unknown;
    }
    
    bool Gamepad::componentIsButton(Components component) {
        return component >= LeftShoulderButton && component < Unknown;
    }

    bool Gamepad::componentIsAxis(Components component) {
        return component >= LeftStickX && component <= RightTrigger;
    }

    
    /*
     OIS::JoyStick *_joystick;
     bool _logEvents;
     vector<double> _currentState, _previousState;
     vector<Components> _deviceButtonToComponentsMap;
     vector<pair<Components,axis_mapping>> _deviceAxisToComponentsMap;
     double _deviceAxisDeadZone;
     */
    
    Gamepad::Gamepad(OIS::JoyStick *joystick):
            _joystick(joystick),
            _logEvents(false),
            _deviceAxisDeadZone(2000)
    {
        _joystick->setEventCallback(this);
        _currentState.resize(ComponentCount, 0);
        _previousState.resize(ComponentCount, 0);
        _deviceButtonToComponentsMap.resize(ComponentCount, Unknown);
        _deviceAxisToComponentsMap.resize(ComponentCount, { Unknown, false, false });
        loadDeviceSemanticMapping();
    }
    
    void Gamepad::update() {
        _previousState = _currentState;
        _joystick->capture();
    }
    
    void Gamepad::loadDeviceSemanticMapping() {
        const string vendor = getVendor();
        XmlTree mappingDoc = XmlTree(app::loadAsset("core/input/gamepad_mappings.xml"));

        auto maybeDeviceMapping = util::xml::findElement(mappingDoc, "mapping", "vendor", vendor);
        if (maybeDeviceMapping) {
            auto deviceMapping = *maybeDeviceMapping;

            CI_LOG_D("Loading mappings for device: " << getId() << " vendor: " << vendor);
            
            if (deviceMapping.hasAttribute("deadzone")) {
                _deviceAxisDeadZone = deviceMapping.getAttributeValue<double>("deadzone");
            }
            
            for (const auto &node : deviceMapping.getChildren()) {
                
                // load the component type
                const Components component = stringToComponents(node->getValue());
                CI_ASSERT_MSG(component != Unknown, (string("Unrecognized component type \"") + node->getValue() + string("\"")).c_str());
                
                // load the index to map from
                size_t idx = node->getAttributeValue<size_t>("index");
                
                if (node->getTag() == "button") {
                    // this is a button
                    CI_LOG_D("mapping button name: " << node->getValue() << " to: " << idx);
                    _deviceButtonToComponentsMap[idx] = component;
                } else if (node->getTag() == "axis") {
                    // this is an axis
                    const bool remap = node->hasAttribute("remapToPositiveUnitRange") && (node->getAttribute("remapToPositiveUnitRange") == "true");
                    const bool invert = node->hasAttribute("invert") && (node->getAttribute("invert") == "true");
                    CI_LOG_D("mapping axis name: " << node->getValue() << "(" << component << ") to: " << idx << " remapToPositiveUnitRange: " << remap << " invert: " << invert);
                    _deviceAxisToComponentsMap[idx] = axis_mapping { component, invert, remap };
                }
            }
        } else {
            CI_LOG_E("Unrecognized device with vendor: \"" << vendor << "\"");
        }
    }
    
#pragma mark - InputDispatcher
    
    /*
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
     */
    
    InputDispatcherRef InputDispatcher::_sInstance;
    ScreenCoordinateSystem::origin InputDispatcher::_screenOrigin = ScreenCoordinateSystem::BottomLeft;
    
    InputDispatcher::InputDispatcher(app::WindowRef window) :
    _lastMousePosition(NULL),
    _lastKeyEvent(),
    _mouseHidden(false),
    _mouseLeftDown(false),
    _mouseMiddleDown(false),
    _mouseRightDown(false)
    {
        _mouseDownId = window->getSignalMouseDown().connect([this](app::MouseEvent &e) {
            _mouseDown(e);
        });
        
        _mouseUpId = window->getSignalMouseUp().connect([this](app::MouseEvent &e) {
            _mouseUp(e);
        });
        
        _mouseWheelId = window->getSignalMouseWheel().connect([this](app::MouseEvent &e) {
            _mouseWheel(e);
        });
        
        _mouseMoveId = window->getSignalMouseMove().connect([this](app::MouseEvent &e) {
            _mouseMove(e);
        });
        
        _mouseDragId = window->getSignalMouseDrag().connect([this](app::MouseEvent &e) {
            _mouseDrag(e);
        });
        
        _keyDownId = window->getSignalKeyDown().connect([this](app::KeyEvent &e) {
            _keyDown(e);
        });
        
        _keyUpId = window->getSignalKeyUp().connect([this](app::KeyEvent &e) {
            _keyUp(e);
        });
        
        //
        //  Build OIS InputManager to corral our joysticks - and print info to console
        //
        
        OIS::ParamList params = get_native_window_paramlist(window);
        _oisInputManager = OIS::InputManager::createInputSystem(params);
        _oisInputManager->enableAddOnFactory(OIS::InputManager::AddOn_All);
        
        unsigned int v = _oisInputManager->getVersionNumber();
        cout << "OIS Version: " << (v >> 16) << "." << ((v >> 8) & 0x000000FF) << "." << (v & 0x000000FF) << ", "
                 << "Release Name: " << _oisInputManager->getVersionName() << ", "
                 << "Manager: " << _oisInputManager->inputSystemName() << ", "
                 << "Total Keyboards: " << _oisInputManager->getNumberOfDevices(OIS::OISKeyboard) << ", "
                 << "Total Mice: " << _oisInputManager->getNumberOfDevices(OIS::OISMouse) << ", "
                 << "Total JoySticks: " << _oisInputManager->getNumberOfDevices(OIS::OISJoyStick) << endl;
        
        const size_t numJoysticks = _oisInputManager->getNumberOfDevices(OIS::OISJoyStick);
        for (int i = 0; i < numJoysticks; i++) {
            OIS::JoyStick *joystick = (OIS::JoyStick*)_oisInputManager->createInputObject(OIS::OISJoyStick, true);
            _gamepads.push_back(shared_ptr<Gamepad>(new Gamepad(joystick)));
        }
    }
    
    InputDispatcher::~InputDispatcher() {
        _mouseDownId.disconnect();
        _mouseUpId.disconnect();
        _mouseWheelId.disconnect();
        _mouseMoveId.disconnect();
        _mouseDragId.disconnect();
        _keyDownId.disconnect();
        _keyUpId.disconnect();

        //
        // destroying _oisInputManager frees our joysticks
        //

        _gamepads.clear();
        OIS::InputManager::destroyInputSystem(_oisInputManager);
        _oisInputManager = nullptr;
    }
    
    bool InputDispatcher::isKeyDown(int keyCode) const {
        return _keyPressState.count(keyCode) > 0;
    }
    
    bool InputDispatcher::wasKeyPressed(int keyCode) const {
        return _keysPressed.count(keyCode) > 0;
    }
    
    bool InputDispatcher::wasKeyReleased(int keyCode) const {
        return _keysReleased.count(keyCode) > 0;
    }
    
    void InputDispatcher::update() {
        for (auto gamepad : _gamepads) {
            gamepad->update();
        }
    }
    
    void InputDispatcher::postUpdate() {
        _keysPressed.clear();
        _keysReleased.clear();
    }
    
    void InputDispatcher::hideMouse() {
        app::AppBase::get()->hideCursor();
        _mouseHidden = true;
    }
    
    void InputDispatcher::unhideMouse() {
        app::AppBase::get()->showCursor();
        _mouseHidden = false;
    }
    
    void InputDispatcher::_addListener(InputListener *listener) {
        _removeListener(listener);
        _listeners.push_back(listener);
        _sortListeners();
    }
    
    void InputDispatcher::_removeListener(InputListener *listener) {
        _listeners.erase(remove(begin(_listeners), end(_listeners), listener), end(_listeners));
    }
    
    InputListener *InputDispatcher::_topListener() const {
        return _listeners.empty() ? NULL : _listeners.back();
    }
    
    void InputDispatcher::_sortListeners() {
        sort(_listeners.begin(), _listeners.end(), inputListenerDispatchIndexSorter);
    }
    
    ivec2 InputDispatcher::_mouseDelta(const app::MouseEvent &event) {
        ivec2 delta(0, 0);
        if (_lastMousePosition) {
            delta = event.getPos() - *_lastMousePosition;
        } else {
            _lastMousePosition = new ivec2;
        }
        
        *_lastMousePosition = event.getPos();
        return delta;
    }
    
    bool InputDispatcher::_mouseDown(app::MouseEvent event) {
        _lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
        _mouseLeftDown = _lastMouseEvent.isLeftDown();
        _mouseMiddleDown = _lastMouseEvent.isMiddleDown();
        _mouseRightDown = _lastMouseEvent.isRightDown();
        
        for (auto listener : _listeners) {
            if (listener->isListening() && listener->onMouseDown(_lastMouseEvent)) return true;
        }
        
        return false;
    }
    
    bool InputDispatcher::_mouseUp(app::MouseEvent event) {
        _lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
        
        // this looks weird but the event isLeft, isRight etc mean is the up event on the left mouse, etc
        _mouseLeftDown = _mouseLeftDown && !_lastMouseEvent.isLeft();
        _mouseMiddleDown = _mouseMiddleDown && !_lastMouseEvent.isMiddle();
        _mouseRightDown = _mouseRightDown && !_lastMouseEvent.isRight();
        
        for (auto listener : _listeners) {
            if (listener->isListening() && listener->onMouseUp(_lastMouseEvent)) return true;
        }
        
        return false;
    }
    
    bool InputDispatcher::_mouseWheel(app::MouseEvent event) {
        _lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
        
        for (auto listener : _listeners) {
            if (listener->isListening() && listener->onMouseWheel(_lastMouseEvent)) return true;
        }
        
        return false;
    }
    
    bool InputDispatcher::_mouseMove(app::MouseEvent event) {
        _lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
        ivec2 delta = _mouseDelta(_lastMouseEvent);
        
        for (auto listener : _listeners) {
            if (listener->isListening() && listener->onMouseMove(_lastMouseEvent, delta)) return true;
        }
        
        return false;
    }
    
    bool InputDispatcher::_mouseDrag(app::MouseEvent event) {
        _lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
        ivec2 delta = _mouseDelta(_lastMouseEvent);
        
        for (auto listener : _listeners) {
            if (listener->isListening() && listener->onMouseDrag(_lastMouseEvent, delta)) return true;
        }
        
        return false;
    }
    
    bool InputDispatcher::_keyDown(app::KeyEvent event) {
        const auto code = event.getCode();
        _keysPressed.insert(code);
        _keyPressState.insert(code);
        _lastKeyEvent = event;
        
        for (auto listener : _listeners) {
            if (listener->isListening() && listener->onKeyDown(event)) return true;
        }
        
        return false;
    }
    
    bool InputDispatcher::_keyUp(app::KeyEvent event) {
        const auto code = event.getCode();
        _keysReleased.insert(code);
        _keyPressState.erase(code);
        _lastKeyEvent = event;
        
        for (auto listener : _listeners) {
            if (listener->isListening() && listener->onKeyUp(event)) return true;
        }
        
        return false;
    }
    
#pragma mark -
#pragma mark InputListener
    
    /*
     bool _listening;
     int _dispatchReceiptIndex;
     */
    
    InputListener::InputListener(int dispatchReceiptIndex) :
    _listening(false),
    _dispatchReceiptIndex(dispatchReceiptIndex) {
        InputDispatcher::get()->_addListener(this);
    }
    
    InputListener::~InputListener() {
        InputDispatcher::get()->_removeListener(this);
    }
    
    void InputListener::setDispatchReceiptIndex(int newIndex) {
        _dispatchReceiptIndex = newIndex;
        InputDispatcher::get()->_sortListeners();
    }
    
}
