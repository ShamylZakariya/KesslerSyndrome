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
    
#pragma mark - BlobTentacleDrawer
    
    class BlobTentacleDrawer {
    public:
        
        struct config {
            gl::Texture2dRef tex;
            ColorA color;
        };
        
    public:
        
        BlobTentacleDrawer(config c, const shared_ptr<BlobPhysicsComponent::tentacle> &tentacle, size_t idx);
        
        void update(const core::time_state &time);
        void draw(const core::render_state &state);
        
    protected:
        
        void updateSplineReps();
        void triangulate();
        
        struct vertex {
            vec2 position;
            vec2 texCoord;
        };
        
        struct spline_rep {
            vector<vec2> segmentVertices, splineVertices;
            float startWidth, endWidth;
        };
        
    protected:
        
        size_t _idx;
        shared_ptr<BlobPhysicsComponent::tentacle> _tentacle;
        vector<vertex> _vertices;
        spline_rep _spline;
        
        gl::Texture2dRef _texture;
        ColorA _color;
        
        gl::GlslProgRef _shader;
        gl::VboRef _vbo;
        gl::BatchRef _batch;
        
    };
    
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
        
        void drawParticles(const core::render_state &renderState, const BlobPhysicsComponentRef &physics);
        void drawTentacles(const core::render_state &renderState, const BlobPhysicsComponentRef &physics);

    protected:
        
        config _config;
        BlobPhysicsComponentWeakRef _physics;
        cpBB _bb;
        
    };

#pragma mark - BlobTentacleDrawComponent

    /// Draws Blobs' tentacles
    class BlobTentacleDrawComponent : public core::EntityDrawComponent {
    public:
        
        struct config {
            int drawLayer;
            ColorA tentacleColor;
            gl::Texture2dRef tentacleTexture;
            
            config():
            drawLayer(0),
            tentacleColor(0,1,1,1)
            {}
        };
        
    public:
        
        BlobTentacleDrawComponent(const config &c);
        
        // DrawComponent
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        cpBB getBB() const override { return _bb; }
        void update(const core::time_state &timeState) override;
        void draw(const core::render_state &renderState) override;
        
    protected:
        
        config _config;
        BlobPhysicsComponentWeakRef _physics;
        cpBB _bb;
        vector<BlobTentacleDrawer> _tentacleDrawers;
        
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
