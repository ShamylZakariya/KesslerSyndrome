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
        bool isRunning() const;
        
        bool isGoingRight() const;
        
        bool isGoingLeft() const;
        
        bool isJumping() const;
        
        bool isCrouching() const;
        
    private:
        
        int _keyRun, _keyLeft, _keyRight, _keyJump, _keyCrouch;
        
    };
    
}

#endif /* PlayerInputComponents_hpp */
