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
     cpSpace *_space;
     cpBody *_centralBody;
     cpConstraint *_centralBodyConstraint;
     
     cpShapeSet _fluidShapes;
     cpBodySet _fluidBodies;
     cpConstraintSet _fluidConstraints, _motorConstraints, _perimeterConstraints;
     
     vector<physics_particle> _physicsParticles;
     double _speed, _lifecycle, _springStiffness, _bodyParticleRadius;
     seconds_t _age;

     */
    
    BlobPhysicsComponent::BlobPhysicsComponent(const config &c):
    _bb(cpBBInvalid),
    _config(c),
    _space(nullptr),
    _centralBody(nullptr),
    _centralBodyConstraint(nullptr),
    _speed(0),
    _lifecycle(0),
    _springStiffness(0),
    _bodyParticleRadius(0),
    _age(0)
    {
        _config.shapeFilter.group = reinterpret_cast<cpGroup>(this);
    }
    
    void BlobPhysicsComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        _age = 0;
        _space = stage->getSpace()->getSpace();
        switch(_config.type) {
            case FluidType::PROTOPLASMIC:
                createProtoplasmic();
                break;
            case FluidType::AMORPHOUS:
                createAmorphous();
                break;
        }
    }
    
    void BlobPhysicsComponent::onCleanup() {
        for (cpConstraint *c : _perimeterConstraints) {
            cpCleanupAndFree(c);
        }
        
        for (auto &p : _physicsParticles) {
            cpCleanupAndFree(p.springConstraint);
            cpCleanupAndFree(p.slideConstraint);
            cpCleanupAndFree(p.motorConstraint);
            cpCleanupAndFree(p.shape);
            cpCleanupAndFree(p.body);
        }
        
        cpCleanupAndFree(_centralBodyConstraint);
        cpCleanupAndFree(_centralBody);
        _space = nullptr;
    }
    
    void BlobPhysicsComponent::step(const core::time_state &time) {
        
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

        //
        //    create the main body
        //
        
        const double
            radius = std::max<double>(_config.radius * 0.5, 0.5),
            mass = 2 * M_PI * radius * radius,
            moment = cpMomentForCircle( mass, 0, radius, cpvzero );
        
        _centralBody = cpBodyNew( mass, moment );
        cpBodySetUserData( _centralBody, this );
        cpBodySetPosition( _centralBody, cpv( _config.position ));
        cpSpaceAddBody( _space, _centralBody );
        
        _centralBodyConstraint = cpSimpleMotorNew( cpSpaceGetStaticBody( _space), _centralBody, 0 );
        cpSpaceAddConstraint( _space, _centralBodyConstraint );
        _motorConstraints.insert( _centralBodyConstraint );
        
        //
        //    Create particles for the jumbly body
        //
        
        const double
            stiffness = saturate( _config.stiffness ),
            bodyParticleRadius = 1 * (2 * M_PI * radius) / _config.numParticles,
            bodyParticleMass = 1 * M_PI * bodyParticleRadius * bodyParticleRadius,
            bodyParticleMoment = INFINITY,
            bodyParticleSlideExtent = _config.radius - 0.5 * bodyParticleRadius,
            springDamping = lrp<double>(stiffness, 1, 10 );
        
        const auto gravity = getStage()->getGravitation(_config.position);
        _springStiffness = lrp<double>(stiffness, 0.1, 0.5 ) * mass * gravity.magnitude;
        
        for ( int i = 0, N = _config.numParticles; i < N; i++ ) {
            const double offsetAngle = (i * 2 * M_PI ) / N;
            
            physics_particle physicsParticle;
            
            physicsParticle.scale = 1;
            physicsParticle.offsetPosition = radius * dvec2( std::cos( offsetAngle ), std::sin( offsetAngle ));
            dvec2 position = _config.position + physicsParticle.offsetPosition;
            
            physicsParticle.body = cpBodyNew( bodyParticleMass, bodyParticleMoment );
            cpBodySetPosition( physicsParticle.body, cpv(position) );
            cpBodySetUserData( physicsParticle.body, this );
            cpSpaceAddBody( _space, physicsParticle.body );
            _fluidBodies.insert( physicsParticle.body );
            
            physicsParticle.shape = cpCircleShapeNew( physicsParticle.body, bodyParticleRadius, cpvzero );
            cpShapeSetCollisionType( physicsParticle.shape, _config.collisionType );
            cpShapeSetFilter(physicsParticle.shape, _config.shapeFilter);
            cpShapeSetFriction( physicsParticle.shape, _config.friction );
            cpShapeSetElasticity( physicsParticle.shape, _config.elasticity );
            cpSpaceAddShape( _space, physicsParticle.shape );
            _fluidShapes.insert( physicsParticle.shape );
            
            physicsParticle.slideConstraintLength = bodyParticleSlideExtent;
            physicsParticle.slideConstraint = cpSlideJointNew(
                                                              _centralBody,
                                                              physicsParticle.body,
                                                              cpvzero,
                                                              cpvzero,
                                                              0,
                                                              bodyParticleSlideExtent);
            
            cpSpaceAddConstraint( _space, physicsParticle.slideConstraint );
            _fluidConstraints.insert( physicsParticle.slideConstraint );
            
            physicsParticle.springConstraint = cpDampedSpringNew(
                                                                 _centralBody,
                                                                 physicsParticle.body,
                                                                 cpBodyWorldToLocal( _centralBody, cpv(position)),
                                                                 cpvzero,
                                                                 0, // rest length
                                                                 0, // zero stiffness at initialization time
                                                                 springDamping );
            
            cpSpaceAddConstraint( _space, physicsParticle.springConstraint );
            _fluidConstraints.insert( physicsParticle.springConstraint );
            
            _physicsParticles.push_back( physicsParticle );
        }
        
        for ( int i = 0, N = _config.numParticles; i < N; i++ ) {
            cpBody
                *a = _physicsParticles[i].body,
                *b = _physicsParticles[(i+1)%N].body;
            
            cpConstraint *slide = cpSlideJointNew(a, b, cpvzero, cpvzero, 0, cpvdist( cpBodyGetPosition(a), cpBodyGetPosition(b)));
            cpSpaceAddConstraint( _space, slide );
            _perimeterConstraints.insert( slide );
            _fluidConstraints.insert( slide );
        }
    }

    void BlobPhysicsComponent::updateProtoplasmic(const core::time_state &time) {
        double
            lifecycle = _lifecycle,
            phaseOffset = M_PI * (time.time / _config.pulsePeriod),
            phaseIncrement = M_PI / _physicsParticles.size();
        
        cpBB bounds = cpBBInvalid;
        
        //
        //    Update fluid particles. Note, fluidParticles is one larger than physicsParticles,
        //    corresponding to the spawnpoint central body -- so we'll manually update the last one outside the loop
        //
        
        for( auto physicsParticle = _physicsParticles.begin(), end = _physicsParticles.end();
            physicsParticle != end;
            ++physicsParticle )
        {
            //
            //    spring want to hold particle at physics_particle::offsetPosition relative to _centralBody -
            //    we need to ramp up/down spring stiffness to allow this to work
            //
            
            cpSlideJointSetMax( physicsParticle->slideConstraint, lifecycle * physicsParticle->slideConstraintLength );
            cpDampedSpringSetStiffness(physicsParticle->springConstraint, lifecycle * _springStiffness);
            
            //
            //    Scale the particle radius by lifecycle
            //
            
            physicsParticle->scale = lifecycle;
            double currentRadius = physicsParticle->radius * physicsParticle->scale;
            cpCircleShapeSetRadius( physicsParticle->shape, currentRadius );
            
            //
            //    Update position and angle
            //
            
            cpBBExpand( bounds, cpBodyGetPosition( physicsParticle->body ), currentRadius );
            
            phaseOffset += phaseIncrement;
        }
        
        //
        //    Update speed
        //
        
        if ( _centralBodyConstraint )
        {
            const double
                circumference = 2 * _config.radius * M_PI,
                requiredTurns = _speed / circumference,
                motorRate = 2 * M_PI * requiredTurns;
            
            cpSimpleMotorSetRate( _centralBodyConstraint, motorRate );
        }
        
        //
        //    update Aabb
        //
        
        _bb = bounds;
    }
    
    void BlobPhysicsComponent::createAmorphous() {
        //
        //    Create particles for the main body which the jumbly particles are attached to
        //    the main body has no collider, and a gear joint against the world to prevent rotation
        //
        
        const double
            centralBodyRadius = 1,
            centralBodyMass = 1 * M_PI * centralBodyRadius * centralBodyRadius,
            centralBodyMoment = cpMomentForCircle( centralBodyMass, 0, centralBodyRadius, cpvzero );
        
        _centralBody = cpBodyNew( centralBodyMass, centralBodyMoment );
        cpBodySetUserData( _centralBody, this );
        cpBodySetPosition( _centralBody, cpv( _config.position ));
        cpSpaceAddBody( _space, _centralBody );
        
        _centralBodyConstraint = cpGearJointNew( cpSpaceGetStaticBody( _space ), _centralBody, 0, 1 );
        cpSpaceAddConstraint( _space, _centralBodyConstraint );
        
        const double
            overallRadius = std::max<double>(_config.radius, 0.5),
            bodyParticleRadius = 0.5 * (2 * M_PI * overallRadius) / _config.numParticles,
            bodyParticleMass = 1 * M_PI * bodyParticleRadius * bodyParticleRadius,
            bodyParticleMoment = cpMomentForCircle( bodyParticleMass, 0, bodyParticleRadius, cpvzero ),
            bodyParticleSlideExtent = _config.radius + _config.pulseMagnitude * bodyParticleRadius;
        
        _bodyParticleRadius = bodyParticleRadius;
        
        for ( int i = 0, N = _config.numParticles; i < N; i++ )
        {
            double
            offsetAngle = (i * 2 * M_PI ) / N;
            
            physics_particle physicsParticle;
            
            physicsParticle.scale = 1;
            physicsParticle.offsetPosition = overallRadius * dvec2( std::cos( offsetAngle ), std::sin( offsetAngle ));
            dvec2 position = _config.position + physicsParticle.offsetPosition;
            
            physicsParticle.body = cpBodyNew( bodyParticleMass, bodyParticleMoment );
            cpBodySetPosition( physicsParticle.body, cpv(position ));
            cpBodySetUserData( physicsParticle.body, this );
            cpSpaceAddBody( _space, physicsParticle.body );
            _fluidBodies.insert( physicsParticle.body );
            
            physicsParticle.shape = cpCircleShapeNew( physicsParticle.body, bodyParticleRadius, cpvzero );
            cpShapeSetCollisionType( physicsParticle.shape, _config.collisionType );
            cpShapeSetFilter( physicsParticle.shape, _config.shapeFilter );
            cpShapeSetFriction( physicsParticle.shape, _config.friction );
            cpShapeSetElasticity( physicsParticle.shape, _config.elasticity );
            cpSpaceAddShape( _space, physicsParticle.shape );
            _fluidShapes.insert( physicsParticle.shape );
            
            physicsParticle.slideConstraintLength = bodyParticleSlideExtent;
            physicsParticle.slideConstraint = cpSlideJointNew(
                                                              _centralBody,
                                                              physicsParticle.body,
                                                              cpvzero,
                                                              cpvzero,
                                                              0,
                                                              bodyParticleSlideExtent);
            
            cpSpaceAddConstraint( _space, physicsParticle.slideConstraint );
            _fluidConstraints.insert( physicsParticle.slideConstraint );
            
            physicsParticle.motorConstraint = cpSimpleMotorNew( cpSpaceGetStaticBody(_space), physicsParticle.body, 0 );
            cpSpaceAddConstraint( _space, physicsParticle.motorConstraint );
            _motorConstraints.insert( physicsParticle.motorConstraint );
            
            _physicsParticles.push_back( physicsParticle );
        }
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
                requiredTurns = _speed / circumference;
            
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
            
            cpBBExpand( bounds, cpBodyGetPosition( physicsParticle->body ), physicsParticle->radius );
            
            phaseOffset += phaseIncrement;
        }
        
        //
        //    update Aabb
        //
        
        _bb = bounds;
    }


#pragma mark - BlobDrawComponent
    
    /*
     BlobPhysicsComponentWeakRef _physics;
     */
    BlobDrawComponent::BlobDrawComponent(const BlobDrawComponent::config &c):
            DrawComponent(DrawLayers::PLAYER,VisibilityDetermination::FRUSTUM_CULLING)
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
        for (const auto &particle : physics->getPhysicsParticles()) {
            dvec2 position = v2(cpBodyGetPosition(particle.body));
            ci::gl::drawSolidCircle(position, particle.radius);
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
        double speed = 0;
        
        if (isKeyDown(app::KeyEvent::KEY_LEFT)) {
            speed = -1;
        }

        if (isKeyDown(app::KeyEvent::KEY_RIGHT)) {
            speed = -1;
        }
        
        if (_gamepad) {
            speed += _gamepad->getLeftStick().x;
            speed += _gamepad->getDPad().x;
        }
        
        if (speed < -1) { speed = -1; }
        if (speed > +1) { speed = +1; }

        auto physics = _physics.lock();
        physics->setSpeed(speed);
    }
    
#pragma mark - Blob

    BlobRef Blob::create(string name, const config &c, GamepadRef gamepad) {
        auto physics = make_shared<BlobPhysicsComponent>(c.physics);
        auto draw = make_shared<BlobDrawComponent>(c.draw);
        auto input = make_shared<BlobControllerComponent>(gamepad);
        auto blob = Object::create<Blob>(name, { physics, draw, input });
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


    
}
