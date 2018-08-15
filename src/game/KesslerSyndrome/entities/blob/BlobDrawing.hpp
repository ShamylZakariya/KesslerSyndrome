//
//  BlobDrawing.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 8/15/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef BlobDrawing_hpp
#define BlobDrawing_hpp

#include "elements/ParticleSystem/ParticleSystem.hpp"
#include "game/KesslerSyndrome/entities/blob/BlobPhysics.hpp"


namespace game {
    
#pragma mark - BlobDebugDrawComponent
    
    /// Draws Blobs' physics particle, tentacle bodies and shapes etc for debugging
    class BlobDebugDrawComponent : public core::EntityDrawComponent {
    public:
        
        struct config {
            int drawLayer;
            ColorA particleColor;
            ColorA tentacleColor;
            
            config():
            drawLayer(0),
            particleColor(0,0,0,1),
            tentacleColor(0,1,1,1)
            {}
        };
        
    public:
        
        BlobDebugDrawComponent(const config &c);
        
        // DrawComponent
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        cpBB getBB() const override { return _bb; }
        void update(const core::time_state &timeState) override;
        void draw(const core::render_state &renderState) override;
                
    protected:
        
        config _config;
        BlobPhysicsComponentWeakRef _physics;
        cpBB _bb;
        
    };
    
#pragma mark - BlobParticleSimulation
    
    
    /// BlobParticleSimulation is an adapter to allow us to use a particle system to render the blob body
    class BlobParticleSimulation : public elements::BaseParticleSimulation {
    public:
        
        BlobParticleSimulation(BlobPhysicsComponentRef physics);
        
        void setParticleCount(size_t particleCount) override;
        
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        
        void update(const core::time_state &timeState) override;

        size_t getFirstActive() const override;
        
        size_t getActiveCount() const override;
        
        cpBB getBB() const override;
        
    protected:
        
        BlobPhysicsComponentWeakRef _physics;
        cpBB _bb;
        Perlin _perlin;
        
    };
    
#pragma mark - BlobParticleSystemDrawComponent
    
    /// BlobParticleSystemDrawComponent draws the BlobParticleSimulation with a custom compositor
    class BlobParticleSystemDrawComponent : public elements::ParticleSystemDrawComponent {
    public:
        
        struct config : public elements::ParticleSystemDrawComponent::config {
            gl::Texture2dRef tonemap;
            gl::Texture2dRef background;
            ColorA highlightColor;
            float backgroundRepeat;
        };
        
    public:
        
        BlobParticleSystemDrawComponent(config c);
        
    protected:
        
        gl::GlslProgRef createDefaultShader() const override;
        
    };
    
    
}

#endif /* BlobDrawing_hpp */
