//
//  Blob.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 8/5/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef Blob_hpp
#define Blob_hpp

#include "core/Core.hpp"
#include "game/KesslerSyndrome/entities/blob/BlobPhysics.hpp"
#include "game/KesslerSyndrome/entities/blob/BlobDrawing.hpp"


namespace game {
    
    SMART_PTR(BlobControllerComponent);
    SMART_PTR(Blob);
    
    
    class BlobControllerComponent : public core::InputComponent {
    public:
        
        BlobControllerComponent(core::GamepadRef gamepad);
        
        // component
        void update(const core::time_state &time) override;
        
        // BlobControllerComponent
        double getHorizontalSpeed() const { return _horizontalSpeed; }
        double getJetpackPower() const { return _jetpackPower; }
        dvec2 getAimDirection() const { return _aimDir; }
                
    protected:

        core::GamepadRef _gamepad;
        
        double _horizontalSpeed, _jetpackPower;
        dvec2 _aimDir;
        

    };

    class Blob : public core::Entity {
    public:
        
        struct config {
            core::HealthComponent::config health;
            BlobPhysicsComponent::config physics;
            
            // particle texture for pre-composite particle system rendering. should be square texture with roughly circular, blurred shape
            gl::Texture2dRef particle;
            
            // tonemap texture which maps particle alpha to output color - expects texture shaped something like 128 wide by 1 tall.
            // output color is the color in the tonemap where x is particle alpha scaled [0,1]
            gl::Texture2dRef tonemap;
            
            // background fill image applied to blob - position is static against the viewport
            gl::Texture2dRef background;

            // tentacle fill color
            ColorA tentacleColor;

            // texture to draw on tentacles
            gl::Texture2dRef tentacleTexture;
            
            // highlight color applied to blob
            ColorA highlightColor;
            
            // number of times the background fill texture repeats across the x-axis
            double backgroundRepeat;
            
            bool drawDebugOverlay;
            
            config():
                    backgroundRepeat(16),
                    drawDebugOverlay(false),
                    highlightColor(0.5, 0.7, 0.9, 0.25),
                    tentacleColor(1,0,1,1)
            {}
        };
        
        static BlobRef create(string name, config c, core::GamepadRef gamepad);
        
    public:
        
        Blob(string name);
        const BlobPhysicsComponentRef &getBlobPhysicsComponent() const { return _physics; }
        const BlobControllerComponentRef &getBlobControllerComponent() const { return _input; }
        
        // Entity
        void update(const core::time_state &time) override;
        void onHealthChanged(double oldHealth, double newHealth) override;
        
    protected:

        
        config _config;
        BlobPhysicsComponentRef _physics;
        BlobControllerComponentRef _input;
        
    };
    
    
}

#endif /* Blob_hpp */
