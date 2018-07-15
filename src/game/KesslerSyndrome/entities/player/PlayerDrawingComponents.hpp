//
//  PlayerDrawingComponents.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/8/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef PlayerDrawingComponents_hpp
#define PlayerDrawingComponents_hpp

#include <cinder/Perlin.h>

#include "core/Core.hpp"
#include "core/util/Svg.hpp"
#include "elements/ParticleSystem/ParticleSystem.hpp"

#include "game/KesslerSyndrome/entities/player/PlayerPhysicsComponents.hpp"


namespace game {
    
    // Defined in PlayerDrawingComponents
    SMART_PTR(LegTessellator);
    SMART_PTR(LegBatchDrawer);
    SMART_PTR(PlayerDrawComponent);
    SMART_PTR(PlayerUIDrawComponent);

    
#pragma mark - LegDrawer
    
    class LegTessellator {
    public:
        
        struct vertex {
            vec2 position;
            ColorA color;
        };
        
        
        LegTessellator(LegPhysicsRef leg):
        _leg(leg)
        {
            _bezierControlPoints.resize(4);
        }
        
        LegPhysicsRef getLeg() const { return _leg.lock(); }
        void computeBezier(const core::render_state &state, Perlin &perlin);
        void tessellate(const core::render_state &state, float width, size_t subdivisions, ColorA color, vector<vertex> &triangles);
        
    protected:
        
        LegPhysicsWeakRef _leg;
        vector<vec2> _bezierControlPoints;
    };
    
    
    class LegBatchDrawer {
    public:
        
        LegBatchDrawer(vector<LegTessellatorRef> legTessellators, ColorA legColor);
        
        void draw(const core::render_state &state, Perlin &perlin);
        
        void setLegColor(ColorA color) { _legColor = color; }
        ColorA getLegColor() const { return _legColor; }
        
    private:
        
        vector<LegTessellatorRef> _legTessellators;
        vector<LegTessellator::vertex> _vertices;
        gl::GlslProgRef _shader;
        gl::VboRef _vbo;
        gl::BatchRef _batch;
        ColorA _legColor;
        
    };
    
#pragma mark - PlayerDrawComponent
    
    class PlayerDrawComponent : public core::EntityDrawComponent {
    public:
        
        PlayerDrawComponent();
        
        virtual ~PlayerDrawComponent();
        
        // DrawComponent
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        
        cpBB getBB() const override;
        
        void update(const core::time_state &timeState) override;
        void draw(const core::render_state &renderState) override;
        
        
    protected:
        
        void drawPlayer(const core::render_state &renderState);
        void drawPlayerPhysics(const core::render_state &renderState);
        void buildThrustParticleSystem(core::StageRef stage);
        
    private:
        
        PlayerPhysicsComponentWeakRef _physics;
        LegBatchDrawerRef _legBatchDrawer;
        core::util::svg::GroupRef _svgDoc, _root, _bulb;
        vector<core::util::svg::GroupRef> _eyes;
        ci::Perlin _perlin;
        elements::ParticleSystemRef _thrustParticleSystem;
        elements::ParticleEmitterRef _thrustParticleEmitter;
        elements::ParticleEmitter::emission_id _thrustEmissionId;
    };
    
    class PlayerUIDrawComponent : public core::ScreenDrawComponent {
    public:
        
        PlayerUIDrawComponent();
        virtual ~PlayerUIDrawComponent();
        
        // ScreenDrawComponent
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        void drawScreen(const core::render_state &renderState) override;
        
    protected:
        
        void drawCharge(const core::render_state &renderState);
        
    private:
        
        PlayerPhysicsComponentWeakRef _physics;
        
    };
    
}

#endif /* PlayerDrawingComponents_hpp */
