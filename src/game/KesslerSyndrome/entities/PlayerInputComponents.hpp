//
//  PlayerInputComponents.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/8/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef PlayerInputComponents_hpp
#define PlayerInputComponents_hpp

#include "Core.hpp"

namespace game {
    
    SMART_PTR(PlayerInputComponent);

#pragma mark - PlayerInputComponent
    
    class PlayerInputComponent : public core::InputComponent {
    public:
        
        PlayerInputComponent();
        
        virtual ~PlayerInputComponent();
        
        // actions
        bool isRunning() const { return false; }
        
        bool isGoingRight() const;
        
        bool isGoingLeft() const;
        
        bool isJumping() const;
        
        bool isCrouching() const;
        
        bool isAimingCounterClockwise() const;
        
        bool isAimingClockwise() const;
        
        // true for the instant after the fire key is depressed
        bool didFire() const;

        // true while the fire key is pressed
        bool isFiring() const;
        
    private:
        
        int _keyLeft, _keyRight, _keyJump, _keyCrouch, _keyAimCCW, _keyAimCW, _keyFire;
        
    };
    
}

#endif /* PlayerInputComponents_hpp */
