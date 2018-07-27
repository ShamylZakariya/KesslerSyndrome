//
//  PlayerInputComponents.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/8/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "game/KesslerSyndrome/entities/player/PlayerInputComponents.hpp"

#include "game/KesslerSyndrome/GameConstants.hpp"

using namespace core;
namespace game {
    
#pragma mark - PlayerInputComponent
    
    /**
     int _keyLeft, _keyRight, _keyJump, _keyCrouch, _keyAimCCW, _keyAimCW, _keyFire;
     */
    
    PlayerInputComponent::PlayerInputComponent():
    InputComponent(InputDispatchReceiptOrder::PLAYER)
    {
        _keyLeft = app::KeyEvent::KEY_a;
        _keyRight = app::KeyEvent::KEY_d;
        _keyJump = app::KeyEvent::KEY_w;
        _keyCrouch = app::KeyEvent::KEY_s;
        _keyAimCCW = app::KeyEvent::KEY_q;
        _keyAimCW = app::KeyEvent::KEY_e;
        _keyFire = app::KeyEvent::KEY_SPACE;
    }
    
    PlayerInputComponent::~PlayerInputComponent() {
    }
        
    bool PlayerInputComponent::isGoingRight() const {
        return isKeyDown(_keyRight);
    }
    
    bool PlayerInputComponent::isGoingLeft() const {
        return isKeyDown(_keyLeft);
    }
    
    bool PlayerInputComponent::isJumping() const {
        return isKeyDown(_keyJump);
    }
    
    bool PlayerInputComponent::isCrouching() const {
        return isKeyDown(_keyCrouch);
    }
    
    bool PlayerInputComponent::isAimingCounterClockwise() const {
        return isKeyDown(_keyAimCCW);
    }
    
    bool PlayerInputComponent::isAimingClockwise() const {
        return isKeyDown(_keyAimCW);
    }
    
    bool PlayerInputComponent::didFire() const {
        return wasKeyPressed(_keyFire);
    }
    
    bool PlayerInputComponent::isFiring() const {
        return isKeyDown(_keyFire);
    }
    
}
