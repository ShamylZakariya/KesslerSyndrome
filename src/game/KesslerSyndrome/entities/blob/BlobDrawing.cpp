//
//  BlobDrawing.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 8/15/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "BlobDrawing.hpp"

#include "core/util/GlslProgLoader.hpp"

#include <cinder/Perlin.h>

using namespace core;

namespace game {
    
    namespace {
        const ColorA ParticleColor(0,0,0,1);
        
        void drawAxes(cpBody *body, double length, ColorA xAxis = ColorA(1,0,0,1), ColorA yAxis = ColorA(0,1,0,1)) {
            dvec2 origin = v2(cpBodyGetPosition(body));
            dvec2 rotation = v2(cpBodyGetRotation(body));
            
            ci::gl::color(xAxis);
            ci::gl::drawLine(origin, origin + rotation * length);
            
            ci::gl::color(yAxis);
            ci::gl::drawLine(origin, origin + rotateCCW(rotation) * length);
        }
    }

    
#pragma mark - BlobDebugDrawComponent
    
    /*
     config _config;
     BlobPhysicsComponentWeakRef _physics;
     cpBB _bb;
     */
    
    BlobDebugDrawComponent::BlobDebugDrawComponent(const config &c):
            EntityDrawComponent(c.drawLayer,VisibilityDetermination::FRUSTUM_CULLING),
            _config(c)
    {}
    
    void BlobDebugDrawComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        DrawComponent::onReady(parent,stage);
        _physics = getSibling<BlobPhysicsComponent>();
    }
    
    
    void BlobDebugDrawComponent::update(const core::time_state &timeState) {
        const auto physics = _physics.lock();
        _bb = physics->getBB();
    }
    
    void BlobDebugDrawComponent::draw(const core::render_state &renderState) {
        const auto physics = _physics.lock();
        const double axisLength = 16.0 * renderState.viewport->getReciprocalScale();
        
        for (const auto &particle : physics->getPhysicsParticles()) {
            dvec2 position = v2(cpBodyGetPosition(particle.body));
            double radius = particle.radius * particle.scale;
            ci::gl::color(_config.particleColor.lerp(particle.shepherdValue, ColorA(1,1,0,1)));
            ci::gl::drawSolidCircle(position, radius, 16);
        }
        
        drawAxes(physics->getCentralBody(), axisLength);
    }    
    
#pragma mark - BlobParticleSimulation
    
    
    BlobParticleSimulation::BlobParticleSimulation(BlobPhysicsComponentRef physics):
            _physics(physics),
            _perlin(4,12345)
    {}
    
    void BlobParticleSimulation::setParticleCount(size_t particleCount) {
        BaseParticleSimulation::setParticleCount(particleCount);
    }
    
    void BlobParticleSimulation::onReady(core::ObjectRef parent, core::StageRef stage) {
        for (auto state(_state.begin()), end(_state.end()); state != end; ++state) {
            state->active = true;
            state->atlasIdx = 0;
            state->color = ParticleColor;
            state->additivity = 0;
        }
        
        update(stage->getTimeState());
    }
    
    void BlobParticleSimulation::update(const core::time_state &timeState) {
        auto blobPhysics = _physics.lock();
        auto physics = blobPhysics->getPhysicsParticles().begin();
        auto state = _state.begin();
        const auto end = _state.end();
        cpBB bb = cpBBInvalid;
        double noiseOffset = timeState.time;
        double noiseStep = 0.42;
        
        for (; state != end; ++state, ++physics, noiseOffset += noiseStep) {
            
            state->position = v2(cpBodyGetPosition(physics->body));
            
            const double angle = _perlin.noise(noiseOffset);
            double cosa, sina;
            __sincos(angle, &sina, &cosa);
            const double radius = 3 * physics->radius * physics->scale * (1-physics->shepherdValue);
            state->right = radius * dvec2(cosa, sina);
            state->up = rotateCCW(state->right);
            
            bb = cpBBExpand(bb, state->position, physics->radius);
        }
        
        _bb = bb;
    }
    
    size_t BlobParticleSimulation::getFirstActive() const {
        return 0;
    };
    
    size_t BlobParticleSimulation::getActiveCount() const {
        return _state.size();
    }
    
    cpBB BlobParticleSimulation::getBB() const {
        return _bb;
    }
    
    namespace {
        
        /// custom FilterStack for running a composite filter on blob's particle system render pass
        class TonemapScreenCompositor : public FilterStack {
        public:
            TonemapScreenCompositor(BlobParticleSystemDrawComponent::config config):
                    _config(config)
            {
                auto compositor = util::loadGlslAsset("kessler/filters/tonemap_compositor.glsl");
                compositor->uniform("Alpha", static_cast<float>(1));
                setScreenCompositeShader(compositor);
            }
            
        protected:
            
            void performScreenComposite(const render_state &state, const gl::GlslProgRef &shader, const gl::FboRef &color) override {
                gl::ScopedTextureBind tonemap(_config.tonemap, 1);
                shader->uniform("Tonemap", 1);
                
                gl::ScopedTextureBind background(_config.background, 2);
                shader->uniform("BackgroundFill", 2);
                
                // use aspect ratio to build a scaling factor to guarantee background fills the viewport without tiling when BackgroundFillRepeat == 1
                const float aspectRatio = static_cast<float>(state.viewport->getAspect());
                vec2 aspect(1,1);
                if (aspectRatio > 1) {
                    aspect.y = 1 / aspectRatio;
                } else {
                    aspect.x = aspectRatio;
                }
                
                shader->uniform("Aspect", aspect);
                shader->uniform("BackgroundFillRepeat", _config.backgroundRepeat);
                shader->uniform("HighlightColor", _config.highlightColor);
                
                FilterStack::performScreenComposite(state, shader, color);
            }
            
        private:
            
            BlobParticleSystemDrawComponent::config _config;
            
        };
        
    }
    
    BlobParticleSystemDrawComponent::BlobParticleSystemDrawComponent(config c):
            elements::ParticleSystemDrawComponent(c)
    {
        auto clearColor = ColorA(ParticleColor,0);
        auto stack = make_shared<TonemapScreenCompositor>(c);
        setFilterStack(stack, clearColor);
    }
    
    gl::GlslProgRef BlobParticleSystemDrawComponent::createDefaultShader() const {
        return util::loadGlslAsset("kessler/shaders/blob_ps.glsl");
    }
    
    
}
