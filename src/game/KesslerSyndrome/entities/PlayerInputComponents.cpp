//
//  PlayerInputComponents.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/8/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "PlayerInputComponents.hpp"

using namespace core;
namespace game {
    
#pragma mark - PlayerInputComponent
    
    PlayerInputComponent::PlayerInputComponent() {
        _keyRun = app::KeyEvent::KEY_LSHIFT;
        _keyLeft = app::KeyEvent::KEY_a;
        _keyRight = app::KeyEvent::KEY_d;
        _keyJump = app::KeyEvent::KEY_w;
        _keyCrouch = app::KeyEvent::KEY_s;
        
        monitorKey(_keyLeft);
        monitorKey(_keyRight);
        monitorKey(_keyJump);
        monitorKey(_keyCrouch);
        monitorKey(_keyRun);
    }
    
    PlayerInputComponent::~PlayerInputComponent() {
    }
    
    bool PlayerInputComponent::isRunning() const {
        return isMonitoredKeyDown(_keyRun);
    }
    
    bool PlayerInputComponent::isGoingRight() const {
        return isMonitoredKeyDown(_keyRight);
    }
    
    bool PlayerInputComponent::isGoingLeft() const {
        return isMonitoredKeyDown(_keyLeft);
    }
    
    bool PlayerInputComponent::isJumping() const {
        return isMonitoredKeyDown(_keyJump);
    }
    
    bool PlayerInputComponent::isCrouching() const {
        return isMonitoredKeyDown(_keyCrouch);
    }
    
}
