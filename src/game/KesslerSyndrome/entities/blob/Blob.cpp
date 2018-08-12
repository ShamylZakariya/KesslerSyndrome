//
//  Blob.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 8/5/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "Blob.hpp"

#include "core/util/Easing.hpp"
#include "game/KesslerSyndrome/GameConstants.hpp"


#include <chipmunk/chipmunk_unsafe.h>

using namespace core;
namespace game {

#pragma mark - BlobPhysicsComponent
    
    /*
     cpBB _bb;
     config _config;
     cpBody *_centralBody;
     cpConstraint *_centralBodyGearConstraint;
     cpShape *_centralBodyShape;
     
     vector<physics_particle> _physicsParticles;
     double _speed, _currentSpeed, _jetpackPower, _currentJetpackPower, _lifecycle, _particleMass;
     core::seconds_t _age;
     */
    
    BlobPhysicsComponent::BlobPhysicsComponent(const config &c):
    _bb(cpBBInvalid),
    _config(c),
    _centralBody(nullptr),
    _centralBodyGearConstraint(nullptr),
    _centralBodyShape(nullptr),
    _speed(0),
    _currentSpeed(0),
    _jetpackPower(0),
    _currentJetpackPower(0),
    _lifecycle(0),
    _particleMass(0),
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
    
    void BlobPhysicsComponent::setJetpackPower(double power) {
        _jetpackPower = clamp<double>(power, -1, 1);
    }
    
    void BlobPhysicsComponent::createProtoplasmic() {
        const cpSpace *space = getSpace()->getSpace();
        
        //
        //    create the main body
        //
        
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
            bodyParticleMoment = cpMomentForCircle(bodyParticleMass, 0, bodyParticleRadius, cpvzero);
        
        const auto gravity = getStage()->getGravitation(_config.position);
        double springStiffness = lrp<double>(saturate( _config.stiffness ), 0.1, 1 ) * bodyParticleMass * gravity.magnitude;
        
        _particleMass = bodyParticleMass;
        
        for ( int i = 0, N = _config.numParticles; i < N; i++ ) {
            const double across =  static_cast<double>(i) / static_cast<double>(N);
            const double offsetAngle = across * 2 * M_PI;
            const dvec2 offsetDir = dvec2( std::cos( offsetAngle ), std::sin( offsetAngle ));
            
            physics_particle particle;
            
            particle.radius = bodyParticleRadius;
            particle.scale = 1;
            dvec2 position = _config.position + _config.radius * across * offsetDir;
            
            particle.body = add(cpBodyNew( bodyParticleMass, bodyParticleMoment ));
            cpBodySetPosition( particle.body, cpv(position) );

            particle.wheelShape = add(cpCircleShapeNew( particle.body, bodyParticleRadius, cpvzero ));
            cpShapeSetFriction( particle.wheelShape, _config.friction );            
            
            cpConstraint *spring = add(cpDampedSpringNew(
                                                         _centralBody,
                                                         particle.body,
                                                         cpv(offsetDir * bodyParticleRadius),
                                                         cpvzero,
                                                         2 * bodyParticleRadius,
                                                         springStiffness,
                                                         damping ));
            
            cpConstraint *grooveConstraint = add(cpGrooveJointNew(_centralBody, particle.body, cpvzero, cpv(offsetDir * _config.radius), cpvzero));
            particle.wheelMotor = add(cpSimpleMotorNew( cpSpaceGetStaticBody(space), particle.body, 0 ));
            
            _physicsParticles.push_back( particle );
            
            // weaken the groove constraint
            cpConstraintSetMaxForce(grooveConstraint, springStiffness * 8);
            cpConstraintSetErrorBias(grooveConstraint, cpConstraintGetErrorBias(grooveConstraint) * 0.01);
            
            // dampen the spring constraint
            cpConstraintSetErrorBias(spring, cpConstraintGetErrorBias(spring) * 0.01);
        }
        
        build(_config.shapeFilter, _config.collisionType);
    }

    void BlobPhysicsComponent::updateProtoplasmic(const core::time_state &time) {
        
        _currentSpeed = lrp(0.25, _currentSpeed, _speed);
        _currentJetpackPower = lrp(0.25, _currentJetpackPower, _jetpackPower);
        
        const cpVect centralBodyPosition = cpBodyGetPosition(_centralBody);
        cpBB bounds = _centralBodyShape ? cpBBExpand(cpBBInvalid, centralBodyPosition, cpCircleShapeGetRadius(_centralBodyShape)) : cpBBInvalid;
        const double acrossStep = static_cast<double>(1) / static_cast<double>(_physicsParticles.size());
        const seconds_t breathPeriod = 2;
        const double lifecycleScale = lrp<double>(_lifecycle, 0.1, 1);
        const double targetSpeed = _currentSpeed * _config.maxSpeed;
        const auto G = getSpace()->getGravity(v2(cpBodyGetPosition(_centralBody)));
        const dvec2 down = G.dir;
        const dvec2 right = rotateCCW(down);

        // thresholds for shepherding state
        const double shepherdEnablementThreshold = _config.radius * 2;
        const double shepherdDisablementThreshold = _config.radius * 0.1;
        const double shepherdEnablementThreshold2 = shepherdEnablementThreshold * shepherdEnablementThreshold;
        const double shepherdDisablementThreshold2 = shepherdDisablementThreshold * shepherdDisablementThreshold;

        dvec2 averageParticlePosition(0,0);
        double across = 0;
        const seconds_t shepherdTransitionDuration = 0.4;
        size_t idx = 0;

        for( auto particle = _physicsParticles.begin(), end = _physicsParticles.end();
            particle != end;
            ++particle, across += acrossStep, ++idx )
        {
            const double radius = particle->radius * particle->scale;
            
            //
            //  Update particle radius over time - a pulsing "breath" cycle, and lifecycle.
            //  Note: When shepherding particle back to flock, we scale radius by 1-shephardValue
            //  to scale the particle down and back up smoothly.
            //
            
            {
                const double phasePosition = (time.time / breathPeriod) + (across * breathPeriod);
                const double breathCycle = cos(phasePosition * 2 * M_PI);
                particle->scale = lrp(breathCycle * 0.5 + 0.5, 0.9, 1.75) * lifecycleScale;
                cpCircleShapeSetRadius(particle->wheelShape, particle->radius * particle->scale * (1-particle->shepherdValue));
            }

            //
            //    Update motor rate
            //
            
            if (particle->wheelMotor) {
                const double circumference = 2 * M_PI * radius;
                const double targetTurnsPerSecond = targetSpeed / circumference;
                const double motorRate = 2 * M_PI * targetTurnsPerSecond;
                cpSimpleMotorSetRate( particle->wheelMotor, motorRate );
            }
            
            //
            //    Update bounding box and average position (which is used when controlling thrust on central body)
            //
            
            const auto position = cpBodyGetPosition(particle->body);
            averageParticlePosition += v2(position);
            bounds = cpBBExpand( bounds, position, radius );
            
            
            //
            //  Monitor state changes for shepherding
            //
            
            switch(particle->shepherdingState) {
                case ShepherdingState::Disabled: {
                    // Transition to Enabling if this particle is beyond threshold distance
                    if (cpvdistsq(position, centralBodyPosition) > shepherdEnablementThreshold2) {
                        particle->shepherdTransitionStartTime = time.time;
                        particle->shepherdingState = ShepherdingState::Enabling;
                        particle->shepherdValue = 0;
                    }
                    break;
                }
                case ShepherdingState::Enabling: {
                    // animate transition to Enabled state
                    seconds_t elapsed = time.time - particle->shepherdTransitionStartTime;
                    if (elapsed < shepherdTransitionDuration) {
                        particle->shepherdValue = core::util::easing::sine_ease_in_out(elapsed / shepherdTransitionDuration);
                    } else {
                        // we've finished enabling, turn off collision detection and got o Enabled state
                        particle->shepherdValue = 1;
                        particle->shepherdingState = ShepherdingState::Enabled;
                        cpShapeSetFilter(particle->wheelShape, CP_SHAPE_FILTER_NONE);
                    }
                    break;
                }
                case ShepherdingState::Enabled: {
                    // Transition to Disabling and re-enable collision detection if we've returned to the flock
                    if (cpvdistsq(position, centralBodyPosition) < shepherdDisablementThreshold2) {
                        particle->shepherdTransitionStartTime = time.time;
                        particle->shepherdingState = ShepherdingState::Disabling;
                        cpShapeSetFilter(particle->wheelShape, _config.shapeFilter);
                    }
                    break;
                }
                case ShepherdingState::Disabling: {
                    // animate transition to Disabled state
                    seconds_t elapsed = time.time - particle->shepherdTransitionStartTime;
                    if (elapsed < shepherdTransitionDuration) {
                        particle->shepherdValue = 1 - core::util::easing::sine_ease_in_out(elapsed / shepherdTransitionDuration);
                    } else {
                        particle->shepherdValue = 0;
                        particle->shepherdingState = ShepherdingState::Disabled;
                    }
                    break;
                }
            }
        }
        
        averageParticlePosition /= _physicsParticles.size();
        
        //
        //  Compute forces to apply to body to aid in locomotion and jetpack control
        //
        
        const double totalMass = cpBodyGetMass(_centralBody) + (_config.numParticles * _particleMass);

        if (abs(_speed) > 0) {
            const cpVect linearVel = cpBodyGetVelocity(_centralBody);
            const double distanceFromCentroid = length(v2(cpBodyGetPosition(_centralBody)) - averageParticlePosition);
            const double forceScale = 1.0 - clamp<double>(distanceFromCentroid / _config.radius, 0, 1);
            
            const double horizontalVel = linearVel.x;
            const double velError = targetSpeed - horizontalVel;
            const double forceMultiplier = 8;
            const double horizontalForce = forceMultiplier * forceScale * totalMass * velError * abs(velError) * time.deltaT;

            cpBodyApplyForceAtLocalPoint(_centralBody, cpv(right * horizontalForce), cpvzero);
        }
        
        if (abs(_currentJetpackPower) > 1e-3) {
            _jetpackForceDir = sign(_currentJetpackPower) * G.dir;
            dvec2 force = -_config.jetpackPower * totalMass * G.magnitude * _jetpackForceDir * abs(_currentJetpackPower);
            cpBodyApplyForceAtWorldPoint(_centralBody, cpv(force), cpBodyLocalToWorld(_centralBody, cpvzero));
        }
        
        //
        //    update Aabb
        //
        
        _bb = bounds;
        notifyMoved();
    }
    
#pragma mark - BlobDrawComponent
    
    namespace {
        
        void draw_axes(cpBody *body, double length, ColorA xAxis = ColorA(1,0,0,1), ColorA yAxis = ColorA(0,1,0,1)) {
            dvec2 origin = v2(cpBodyGetPosition(body));
            dvec2 rotation = v2(cpBodyGetRotation(body));
            
            ci::gl::color(xAxis);
            ci::gl::drawLine(origin, origin + rotation * length);
            
            ci::gl::color(yAxis);
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
        const double axisLength = 16.0 * renderState.viewport->getReciprocalScale();

        for (const auto &particle : physics->getPhysicsParticles()) {
            dvec2 position = v2(cpBodyGetPosition(particle.body));
            double radius = particle.radius * particle.scale;
            ci::gl::color(ColorA(0.3,0.3,0.3,1).lerp(particle.shepherdValue, ColorA(1,0,1,1)));
            ci::gl::drawSolidCircle(position, radius, 16);
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
        auto physics = _physics.lock();

        {
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

            physics->setSpeed(speed);
        }

        {
            double jetpack = 0;
            if (isKeyDown(app::KeyEvent::KEY_UP)) {
                jetpack += 1;
            }

            if (isKeyDown(app::KeyEvent::KEY_DOWN)) {
                jetpack -= 1;
            }
            
            if (_gamepad) {
                jetpack += _gamepad->getLeftStick().y;
                jetpack += _gamepad->getDPad().y;
            }

            physics->setJetpackPower(jetpack);
        }
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
