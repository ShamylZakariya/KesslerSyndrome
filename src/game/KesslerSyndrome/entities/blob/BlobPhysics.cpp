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

#include <cinder/Rand.h>
#include <chipmunk/chipmunk_unsafe.h>

using namespace core;
namespace game {
    
    namespace {
        
        enum class LocomotionStyle {
            WHEEL_MOTOR,
            SURFACE_VELOCITY
        };
        
        const LocomotionStyle lomotionStyle = LocomotionStyle::WHEEL_MOTOR;
        
    }
    
#pragma mark - BlobPhysicsComponent
    
    /*
     cpBB _bb;
     config _config;
     cpBody *_centralBody;
     cpConstraint *_centralBodyGearConstraint;
     
     vector<physics_particle> _physicsParticles;
     double _jetpackPower, _currentJetpackPower, _lifecycle, _tentacleAimStrength, _totalMass;
     dvec2 _jetpackForceDir, _motionDirection, _aimDirection, _targetVelocity;
     core::seconds_t _age;
     
     vector<shared_ptr<tentacle>> _tentacles;
     Perlin _tentaclePerlin;
     
     dvec2 _trackingPosition, _trackingUp;
     */
    
    BlobPhysicsComponent::BlobPhysicsComponent(const config &c):
            _bb(cpBBInvalid),
            _config(c),
            _centralBody(nullptr),
            _centralBodyGearConstraint(nullptr),
            _jetpackPower(0),
            _currentJetpackPower(0),
            _jetpackForceDir(0,0),
            _motionDirection(0,0),
            _aimDirection(0,0),
            _targetVelocity(0,0),
            _lifecycle(0),
            _tentacleAimStrength(0),
            _totalMass(0),
            _age(0),
            _tentaclePerlin(4,567)
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

        const auto localGravity = getSpace()->getGravity(v2(cpBodyGetPosition(_centralBody)));
        
        // update protoplasmic shape and tentacles, and our bb
        _bb = updateProtoplasmic(time, localGravity);
        _bb = cpBBExpand(_bb, updateTentacles(time, localGravity));
        
        updateTracking(time, localGravity);
        
        notifyMoved();
    }
    
    void BlobPhysicsComponent::setMotionDirection(dvec2 direction) {
        _motionDirection = direction;
        if (lengthSquared(_motionDirection) > 1) {
            _motionDirection = normalize(_motionDirection);
        }
    }
    
    void BlobPhysicsComponent::setAimDirection(dvec2 dir) {
        _aimDirection = dir;
        if (lengthSquared(_aimDirection) > 1) {
            _aimDirection = normalize(_aimDirection);
        }
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
    
        _totalMass += centralBodyMass;
        
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
            
            _totalMass += bodyParticleMass;
        }
    }
    
    cpBB BlobPhysicsComponent::updateProtoplasmic(const core::time_state &time, const core::GravitationCalculator::force &localGravity) {
        
        _targetVelocity = lrp(0.25, _targetVelocity, _motionDirection * _config.maxSpeed);
        _currentJetpackPower = lrp(0.25, _currentJetpackPower, _motionDirection.y);
        
        const cpVect centralBodyPosition = cpBodyGetPosition(_centralBody);
        cpBB bounds = cpBBInvalid;
        const double acrossStep = static_cast<double>(1) / static_cast<double>(_physicsParticles.size());
        const seconds_t breathPeriod = 2;
        const double lifecycleScale = lrp<double>(_lifecycle, 0.1, 1);
        const dvec2 down = localGravity.dir;
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
            //  Update surface velocity for motion
            //

            switch (lomotionStyle) {
                case LocomotionStyle::WHEEL_MOTOR:
                    if (particle->wheelMotor) {
                        const double circumference = 2 * M_PI * radius;
                        const double targetTurnsPerSecond = _targetVelocity.x / circumference;
                        const double motorRate = 2 * M_PI * targetTurnsPerSecond;
                        cpSimpleMotorSetRate( particle->wheelMotor, motorRate );
                    }
                    break;
                case LocomotionStyle::SURFACE_VELOCITY:
                    cpShapeSetSurfaceVelocity(particle->wheelShape, cpv(-_targetVelocity.x, -_targetVelocity.y));
                    break;
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
        
        if (abs(_targetVelocity.x) > 1e-3) {
            const cpVect linearVel = cpBodyGetVelocity(_centralBody);
            const double distanceFromCentroid = length(v2(cpBodyGetPosition(_centralBody)) - averageParticlePosition);
            const double forceScale = 1.0 - clamp<double>(distanceFromCentroid / _config.radius, 0, 1);
            
            const double horizontalVel = linearVel.x;
            const double velError = _targetVelocity.x - horizontalVel;
            const double forceMultiplier = 8;
            const double horizontalForce = forceMultiplier * forceScale * _totalMass * velError * abs(velError) * time.deltaT;
            
            cpBodyApplyForceAtLocalPoint(_centralBody, cpv(right * horizontalForce), cpvzero);
        }
        
        if (abs(_currentJetpackPower) > 1e-3) {
            _jetpackForceDir = sign(_currentJetpackPower) * localGravity.dir;
            dvec2 force = -_config.jetpackPower * _totalMass * localGravity.magnitude * _jetpackForceDir * abs(_currentJetpackPower);
            cpBodyApplyForceAtWorldPoint(_centralBody, cpv(force), cpBodyLocalToWorld(_centralBody, cpvzero));
        }
        
        return bounds;
    }
    
    double BlobPhysicsComponent::estimateTotalTentacleMass() const {
        double totalLength = 0;
        double segmentLength = _config.tentacleSegmentLength;
        for (int i = 0; i < _config.numTentacleSegments; i++) {
            totalLength += segmentLength;
            segmentLength *= _config.tentacleSegmentLengthFalloff;
        }
        
        return _config.tentacleSegmentWidth * totalLength * _config.tentacleSegmentDensity * 0.5;
    }
    
    namespace {
        double apply_variance(Rand &rng, double value, double variance) {
            return value + rng.nextFloat(-value * variance , +value * variance);
        }
    }

    void BlobPhysicsComponent::createTentacles() {
        const auto G = getSpace()->getGravity(v2(cpBodyGetPosition(_centralBody)));
        Rand rng;
        
        for (size_t i = 0; i < _config.numTentacles; i++) {
            double
                segmentWidth = apply_variance(rng, _config.tentacleSegmentWidth, _config.tentacleVariance),
                segmentLength = apply_variance(rng, _config.tentacleSegmentLength, _config.tentacleVariance),
                segmentWidthIncrement = segmentWidth * real(-1) / _config.numTentacleSegments;

            const double
                minAngularLimit = radians<double>(apply_variance(rng, 7.5, _config.tentacleVariance * 0.5)),
                maxAngularLimit = radians<double>(apply_variance(rng, 40, _config.tentacleVariance * 0.5)),
                torqueMax = estimateTotalTentacleMass() * G.magnitude * 64,
                torqueMin = torqueMax * 0.25,
                angleIncrement = 2 * M_PI / _config.numTentacles,
                angle = i * angleIncrement;

            cpBody *previousBody = _centralBody;
            
            const dvec2 dir(cos(angle), sin(angle));
            dvec2 anchor = dir * _config.radius * 0.0;
            dvec2 position = v2(cpBodyGetPosition(_centralBody)) + anchor;
            
            auto currentTentacle = make_shared<tentacle>();
            currentTentacle->rootBody = _centralBody;
            currentTentacle->attachmentAnchor = anchor;
            currentTentacle->angleOffset = angle;
            currentTentacle->mass = 0;
            currentTentacle->length = 0;
            
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
                seg.angularRange = lrp(distanceAlongTentacle, minAngularLimit, maxAngularLimit);

                // connect this segment to previous body
                seg.pivotJoint = add(cpPivotJointNew( previousBody, seg.body, cpv(position)));


                // gear joint is used to add a waving motion to tentacle
                seg.gearJoint = add(cpGearJointNew( previousBody, seg.body, 0, 1 ));
                cpConstraintSetErrorBias(seg.gearJoint, seg.angularRange * 0.25);
                
                // angular range allow "limp" tentacle to not simply fall loose like a chain
                seg.angularLimitJoint = add(cpRotaryLimitJointNew( previousBody, seg.body, -seg.angularRange, +seg.angularRange ));
                cpConstraintSetErrorBias(seg.angularLimitJoint, seg.angularRange * 0.25);

                //
                //    Add this segment
                //
                
                currentTentacle->segments.push_back( seg );
                currentTentacle->mass += mass;
                currentTentacle->length += segmentLength;
                
                //
                //    Move forward to end of this segment, and apply size falloff
                //
                
                position += dir * segmentLength;
                previousBody = seg.body;
                
                segmentLength *= _config.tentacleSegmentLengthFalloff;
                segmentWidth = std::max<double>( segmentWidth + segmentWidthIncrement, 0.05 );
                
                _totalMass += mass;
            }
            
            //
            //  Now build aiming constraint. AnchorA is on Blob's _centralBody with a force of zero. To aim
            //  we'll move anchorA to a point outside radius of body and ramp up the force
            //
            
            currentTentacle->aimingPinJoint = add(cpPinJointNew(currentTentacle->rootBody, previousBody, cpvzero, cpvzero));
            cpConstraintSetMaxForce(currentTentacle->aimingPinJoint, 0);
            cpPinJointSetDist(currentTentacle->aimingPinJoint, _config.radius * 0.25);
            
            _tentacles.push_back(currentTentacle);            
        }
        
    }
    
    cpBB BlobPhysicsComponent::updateTentacles(const core::time_state &time, const core::GravitationCalculator::force &localGravity) {
        
        // tentacle waving period
        const double wavePeriod = 1;
        const double velocityScaling = 1 - _config.tentacleDamping;

        //
        //  When user is aiming, _tentacleAimStrength is 1, when not aiming, limpness is 1
        //

        const double aimStrength = length(_aimDirection);
        _tentacleAimStrength = lrp<double>(0.25, _tentacleAimStrength, aimStrength);
        const double limpness = 1 - _tentacleAimStrength;
        const double gearJointStrength = lrp<double>(limpness, 0.5, 1.0);
        const double angularLimitStrength = lrp<double>(limpness, 0.0, 1.0);

        
        cpBB bb = cpBBInvalid;
        int tentacleIdx = 0;
        for (const auto &tentacle : _tentacles) {
            int segmentIdx = 0;
            double tentaclePerlinOffset = tentacleIdx * 0.3;
            for (const auto &segment : tentacle->segments) {
                
                const double distanceAlongSegment = static_cast<double>(segmentIdx) / tentacle->segments.size();
                
                // apply damping
                cpBodySetVelocity(segment.body, cpvmult(cpBodyGetVelocity(segment.body), velocityScaling));
                cpBodySetAngularVelocity(segment.body, cpBodyGetAngularVelocity(segment.body) * velocityScaling);

                // apply a gentle undulation
                {
                    const double angle = segment.angularRange * sin( (time.time / wavePeriod) + (segmentIdx * 0.5 + tentacleIdx * 2.5) * (0.5 + 0.5 * _tentaclePerlin.noise(tentaclePerlinOffset)));
                    cpGearJointSetPhase( segment.gearJoint, angle );
                    const double appliedStrength = lrp<double>(limpness, 1.0, distanceAlongSegment);
                    cpConstraintSetMaxForce(segment.gearJoint, appliedStrength * gearJointStrength * segment.torque);
                }

                // ramp up limit force when not aiming
                cpConstraintSetMaxForce(segment.angularLimitJoint, angularLimitStrength * segment.torque);
                
                // expand our bb
                bb = cpBBExpand(bb, cpBodyGetPosition(segment.body), segment.width);
                segmentIdx++;
            }

            // update the aiming constraint
            // aiming anchor is in coordinate system of _centralBody
            const cpVect aimAnchorPosition = cpv(_aimDirection * tentacle->length * 1.2);
            cpPinJointSetAnchorA(tentacle->aimingPinJoint, aimAnchorPosition);
            cpConstraintSetMaxForce(tentacle->aimingPinJoint, aimStrength * tentacle->mass * localGravity.magnitude);

            tentacleIdx++;
        }
        
        return bb;
    }
    
    void BlobPhysicsComponent::updateTracking(const core::time_state &time, const core::GravitationCalculator::force &localGravity) {
        _trackingPosition = v2(cpBodyGetPosition(_centralBody));
        _trackingUp = -localGravity.dir;
    }
    
}
