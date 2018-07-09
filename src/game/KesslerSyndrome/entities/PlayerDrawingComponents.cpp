//
//  PlayerDrawingComponents.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/8/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "PlayerDrawingComponents.hpp"

#include "Bezier.hpp"
#include "GameConstants.hpp"
#include "GlslProgLoader.hpp"


using namespace core;
namespace game {
    
    
#pragma mark - LegTessellator
    
    /*
     LegPhysicsWeakRef _leg;
     vector<dvec2> _bezierControlPoints;
     vector<vec2> _vertices;
     */
    
    void LegTessellator::computeBezier(const core::render_state &state, Perlin &perlin) {
        const LegPhysicsRef leg = _leg.lock();
        const seconds_t cycle = (state.time * leg->_cycleScale) + leg->_cycleOffset;
        dvec2 cp0Offset(perlin.fBm(cycle + 1), perlin.fBm(cycle + 2));
        dvec2 cp1Offset(perlin.fBm(cycle + 10), perlin.fBm(cycle + 20));
        dvec2 endOffset(perlin.fBm(cycle + 30), perlin.fBm(cycle + 40));
        
        
        // we're doing a simple bezier curve where the control points are equal. we're using
        // the a point along the leg span, offset by its perpendicular
        const auto basePoint = lrp(0.85, leg->getWorldOrigin(), leg->getWorldEnd());
        const auto len = length(leg->getWorldEnd() - leg->getWorldOrigin()) + 1e-4;
        const auto dir = (leg->getWorldEnd() - leg->getWorldOrigin()) / len;
        const auto maxControlPointDeflection = leg->_maxLength * 0.4;
        const auto minControlPointDeflection = leg->_maxLength * 0.05;
        const auto controlPointWiggleRange = leg->_maxLength * 0.5;
        const auto endWiggleRange = leg->_restLength * 0.25;
        const auto controlPointDeflection = lrp((len / leg->_maxLength), minControlPointDeflection, maxControlPointDeflection);
        
        // assign start and end points
        _bezierControlPoints[0] = leg->getWorldOrigin();
        _bezierControlPoints[3] = leg->getWorldEnd() + endWiggleRange * endOffset;
        
        if (dot(rotateCCW(dir), leg->getWorldUp()) > 0) {
            const auto cp = basePoint + (controlPointDeflection * rotateCCW(dir));
            _bezierControlPoints[1] = cp + controlPointWiggleRange * cp0Offset;
            _bezierControlPoints[2] = cp + controlPointWiggleRange * cp1Offset;
        } else {
            const auto cp = basePoint + (controlPointDeflection * rotateCW(dir));
            _bezierControlPoints[1] = cp + controlPointWiggleRange * cp0Offset;
            _bezierControlPoints[2] = cp + controlPointWiggleRange * cp1Offset;
        }
    }
    
    void LegTessellator::tessellate(const core::render_state &state, float width, size_t subdivisions, ColorA color, vector<vertex> &vertices) {
        const auto step = 1 / static_cast<float>(subdivisions);
        
        // bzA bezier segment start
        // bzB bezier segment end
        // bzDir normalized segment dir
        // bzDirPerp CW-perpendicular to dir
        // tAl tesselated segment start left
        // tAr tesselated segment start right
        // tBl tesselated segment end left
        // tBr tesselated segment end right
        
        vec2 bzA = _bezierControlPoints[0];
        vec2 bzB = util::bezier(step, _bezierControlPoints[0], _bezierControlPoints[1], _bezierControlPoints[2], _bezierControlPoints[3]);
        vec2 bzDir = normalize(bzB - bzA);
        vec2 bzDirPerp = rotateCW(bzDir);
        vec2 tBl = bzB - width * bzDirPerp;
        vec2 tBr = bzB + width * bzDirPerp;
        
        // create first triangle
        vertices.push_back({_bezierControlPoints[0], color});
        vertices.push_back({tBl, color});
        vertices.push_back({tBr, color});
        
        bzA = bzB;
        vec2 tAl = tBl;
        vec2 tAr = tBr;
        
        for (size_t seg = 1; seg < subdivisions - 1; seg++) {
            const auto t = (seg+1) * step;
            bzB = util::bezier(t, _bezierControlPoints[0], _bezierControlPoints[1], _bezierControlPoints[2], _bezierControlPoints[3]);
            bzDir = normalize(bzB - bzA);
            bzDirPerp = rotateCW(bzDir);
            
            const auto scaledWidth = lrp<float>(t, width, width * 0.5f);
            tBl = bzB - scaledWidth * bzDirPerp;
            tBr = bzB + scaledWidth * bzDirPerp;
            
            vertices.push_back({tAl, color});
            vertices.push_back({tBl, color});
            vertices.push_back({tAr, color});
            
            vertices.push_back({tAr, color});
            vertices.push_back({tBl, color});
            vertices.push_back({tBr, color});
            
            tAl = tBl;
            tAr = tBr;
        }
        
        // create last triangle
        vertices.push_back({_bezierControlPoints[3], color});
        vertices.push_back({tBr, color});
        vertices.push_back({tBl, color});
    }
    
#pragma mark - LegBatchDrawer
    
    /*
     vector<LegTessellatorRef> _legTessellators;
     vector<LegTessellator::vertex> _vertices;
     gl::GlslProgRef _shader;
     gl::VboRef _vbo;
     gl::BatchRef _batch;
     ColorA _legColor;
     */
    LegBatchDrawer::LegBatchDrawer(vector<LegTessellatorRef> legTessellators, ColorA legColor):
    _legTessellators(legTessellators),
    _shader(core::util::loadGlslAsset("kessler/shaders/player_leg.glsl")),
    _legColor(legColor)
    {
    }
    
    void LegBatchDrawer::draw(const core::render_state &state, Perlin &perlin) {
        //
        // update and tesselate our leg geometry
        //
        
        _vertices.clear();
        for (const auto &lt : _legTessellators) {
            lt->computeBezier(state, perlin);
            lt->tessellate(state, 1.5, 10, _legColor, _vertices);
        }
        
        if (!_vbo) {
            
            //
            //  Lazily create VBO - note we have a constant number of vertices
            //
            
            _vbo = gl::Vbo::create(GL_ARRAY_BUFFER, _vertices, GL_STREAM_DRAW);
            
            geom::BufferLayout particleLayout;
            particleLayout.append(geom::Attrib::POSITION, 2, sizeof(LegTessellator::vertex), offsetof(LegTessellator::vertex, position));
            particleLayout.append(geom::Attrib::COLOR, 4, sizeof(LegTessellator::vertex), offsetof(LegTessellator::vertex, color));
            
            // pair our layout with vbo to create a cinder Batch
            auto mesh = gl::VboMesh::create(static_cast<uint32_t>(_vertices.size()), GL_TRIANGLES, {{particleLayout, _vbo}});
            _batch = gl::Batch::create(mesh, _shader);
        } else {
            
            //
            // Vbo exists, so just copy new positions over
            //
            
            void *gpuMem = _vbo->mapReplace();
            memcpy(gpuMem, _vertices.data(), _vertices.size() * sizeof(LegTessellator::vertex));
            _vbo->unmap();
        }
        
        //
        //  Now draw
        //
        
        _batch->draw();
        
        if (state.testGizmoBit(Gizmos::WIREFRAME)) {
            gl::ScopedColor(ColorA(1,0,1,1));
            
            for (int i = 0; i < _vertices.size(); i += 3) {
                vec2 a = _vertices[i + 0].position;
                vec2 b = _vertices[i + 1].position;
                vec2 c = _vertices[i + 2].position;
                gl::drawLine(a, b);
                gl::drawLine(b, c);
                gl::drawLine(c, a);
            }
        }
    }
    
    
#pragma mark - PlayerDrawComponent
    
    namespace {
        
        void drawStrokedCapsule(dvec2 a, dvec2 b, double radius) {
            const dvec2 center = (a + b) * 0.5;
            const double len = distance(a, b);
            const dvec2 dir = (b - a) / len;
            const double angle = atan2(dir.y, dir.x);
            
            gl::ScopedModelMatrix smm;
            mat4 M = glm::translate(dvec3(center.x, center.y, 0)) * glm::rotate(angle, dvec3(0, 0, 1));
            gl::multModelMatrix(M);
            
            gl::drawStrokedRoundedRect(Rectf(-len / 2 - radius, -radius, +len / 2 + radius, +radius), radius, 8);
        }

        
    }
    
    /*
     PlayerPhysicsComponentWeakRef _physics;
     LegBatchDrawerRef _legBatchDrawer;
     core::util::svg::GroupRef _svgDoc, _root, _bulb;
     vector<core::util::svg::GroupRef> _eyes;
     ci::Perlin _perlin;
     elements::ParticleSystemRef _thrustParticleSystem;
     elements::ParticleEmitterRef _thrustParticleEmitter;
     elements::ParticleEmitter::emission_id _thrustEmissionId;
     */
    
    PlayerDrawComponent::PlayerDrawComponent():
    EntityDrawComponent(DrawLayers::PLAYER, VisibilityDetermination::FRUSTUM_CULLING),
    _svgDoc(util::svg::Group::loadSvgDocument(app::loadAsset("kessler/players/player.svg"), 1)),
    _thrustEmissionId(0)
    {
    }
    
    PlayerDrawComponent::~PlayerDrawComponent() {
    }
    
    void PlayerDrawComponent::onReady(ObjectRef parent, StageRef stage) {
        DrawComponent::onReady(parent, stage);
        auto physics = getSibling<PlayerPhysicsComponent>();
        _physics = physics;
        
        //
        // determine the approximate world size of the physics component, and scale our SVG accordingly
        //
        
        const auto physicsHeight = physics->getConfig().height;
        const auto svgHeight = _svgDoc->getBB().t - _svgDoc->getBB().b;
        const auto scale = physicsHeight / svgHeight;
        _svgDoc->setScale(scale);
        
        //
        //  Find various components that we animate
        //
        
        _root = _svgDoc->find("artboard/player/root");
        _eyes = _svgDoc->find("artboard/player/bulb/eyes")->getGroups();
        _bulb = _svgDoc->find("artboard/player/bulb");
        
        //
        //  Now determine leg color by examining the SVG
        //
        
        const auto rootShape = _root->getGroupNamed("leg_root")->getShapes().front();
        const auto appearance = rootShape->getAppearance();
        const auto legColor = ColorA(appearance->getFillColor(), appearance->getFillAlpha());
        
        //
        // create the leg tessellators, and a batch drawer to render them
        //
        
        vector<LegTessellatorRef> tesselators;
        for (const auto &leg : physics->getLegs()) {
            tesselators.push_back(make_shared<LegTessellator>(leg));
        }
        _legBatchDrawer = make_shared<LegBatchDrawer>(tesselators, legColor);
        
        //
        //  Build particle system which draws thrust
        //
        
        buildThrustParticleSystem(stage);
    }
    
    cpBB PlayerDrawComponent::getBB() const {
        if (PlayerPhysicsComponentRef ppc = _physics.lock()) {
            return ppc->getBB();
        }
        
        return cpBBInvalid;
    }
    
    void PlayerDrawComponent::update(const core::time_state &timeState) {
        PlayerPhysicsComponentRef physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");
        
        {
            auto cycle = timeState.time * 1 * M_PI;
            auto step = 1 / static_cast<double>(_eyes.size());
            for (auto &eye : _eyes) {
                auto phase = lrp<double>((cos(cycle) + 1) * 0.5, 0.25, 1.0);
                cycle += M_PI * step;
                eye->setOpacity(phase);
            }
        }
        
        {
            auto phase = timeState.time * M_PI + _perlin.fBm(timeState.time * 0.1);
            auto v = (cos(phase) + 1) * 0.5;
            _bulb->setScale(lrp(v, 0.9, 1.1), lrp(1-v, 0.9, 1.1));
        }
        
        _thrustParticleEmitter->setVelocity(physics->getLinearVelocity());
        
        if (physics->isFlying()) {
            const auto thrustDir = physics->getJetpackThrustDirection();
            const PlayerPhysicsComponent::wheel footWheel = physics->getFootWheel();
            const auto pos = footWheel.position;
            if (_thrustEmissionId == 0) {
                _thrustEmissionId = _thrustParticleEmitter->emit(pos, thrustDir, 120);
            } else {
                _thrustParticleEmitter->setEmissionPosition(_thrustEmissionId, pos, thrustDir);
            }
        } else {
            _thrustParticleEmitter->cancel(_thrustEmissionId);
            _thrustEmissionId = 0;
        }
    }
    
    void PlayerDrawComponent::draw(const render_state &renderState) {
        drawPlayer(renderState);
        
        if (renderState.testGizmoBit(Gizmos::PHYSICS)) {
            drawPlayerPhysics(renderState);
        }
    }
    
    void PlayerDrawComponent::drawPlayer(const render_state &renderState) {
        PlayerPhysicsComponentRef physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");
        
        _legBatchDrawer->draw(renderState, _perlin);
        
        _svgDoc->setPosition(physics->getPosition());
        _svgDoc->setRotation(v2(cpBodyGetRotation(physics->getBody())));
        _svgDoc->draw(renderState);
    }
    
    void PlayerDrawComponent::drawPlayerPhysics(const core::render_state &renderState) {
        PlayerPhysicsComponentRef physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");
        
        gl::ScopedBlendAlpha sba;
        
        const PlayerPhysicsComponent::wheel FootWheel = physics->getFootWheel();
        const PlayerPhysicsComponent::capsule BodyCapsule = physics->getBodyCapsule();
        
        // draw the wheel
        gl::color(1, 1, 1, 0.5);
        gl::drawStrokedCircle(FootWheel.position, FootWheel.radius, 32);
        gl::color(0, 0, 0, 0.5);
        gl::drawLine(FootWheel.position, FootWheel.position + FootWheel.radius * dvec2(cos(FootWheel.radians), sin(FootWheel.radians)));
        
        // draw the capsule
        gl::color(1, 1, 1, 0.5);
        drawStrokedCapsule(BodyCapsule.a, BodyCapsule.b, BodyCapsule.radius);
        
        // draw the ground normal indicator
        gl::color(1, 0, 0, 0.5);
        gl::drawLine(physics->getPosition(), physics->getPosition() + physics->getGroundNormal() * 10.0);
        gl::drawSolidCircle(physics->getPosition(), 1, 12);
        
        // draw the jetpack thrust
        if (physics->isFlying()) {
            const auto thrustDir = physics->getJetpackThrustDirection();
            const auto angle = atan2(thrustDir.y, thrustDir.x) + M_PI_2;
            const auto pos = FootWheel.position;
            
            gl::ScopedModelMatrix smm;
            gl::multModelMatrix(glm::translate(dvec3(pos, 0)) * glm::rotate(angle, dvec3(0, 0, 1)));
            
            gl::color(1, 0, 0, 0.5);
            dvec2 a = dvec2(-FootWheel.radius, 0);
            dvec2 b = dvec2(FootWheel.radius, 0);
            dvec2 c = dvec2(0, -4 * FootWheel.radius);
            gl::drawLine(a,b);
            gl::drawLine(b,c);
            gl::drawLine(c,a);
        }
    }
    
    void PlayerDrawComponent::buildThrustParticleSystem(StageRef stage) {
        PlayerPhysicsComponentRef physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");
        
        const auto footWheel = physics->getFootWheel();
        const auto maxRadius = footWheel.radius * 0.3;
        
        auto image = loadImage(app::loadAsset("kessler/textures/Explosion.png"));
        gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
        
        using namespace elements;
        
        ParticleSystem::config config;
        config.maxParticleCount = 500;
        config.keepSorted = true;
        config.drawConfig.drawLayer = getLayer() + 1;
        config.drawConfig.textureAtlas = gl::Texture2d::create(image, fmt);
        config.drawConfig.atlasType = Atlas::TwoByTwo;
        
        _thrustParticleSystem = ParticleSystem::create("PlayerDrawComponent::_thrustParticleSystem", config);
        stage->addObject(_thrustParticleSystem);
        
        //
        // build fire, smoke and spark templates
        //
        
        particle_prototype fire;
        fire.atlasIdx = 0;
        fire.lifespan = 0.75;
        fire.radius = {0, maxRadius * 1.5, maxRadius, maxRadius * 0.1, 0};
        fire.damping = { 0 };
        fire.additivity = { 1, 0.7 };
        fire.mass = {0};
        fire.initialVelocity = 50;
        fire.gravitationLayerMask = GravitationLayers::GLOBAL;
        fire.color = {ColorA(0,0.5,1,0),ColorA(0,0.5,1,1),ColorA(1,0.5,0,1),ColorA(1,0,0,0)};
        
        particle_prototype smoke;
        smoke.atlasIdx = 0;
        smoke.lifespan = 0.75;
        smoke.radius = {0, maxRadius, maxRadius * 2, maxRadius * 0};
        smoke.damping = {0,0,0,0.02};
        smoke.additivity = 0;
        smoke.mass = {0};
        smoke.initialVelocity = 20;
        smoke.gravitationLayerMask = GravitationLayers::GLOBAL;
        smoke.color = ColorA(0.9, 0.9, 0.9, 0.5);
        
        _thrustParticleEmitter = _thrustParticleSystem->createEmitter();
        _thrustParticleEmitter->add(fire, ParticleEmitter::Source(2, 0.2, 0.3), 2);
        _thrustParticleEmitter->add(smoke, ParticleEmitter::Source(2, 0.3, 0.3), 1);
    }
    
#pragma mark - PlayerUIDrawComponent
    
    /*
     PlayerPhysicsComponentWeakRef _physics;
     */
    
    PlayerUIDrawComponent::PlayerUIDrawComponent():
    ScreenDrawComponent(ScreenDrawLayers::PLAYER)
    {}
    
    PlayerUIDrawComponent::~PlayerUIDrawComponent()
    {}
    
    void PlayerUIDrawComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        ScreenDrawComponent::onReady(parent, stage);
        _physics = getSibling<PlayerPhysicsComponent>();
    }
    
    void PlayerUIDrawComponent::drawScreen(const core::render_state &renderState) {
        drawCharge(renderState);
    }
    
    void PlayerUIDrawComponent::drawCharge(const core::render_state &renderState) {
        auto physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");
        
        Rectd bounds = renderState.viewport->getBounds();
        int w = 20;
        int h = 60;
        int p = 10;
        Rectf chargeRectFrame(bounds.getWidth() - p, p, bounds.getWidth() - p - w, p + h);
        
        gl::color(0, 1, 1, 1);
        gl::drawStrokedRect(chargeRectFrame);
        
        double charge = saturate(physics->getJetpackFuelLevel() / physics->getJetpackFuelMax());
        int fillHeight = static_cast<int>(ceil(charge * h));
        Rectf chargeRectFill(bounds.getWidth() - p, p + h - fillHeight, bounds.getWidth() - p - w, p + h);
        gl::drawSolidRect(chargeRectFill);
    }
    
}
