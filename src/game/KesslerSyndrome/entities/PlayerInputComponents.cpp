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
    
    /**
     int _keyLeft, _keyRight, _keyJump, _keyCrouch, _keyAimCCW, _keyAimCW, _keyFire;
     */
    
    PlayerInputComponent::PlayerInputComponent() {
        _keyLeft = app::KeyEvent::KEY_a;
        _keyRight = app::KeyEvent::KEY_d;
        _keyJump = app::KeyEvent::KEY_w;
        _keyCrouch = app::KeyEvent::KEY_s;
        _keyAimCCW = app::KeyEvent::KEY_q;
        _keyAimCW = app::KeyEvent::KEY_e;
        _keyFire = app::KeyEvent::KEY_SPACE;
        
        monitorKeys({
            _keyLeft, _keyRight, _keyJump, _keyCrouch, _keyAimCCW, _keyAimCW, _keyFire
        });
        
    }
    
    PlayerInputComponent::~PlayerInputComponent() {
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
    
    bool PlayerInputComponent::isAimingCounterClockwise() const {
        return isMonitoredKeyDown(_keyAimCCW);
    }
    
    bool PlayerInputComponent::isAimingClockwise() const {
        return isMonitoredKeyDown(_keyAimCW);
    }
    
    bool PlayerInputComponent::didFire() const {
        return wasMonitoredKeyPressed(_keyFire);
    }
    
    bool PlayerInputComponent::isFiring() const {
        return isMonitoredKeyDown(_keyFire);
    }
    
}
