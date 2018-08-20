//
//  BlobPhysics.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 8/15/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "BlobPhysics.hpp"

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
     
     vector<physics_particle> _physicsParticles;
     double _speed, _currentSpeed, _jetpackPower, _currentJetpackPower, _lifecycle, _particleMass;
     dvec2 _jetpackForceDir;
     core::seconds_t _age;
     
     vector<shared_ptr<tentacle>> _tentacles;
     */
    
    BlobPhysicsComponent::BlobPhysicsComponent(const config &c):
            _bb(cpBBInvalid),
            _config(c),
            _centralBody(nullptr),
            _centralBodyGearConstraint(nullptr),
            _speed(0),
            _currentSpeed(0),
            _jetpackPower(0),
            _currentJetpackPower(0),
            _lifecycle(0),
            _particleMass(0),
            _age(0)
    {}
    
    void BlobPhysicsComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        PhysicsComponent::onReady(parent,stage);
        _age = 0;
        createProtoplasmic();
        createTentacles();
        
        build(_config.shapeFilter, _config.collisionType);
    }
    
    void BlobPhysicsComponent::step(const core::time_state &time) {
        PhysicsComponent::step(time);
        
        _age += time.deltaT;
        
        if (_age < _config.introTime) {
            _lifecycle = _age / _config.introTime;
        } else {
            _lifecycle = 1;
        }
        
        // update protoplasmic shape and tentacles, and our bb
        _bb = updateProtoplasmic(time);
        _bb = cpBBExpand(_bb, updateTentacles(time));
        notifyMoved();
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
            bodyParticleMoment = cpMomentForCircle(bodyParticleMass, 0, bodyParticleRadius, cpvzero),
            estimatedTotalTentacleMass = estimateTotalTentacleMass(),
            tentacleMassAccommodation = estimatedTotalTentacleMass * 0.25;
        
        const auto gravity = getStage()->getGravitation(_config.position);
        double springStiffness = lrp<double>(saturate( _config.stiffness ), 0.1, 1 ) * (bodyParticleMass+tentacleMassAccommodation) * gravity.magnitude;
        
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
            cpConstraintSetErrorBias(grooveConstraint, cpConstraintGetErrorBias(grooveConstraint) * 0.75);
            
            // dampen the spring constraint
            cpConstraintSetErrorBias(spring, cpConstraintGetErrorBias(spring) * 1);
        }
    }
    
    cpBB BlobPhysicsComponent::updateProtoplasmic(const core::time_state &time) {
        
        _currentSpeed = lrp(0.25, _currentSpeed, _speed);
        _currentJetpackPower = lrp(0.25, _currentJetpackPower, _jetpackPower);
        
        const cpVect centralBodyPosition = cpBodyGetPosition(_centralBody);
        cpBB bounds = cpBBInvalid;
        const double acrossStep = static_cast<double>(1) / static_cast<double>(_physicsParticles.size());
        const seconds_t breathPeriod = 2;
        const double lifecycleScale = lrp<double>(_lifecycle, 0.1, 1);
        const double targetSpeed = _currentSpeed * _config.maxSpeed;
        const auto G = getSpace()->getGravity(v2(cpBodyGetPosition(_centralBody)));
        const dvec2 down = G.dir;
        const dvec2 right = rotateCCW(down);
        
        // thresholds for shepherding state
        const double shepherdEnablementThreshold = _config.radius * 2;
        const double shepherdDisablementThreshold = _config.radius * 0.75;
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
        
        const double totalMass = cpBodyGetMass(_centralBody) + (_config.numParticles * _particleMass) + estimateTotalTentacleMass();
        
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
        
        return bounds;
    }
    
    double BlobPhysicsComponent::estimateTotalTentacleMass() const {
        return _config.tentacleSegmentWidth * _config.tentacleSegmentLength * _config.tentacleSegmentDensity * _config.numTentacles;
    }

    
    void BlobPhysicsComponent::createTentacles() {
        const auto G = getSpace()->getGravity(v2(cpBodyGetPosition(_centralBody)));
        
        for (size_t i = 0; i < _config.numTentacles; i++) {
            double
                segmentWidth = _config.tentacleSegmentWidth,
                segmentLength = _config.tentacleSegmentLength,
                segmentWidthIncrement = segmentWidth * real(-1) / _config.numTentacleSegments;

            const double
                minAngularLimit = radians<double>(5),
                maxAngularLimit = radians<double>(30),
                torqueMax = segmentWidth * segmentLength * _config.tentacleSegmentDensity * _config.numTentacleSegments * G.magnitude * 8,
                torqueMin = torqueMax * 0.0125,
                angleIncrement = 2 * M_PI / _config.numTentacles,
                angle = -M_PI_2 + i * angleIncrement;

            cpBody *previousBody = _centralBody;
            
            const dvec2 dir(cos(angle), sin(angle));
            dvec2 anchor = dir * _config.radius * 0.0;
            dvec2 position = v2(cpBodyGetPosition(_centralBody)) + anchor;
            
            auto currentTentacle = make_shared<tentacle>();
            currentTentacle->rootBody = _centralBody;
            currentTentacle->attachmentAnchor = anchor;
            currentTentacle->angleOffset = 0; // TODO: Old code this was an offset value from radial distribution, what should it be here?
            
            for (size_t s = 0; s < _config.numTentacleSegments; s++) {
                const double distanceAlongTentacle = static_cast<double>(s) / _config.numTentacleSegments;
                
                tentacle::segment seg;
                seg.width = segmentWidth;
                seg.length = segmentLength;
                
                const double
                    mass = std::max<double>( seg.width * seg.length * _config.tentacleSegmentDensity, 0.1 ),
                    moment = cpMomentForBox( mass, seg.width, seg.length );
                
                seg.body = add(cpBodyNew( mass, moment ));
                cpBodySetPosition( seg.body, cpv(position + dir * seg.length * 0.5 ));
                
                //
                //    Create joints
                //
                
                seg.torque = lrp( pow(distanceAlongTentacle, 0.25), torqueMax, torqueMin );
                
                seg.joint = add(cpPivotJointNew( previousBody, seg.body, cpv(position)));
                
//                seg.rotation = add(cpGearJointNew( previousBody, seg.body, 0, 1 ));
//                cpConstraintSetMaxForce( seg.rotation, seg.torque );
//                cpConstraintSetErrorBias(seg.rotation, lrp<double>(distanceAlongTentacle, cpConstraintGetErrorBias(seg.rotation), 0.001 * cpConstraintGetErrorBias(seg.rotation)));
                
//                double angle = i == 0 ? (currentTentacle->angleOffset-M_PI_2) : 0;
//                seg.angularLimit = add(cpRotaryLimitJointNew( previousBody, seg.body, angle - seg.angularRange, angle + seg.angularRange ));
                
                const double angularLimit = lrp(distanceAlongTentacle, minAngularLimit, maxAngularLimit);
                seg.angularLimit = add(cpRotaryLimitJointNew( previousBody, seg.body, -angularLimit, +angularLimit ));
//                cpConstraintSetErrorBias(seg.angularLimit, lrp<double>(distanceAlongTentacle, cpConstraintGetErrorBias(seg.angularLimit), 0.0000001 * cpConstraintGetErrorBias(seg.angularLimit)));
                cpConstraintSetMaxBias(seg.angularLimit, 0.02);

                
                //
                //    Add this segment
                //
                
                currentTentacle->segments.push_back( seg );
                
                //
                //    Move forward to end of this segment, and apply size falloff
                //
                
                position += dir * segmentLength;
                previousBody = seg.body;
                
                segmentLength *= 0.9;
                segmentWidth = std::max<double>( segmentWidth + segmentWidthIncrement, 0.05 );
            }
            
            _tentacles.push_back(currentTentacle);            
        }
        
    }
    
    cpBB BlobPhysicsComponent::updateTentacles(const core::time_state &time) {
        cpBB bb = cpBBInvalid;
        for (const auto &tentacle : _tentacles) {
            for (const auto &segment : tentacle->segments) {
                
                // expend our bb
                bb = cpBBExpand(bb, cpBodyGetPosition(segment.body), segment.width);
            }            
        }
        return bb;
    }
    
}
