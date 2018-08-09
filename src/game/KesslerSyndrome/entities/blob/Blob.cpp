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
     double _speed, _lifecycle, _springStiffness, _bodyParticleRadius;
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
    _lifecycle(0),
    _springStiffness(0),
    _bodyParticleRadius(0),
    _age(0)
    {
    }
    
    void BlobPhysicsComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        PhysicsComponent::onReady(parent,stage);
        _age = 0;
        switch(_config.type) {
            case FluidType::PROTOPLASMIC:
                createProtoplasmic();
                break;
            case FluidType::AMORPHOUS:
                createAmorphous();
                break;
        }
    }
    
    void BlobPhysicsComponent::step(const core::time_state &time) {
        PhysicsComponent::step(time);
        
        _age += time.deltaT;

        if (_age < _config.introTime) {
            _lifecycle = _age / _config.introTime;
        } else {
            _lifecycle = 1;
        }
        
        switch(_config.type ) {
            case FluidType::PROTOPLASMIC:
                updateProtoplasmic(time);
                break;
            case FluidType::AMORPHOUS:
                updateAmorphous(time);
                break;
        }
    }
    
    
    void BlobPhysicsComponent::createProtoplasmic() {

        // protoplasmic doesn't need the circles to collide
        _config.shapeFilter.group = reinterpret_cast<cpGroup>(this);
        
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
        _centralBodyShape = add(cpCircleShapeNew(_centralBody, centralBodyRadius, cpvzero));
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
            bodyParticleSlideExtent = _config.radius - centralBodyRadius;
        
        const auto gravity = getStage()->getGravitation(_config.position);
        _springStiffness = lrp<double>(saturate( _config.stiffness ), 0.1, 1 ) * 0.25 * bodyParticleMass * gravity.magnitude;
        
        for ( int i = 0, N = _config.numParticles; i < N; i++ ) {
            const double offsetAngle = (i * 2 * M_PI ) / N;
            
            physics_particle physicsParticle;
            
            physicsParticle.radius = bodyParticleRadius;
            physicsParticle.scale = 1;
            physicsParticle.offsetPosition = _config.radius * dvec2( std::cos( offsetAngle ), std::sin( offsetAngle ));
            dvec2 position = _config.position + physicsParticle.offsetPosition;
            
            physicsParticle.body = add(cpBodyNew( bodyParticleMass, bodyParticleMoment ));
            cpBodySetPosition( physicsParticle.body, cpv(position) );
            
            physicsParticle.shape = add(cpCircleShapeNew( physicsParticle.body, bodyParticleRadius, cpvzero ));
            cpShapeSetFriction( physicsParticle.shape, _config.friction );
            cpShapeSetElasticity( physicsParticle.shape, _config.elasticity );
            
            physicsParticle.slideConstraintLength = bodyParticleSlideExtent;
            physicsParticle.slideConstraint = add(cpSlideJointNew(
                                                              _centralBody,
                                                              physicsParticle.body,
                                                              cpvzero,
                                                              cpvzero,
                                                              bodyParticleSlideExtent - centralBodyRadius,
                                                              bodyParticleSlideExtent));
            
            physicsParticle.springConstraint = add(cpDampedSpringNew(
                                                                 _centralBody,
                                                                 physicsParticle.body,
                                                                 cpBodyWorldToLocal( _centralBody, cpv(position)),
                                                                 cpvzero,
                                                                 bodyParticleSlideExtent,
                                                                 _springStiffness,
                                                                 damping ));
            
            physicsParticle.motorConstraint = add(cpSimpleMotorNew( cpSpaceGetStaticBody(space), physicsParticle.body, 0 ));
            
            _physicsParticles.push_back( physicsParticle );
        }
        
        for ( int i = 0, N = _config.numParticles; i < N; i++ ) {
            cpBody
                *a = _physicsParticles[i].body,
                *b = _physicsParticles[(i+1)%N].body;
            
            add(cpSlideJointNew(a, b, cpvzero, cpvzero, 0, cpvdist( cpBodyGetPosition(a), cpBodyGetPosition(b))));
        }
        
        build(_config.shapeFilter, _config.collisionType);
    }

    void BlobPhysicsComponent::updateProtoplasmic(const core::time_state &time) {

        cpBB bounds = cpBBExpand(cpBBInvalid, cpBodyGetPosition(_centralBody), cpCircleShapeGetRadius(_centralBodyShape));
        
        for( auto physicsParticle = _physicsParticles.begin(), end = _physicsParticles.end();
            physicsParticle != end;
            ++physicsParticle )
        {
            //
            //    Update motor rate
            //
            
            double radius = physicsParticle->radius * physicsParticle->scale;
            if ( physicsParticle->motorConstraint )
            {
                double circumference = 2 * M_PI * radius;
                double targetTurnsPerSecond = (_speed * _config.maxSpeed) / circumference;
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
    
    void BlobPhysicsComponent::createAmorphous() {
        
        // protoplasmic doesn't need the circles to collide
        _config.shapeFilter.group = reinterpret_cast<cpGroup>(this);

        //
        //    Create particles for the main body which the jumbly particles are attached to
        //    the main body has no collider, and a gear joint against the world to prevent rotation
        //
        
        cpSpace *space = getSpace()->getSpace();
        
        const double
            centralBodyRadius = 1,
            centralBodyMass = 1 * M_PI * centralBodyRadius * centralBodyRadius,
            centralBodyMoment = cpMomentForCircle( centralBodyMass, 0, centralBodyRadius, cpvzero );
        
        _centralBody = add(cpBodyNew( centralBodyMass, centralBodyMoment ));
        cpBodySetUserData( _centralBody, this );
        cpBodySetPosition( _centralBody, cpv( _config.position ));
        
        _centralBodyMotorConstraint = add(cpGearJointNew( cpSpaceGetStaticBody( space ), _centralBody, 0, 1 ));
        
        const double
            overallRadius = std::max<double>(_config.radius, 0.5),
            bodyParticleRadius = 0.5 * (2 * M_PI * overallRadius) / _config.numParticles,
            bodyParticleMass = 1 * M_PI * bodyParticleRadius * bodyParticleRadius,
            bodyParticleMoment = cpMomentForCircle( bodyParticleMass, 0, bodyParticleRadius, cpvzero ),
            bodyParticleSlideExtent = (_config.radius + _config.pulseMagnitude * bodyParticleRadius) * 2;
        
        _bodyParticleRadius = bodyParticleRadius;
        
        for ( int i = 0, N = _config.numParticles; i < N; i++ )
        {
            double
            offsetAngle = (i * 2 * M_PI ) / N;
            
            physics_particle physicsParticle;
            
            physicsParticle.radius = overallRadius;
            physicsParticle.scale = 1;
            physicsParticle.offsetPosition = overallRadius * dvec2( std::cos( offsetAngle ), std::sin( offsetAngle ));
            dvec2 position = _config.position + physicsParticle.offsetPosition;
            
            physicsParticle.body = add(cpBodyNew( bodyParticleMass, bodyParticleMoment ));
            cpBodySetPosition( physicsParticle.body, cpv(position ));
            cpBodySetUserData( physicsParticle.body, this );
            
            physicsParticle.shape = add(cpCircleShapeNew( physicsParticle.body, bodyParticleRadius, cpvzero ));
            cpShapeSetFriction( physicsParticle.shape, _config.friction );
            cpShapeSetElasticity( physicsParticle.shape, _config.elasticity );
            
            physicsParticle.slideConstraintLength = bodyParticleSlideExtent;
            physicsParticle.slideConstraint = add(cpSlideJointNew(
                                                              _centralBody,
                                                              physicsParticle.body,
                                                              cpvzero,
                                                              cpvzero,
                                                              4 * bodyParticleRadius,
                                                              bodyParticleSlideExtent));
            
            
            physicsParticle.motorConstraint = add(cpSimpleMotorNew( cpSpaceGetStaticBody(space), physicsParticle.body, 0 ));

            _physicsParticles.push_back( physicsParticle );
        }
        
        build(_config.shapeFilter, _config.collisionType);
    }
 
    void BlobPhysicsComponent::updateAmorphous(const core::time_state &time) {
        double
            lifecycle = _lifecycle,
            phaseOffset = M_PI * time.time / _config.pulsePeriod,
            phaseIncrement = 2 * M_PI / _physicsParticles.size(),
            pulseMagnitude = _config.pulseMagnitude,
            motorRate = 0;
        
        cpBB bounds = cpBBInvalid;
        
        
        {
            // compute required rotation rate - note, we ignore lifecycle to avoid div by zero.
            const double
                circumference = 2 * _bodyParticleRadius * M_PI,
                requiredTurns = _config.maxSpeed * _speed / circumference;
            
            motorRate = 2 * M_PI * requiredTurns / std::sqrt(_physicsParticles.size());
        }
        
        for( auto physicsParticle = _physicsParticles.begin(), end = _physicsParticles.end();
            physicsParticle != end;
            ++physicsParticle )
        {
            //
            //    spring want to hold particle at physics_particle::offsetPosition relative to _centralBody -
            //    we need to ramp up/down spring stiffness to allow this to work
            //
            
            cpSlideJointSetMax( physicsParticle->slideConstraint, lifecycle * physicsParticle->slideConstraintLength );
            
            //
            //    Scale the particle radius by lifecycle
            //
            
            double pulse = 1 + pulseMagnitude * std::cos( phaseOffset );
            physicsParticle->scale = lifecycle * pulse;
            double currentRadius = physicsParticle->radius * physicsParticle->scale;
            cpCircleShapeSetRadius( physicsParticle->shape, std::max<double>( currentRadius, 0.01 ) );
            
            //
            //    Update motor rate
            //
            
            if ( physicsParticle->motorConstraint )
            {
                cpSimpleMotorSetRate( physicsParticle->motorConstraint, motorRate );
            }
            
            bounds = cpBBExpand( bounds, cpBodyGetPosition( physicsParticle->body ), physicsParticle->radius );
            
            phaseOffset += phaseIncrement;
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
            ci::gl::drawSolidCircle(position, radius);
        }
        
        for (const auto &particle : physics->getPhysicsParticles()) {
            draw_axes(particle.body, axisLength * 0.5);
        }
        
        if (physics->_centralBody) {
            ci::gl::color(ColorA(0.9,0.3,0.9,0.25));
            ci::gl::drawSolidCircle(v2(cpBodyGetPosition(physics->_centralBody)), cpCircleShapeGetRadius(physics->_centralBodyShape));
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
