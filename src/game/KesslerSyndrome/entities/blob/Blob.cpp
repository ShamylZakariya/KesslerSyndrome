//
//  Blob.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 8/5/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "Blob.hpp"
#include "game/KesslerSyndrome/GameConstants.hpp"

#include <chipmunk/chipmunk_unsafe.h>

using namespace core;
namespace game {

#pragma mark - BlobPhysicsComponent
    
    /*
     cpBB _bb;
     config _config;
     cpBody *_centralBody;
     cpConstraint *_centralBodyMotorConstraint, *_centralBodyGearConstraint;
     cpShape *_centralBodyShape;
     
     vector<physics_particle> _physicsParticles;
     double _speed, _currentSpeed, _lifecycle, _springStiffness, _bodyParticleRadius;
     core::seconds_t _age;
     */
    
    BlobPhysicsComponent::BlobPhysicsComponent(const config &c):
    _bb(cpBBInvalid),
    _config(c),
    _centralBody(nullptr),
    _centralBodyMotorConstraint(nullptr),
    _centralBodyGearConstraint(nullptr),
    _centralBodyShape(nullptr),
    _speed(0),
    _currentSpeed(0),
    _lifecycle(0),
    _springStiffness(0),
    _bodyParticleRadius(0),
    _age(0)
    {
    }
    
    void BlobPhysicsComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        PhysicsComponent::onReady(parent,stage);
        _age = 0;
        createProtoplasmic();
    }
    
    void BlobPhysicsComponent::step(const core::time_state &time) {
        PhysicsComponent::step(time);
        
        _age += time.deltaT;

        if (_age < _config.introTime) {
            _lifecycle = _age / _config.introTime;
        } else {
            _lifecycle = 1;
        }
        
        updateProtoplasmic(time);
    }
    
    void BlobPhysicsComponent::setSpeed(double speed) {
        _speed = clamp<double>(speed, -1, 1);
    }
    
    void BlobPhysicsComponent::createProtoplasmic() {

        // protoplasmic doesn't need the circles to collide
        //_config.shapeFilter.group = reinterpret_cast<cpGroup>(this);
        
        //
        //    create the main body
        //
        
        cpSpace *space = getSpace()->getSpace();
        
        const double
            centralBodyRadius = std::max<double>(_config.radius * 0.5, 0.5),
            centralBodyMass = M_PI * centralBodyRadius * centralBodyRadius,
            centralBodyMoment = cpMomentForCircle( centralBodyMass, 0, centralBodyRadius, cpvzero );
        
        _centralBody = add(cpBodyNew( centralBodyMass, centralBodyMoment ));
        cpBodySetPosition( _centralBody, cpv( _config.position ));
        _centralBodyGearConstraint = add(cpGearJointNew( cpSpaceGetStaticBody( space ), _centralBody, 0, 1 ));
        
        
        //
        //    Create particles for the jumbly body
        //
        
        const double
            damping = lrp<double>(saturate(_config.damping), 0, 100),
            blobCircumference = _config.radius * 2 * M_PI,
            bodyParticleRadius = (blobCircumference / _config.numParticles) * 0.5,
            bodyParticleMass = M_PI * bodyParticleRadius * bodyParticleRadius,
            bodyParticleMoment = cpMomentForCircle(bodyParticleMass, 0, bodyParticleRadius, cpvzero),
            bodyParticleSlideExtent = 1.5 * _config.radius - centralBodyRadius;
        
        const auto gravity = getStage()->getGravitation(_config.position);
        _springStiffness = lrp<double>(saturate( _config.stiffness ), 0.1, 1 ) * 1 * bodyParticleMass * gravity.magnitude;
        
        for ( int i = 0, N = _config.numParticles; i < N; i++ ) {
            const double across =  static_cast<double>(i) / static_cast<double>(N);
            const double offsetAngle = across * 2 * M_PI;
            const dvec2 offsetDir = dvec2( std::cos( offsetAngle ), std::sin( offsetAngle ));
            
            physics_particle physicsParticle;
            
            physicsParticle.radius = bodyParticleRadius;
            physicsParticle.scale = 1;
            dvec2 position = _config.position + _config.radius * across * offsetDir;
            
            physicsParticle.body = add(cpBodyNew( bodyParticleMass, bodyParticleMoment ));
            cpBodySetPosition( physicsParticle.body, cpv(position) );
            
            physicsParticle.shape = add(cpCircleShapeNew( physicsParticle.body, bodyParticleRadius, cpvzero ));
            cpShapeSetFriction( physicsParticle.shape, _config.friction );
            cpShapeSetElasticity( physicsParticle.shape, _config.elasticity );
            
            physicsParticle.slideConstraintLength = bodyParticleSlideExtent;
            
            physicsParticle.springConstraint = add(cpDampedSpringNew(
                                                                 _centralBody,
                                                                 physicsParticle.body,
                                                                 cpv(offsetDir * bodyParticleRadius),
                                                                 cpvzero,
                                                                 2 * bodyParticleRadius,
                                                                 _springStiffness,
                                                                 damping ));
            
            physicsParticle.motorConstraint = add(cpSimpleMotorNew( cpSpaceGetStaticBody(space), physicsParticle.body, 0 ));
            
            _physicsParticles.push_back( physicsParticle );
        }
        
        build(_config.shapeFilter, _config.collisionType);
    }

    void BlobPhysicsComponent::updateProtoplasmic(const core::time_state &time) {
        
        _currentSpeed = lrp(0.25, _currentSpeed, _speed);

        cpBB bounds = _centralBodyShape ? cpBBExpand(cpBBInvalid, cpBodyGetPosition(_centralBody), cpCircleShapeGetRadius(_centralBodyShape)) : cpBBInvalid;
        double across = 0;
        const double acrossStep = static_cast<double>(1) / static_cast<double>(_physicsParticles.size());
        const seconds_t breathPeriod = 2;
        const double lifecycleScale = lrp<double>(_lifecycle, 0.1, 1);
        
        for( auto physicsParticle = _physicsParticles.begin(), end = _physicsParticles.end();
            physicsParticle != end;
            ++physicsParticle, across += acrossStep )
        {
            const double radius = physicsParticle->radius * physicsParticle->scale;
            
            //
            //  Update particle radius over time - a pulsing "breath" cycle, and lifecycle
            //
            
            const double phasePosition = (time.time / breathPeriod) + (across * breathPeriod);
            const double breathCycle = cos(phasePosition * 2 * M_PI);
            physicsParticle->scale = lrp(breathCycle * 0.5 + 0.5, 0.9, 1.75) * lifecycleScale;
            cpCircleShapeSetRadius(physicsParticle->shape, physicsParticle->radius * physicsParticle->scale);

            //
            //    Update motor rate
            //
            
            if ( physicsParticle->motorConstraint )
            {
                double circumference = 2 * M_PI * radius;
                double targetTurnsPerSecond = (_currentSpeed * _config.maxSpeed) / circumference;
                double motorRate = 2 * M_PI * targetTurnsPerSecond;
                cpSimpleMotorSetRate( physicsParticle->motorConstraint, motorRate );
            }
 
            //
            //    Update position and angle
            //
            
            bounds = cpBBExpand( bounds, cpBodyGetPosition( physicsParticle->body ), radius );
        }
        
        //
        //    Update speed
        //
        
        if ( _centralBodyMotorConstraint && _centralBodyShape )
        {
            const double
                circumference = 2 * M_PI * cpCircleShapeGetRadius(_centralBodyShape),
                targetTurnsPerSecond = (_config.maxSpeed * _speed) / circumference,
                motorRate = 2 * M_PI * targetTurnsPerSecond;

            cpSimpleMotorSetRate( _centralBodyMotorConstraint, motorRate );
        }
        
        //
        //    update Aabb
        //
        
        _bb = bounds;
        notifyMoved();
    }
    
#pragma mark - BlobDrawComponent
    
    namespace {
        
        void draw_axes(cpBody *body, double length) {
            dvec2 origin = v2(cpBodyGetPosition(body));
            dvec2 rotation = v2(cpBodyGetRotation(body));
            
            ci::gl::color(ColorA(1,0,0,1));
            ci::gl::drawLine(origin, origin + rotation * length);
            
            ci::gl::color(ColorA(0,1,0,1));
            ci::gl::drawLine(origin, origin + rotateCCW(rotation) * length);
        }
        
    }
    
    /*
     BlobPhysicsComponentWeakRef _physics;
     */
    
    BlobDrawComponent::BlobDrawComponent(const BlobDrawComponent::config &c):
            EntityDrawComponent(DrawLayers::PLAYER,VisibilityDetermination::FRUSTUM_CULLING)
    {}
    
    
    void BlobDrawComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        DrawComponent::onReady(parent,stage);
        _physics = getSibling<BlobPhysicsComponent>();
    }
    
    void BlobDrawComponent::update(const core::time_state &timeState) {
        const auto physics = _physics.lock();
        _bb = physics->getBB();
    }

    void BlobDrawComponent::draw(const core::render_state &renderState) {
        const auto physics = _physics.lock();
        const double axisLength = 10.0 * renderState.viewport->getReciprocalScale();

        ci::gl::color(ColorA(0.3,0.3,0.3,1));
        for (const auto &particle : physics->getPhysicsParticles()) {
            dvec2 position = v2(cpBodyGetPosition(particle.body));
            double radius = particle.radius * particle.scale;
            ci::gl::drawSolidCircle(position, radius, 16);
        }
        
        for (const auto &particle : physics->getPhysicsParticles()) {
            draw_axes(particle.body, axisLength * 0.5);
        }

        if (physics->_centralBodyShape) {
            ci::gl::color(ColorA(0.9,0.3,0.9,0.25));
            ci::gl::drawSolidCircle(v2(cpBodyGetPosition(physics->_centralBody)), cpCircleShapeGetRadius(physics->_centralBodyShape), 16);
        }

        if (physics->_centralBody) {
            draw_axes(physics->_centralBody, axisLength);
        }
    }
    
#pragma mark - BlobControllerComponent

    /*
     BlobPhysicsComponentWeakRef _physics;
     core::GamepadRef _gamepad;
     */
    
    BlobControllerComponent::BlobControllerComponent(core::GamepadRef gamepad):
            InputComponent(0),
            _gamepad(gamepad)
    {}
    
    // component
    void BlobControllerComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        InputComponent::onReady(parent, stage);
        _physics = getSibling<BlobPhysicsComponent>();
    }

    void BlobControllerComponent::update(const core::time_state &time) {
        InputComponent::update(time);
        
        double speed = 0;
        
        if (isKeyDown(app::KeyEvent::KEY_LEFT)) {
            speed += -1;
        }

        if (isKeyDown(app::KeyEvent::KEY_RIGHT)) {
            speed += +1;
        }
        
        if (_gamepad) {
            speed += _gamepad->getLeftStick().x;
            speed += _gamepad->getDPad().x;
        }
        
        speed = clamp<double>(speed,-1,1);

        auto physics = _physics.lock();
        physics->setSpeed(speed);
    }
    
#pragma mark - Blob

    BlobRef Blob::create(string name, const config &c, GamepadRef gamepad) {
        auto physics = make_shared<BlobPhysicsComponent>(c.physics);
        auto draw = make_shared<BlobDrawComponent>(c.draw);
        auto input = make_shared<BlobControllerComponent>(gamepad);
        auto health = make_shared<HealthComponent>(c.health);
        auto blob = Entity::create<Blob>(name, { physics, draw, input, health });
        blob->_config = c;
        blob->_physics = physics;
        blob->_drawer = draw;
        blob->_input = input;
        return blob;
    }
    
    Blob::Blob(string name):
            Entity(name)
    {
    }
    
    void Blob::onHealthChanged(double oldHealth, double newHealth) {
        Entity::onHealthChanged(oldHealth,newHealth);
    }


    
}
