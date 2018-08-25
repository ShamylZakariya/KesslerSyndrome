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

#include "game/KesslerSyndrome/elements/Planet.hpp"

namespace game {
    
    SMART_PTR(BlobControllerComponent);
    SMART_PTR(Blob);
    
    
    class BlobControllerComponent : public core::InputComponent {
    public:
        
        BlobControllerComponent(core::GamepadRef gamepad);
        
        // component
        void update(const core::time_state &time) override;
        
        // BlobControllerComponent
        dvec2 getMotionDirection() const { return _motionDir; }
        dvec2 getAimDirection() const { return _aimDir; }
                
    protected:

        core::GamepadRef _gamepad;        
        dvec2 _motionDir, _aimDir;
        

    };

    class Blob : public core::Entity, public core::Trackable {
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
        virtual ~Blob();
        
        // Blob
        const BlobPhysicsComponentRef &getBlobPhysicsComponent() const { return _physics; }
        const BlobControllerComponentRef &getBlobControllerComponent() const { return _input; }
        
        // Entity
        void update(const core::time_state &time) override;
        void onHealthChanged(double oldHealth, double newHealth) override;
        
        // Tracking
        dvec2 getTrackingPosition() const override;
        dvec2 getTrackingUp() const override;
        
    protected:

        
        config _config;
        BlobPhysicsComponentRef _physics;
        BlobControllerComponentRef _input;
        
    };

#pragma mark - BlobViewportController
    
    class BlobViewportController : public elements::ViewportController {
    public:
        
        BlobViewportController(core::ViewportRef viewport, BlobRef blob, PlanetRef planet);
        
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        void firstUpdate(const core::time_state &time) override;
        void update(const core::time_state &time) override;
        
    private:
        
        BlobRef _blob;
        PlanetRef _planet;
        
    };
    
}

#endif /* Blob_hpp */
