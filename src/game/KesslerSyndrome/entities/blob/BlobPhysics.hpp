//
//  BlobPhysics.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 8/15/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef BlobPhysics_hpp
#define BlobPhysics_hpp

#include "core/Core.hpp"
#include "elements/Components/ViewportController.hpp"

#include <cinder/Perlin.h>

namespace game {
    
    SMART_PTR(BlobPhysicsComponent);
    class BlobPhysicsComponent : public core::PhysicsComponent {
    public:
        
        struct config {
            dvec2 position;
            double radius;
            double stiffness;
            double damping;
            double particleDensity;
            int numParticles;
            
            core::seconds_t introTime;
            core::seconds_t extroTime;
            
            cpCollisionType collisionType;
            cpShapeFilter shapeFilter;
            double friction;
            
            // max units per second we can move linearly
            double maxSpeed;
            
            // max power of jetpack; a value of 1 == hover, values > 1 make jetpack useful
            double jetpackPower;
            
            // number of tentacles sprouting from the blob
            int numTentacles;
            
            // number of segments per tentacle
            int numTentacleSegments;
            
            // amount of random variance applied to tentacles when created. a value of zero means all are the same,
            // and a value of 0.5 means each parameter can be moulated by 50%, etc.
            double tentacleVariance;
            
            // length of each tentacle segment
            double tentacleSegmentLength;
            
            // amount each segment length is reduced from previous segment
            double tentacleSegmentLengthFalloff;
            
            // width of the base tentacle segment - they narrow towards the tip
            double tentacleSegmentWidth;
            
            // density of tentacle segments
            double tentacleSegmentDensity;
            
            // amount of linear/angular velocity lost per timestep by tentacle bodies.
            double tentacleDamping;
            
            
            config():
            introTime(1),
            extroTime(1),
            position(0,0),
            radius(20),
            stiffness(0.5),
            damping(0.25),
            particleDensity(1),
            numParticles(24),
            collisionType(0),
            shapeFilter(CP_SHAPE_FILTER_ALL),
            friction(0.25),
            maxSpeed(50),
            jetpackPower(2),
            numTentacles(11),
            numTentacleSegments(8),
            tentacleVariance(0.75),
            tentacleSegmentLength(12),
            tentacleSegmentLengthFalloff(0.9),
            tentacleSegmentWidth(6),
            tentacleSegmentDensity(1),
            tentacleDamping(0.025)
            {
            }
        };
        
        /// represents the current shepherding state of particles via physics_particle::shepherdingState
        enum class ShepherdingState {
            Disabled,
            Enabling,
            Enabled,
            Disabling
        };
        
        struct physics_particle {
            double radius, scale, shepherdValue;
            cpBody *body;
            cpShape *wheelShape;
            cpConstraint *wheelMotor;
            core::seconds_t shepherdTransitionStartTime;
            ShepherdingState shepherdingState;
            
            physics_particle():
            radius(0),
            scale(1),
            shepherdValue(0),
            body(nullptr),
            wheelShape(nullptr),
            wheelMotor(nullptr),
            shepherdTransitionStartTime(0),
            shepherdingState(ShepherdingState::Disabled)
            {}
        };
        
        
        struct tentacle {
            struct segment {
                cpBody *body;
                cpConstraint *pivotJoint;
                cpConstraint *gearJoint;
                cpConstraint *angularLimitJoint;
                
                double width, length, angularRange, torque;
                dvec2 dir;
                
                segment():
                body(nullptr),
                pivotJoint(nullptr),
                gearJoint(nullptr),
                angularLimitJoint(nullptr),
                width(0),
                length(0),
                angularRange(0),
                torque(0)
                {}
            };
    
            vector<segment> segments;
            
            // root body this tentacle is attached to - this is Blob's _centralBody
            cpBody *rootBody;
            
            // connects final segment's body to blob's _centralBody with an offset to "aim" the tentacle.
            cpConstraint *aimingPinJoint;

            // the attachment anchor point (relative to Blob's _centralBody)
            dvec2 attachmentAnchor;
            
            // tentacles are radially attached, this is the angle of this tentacle's root
            double angleOffset;
            
            // total mass of tentacle
            double mass;
            
            // total length of tentacle
            double length;
            
            
            tentacle():
            rootBody(nullptr),
            aimingPinJoint(nullptr),
            angleOffset(0),
            mass(0),
            length(0)
            {}
        };
        
    public:
        
        BlobPhysicsComponent(const config &c);
        
        // PhysicsComponent
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        void step(const core::time_state &time) override;
        cpBB getBB() const override { return _bb; }
        
        // BlobPhysicsComponent
        
        /// get the body representing the Blob's central mass
        cpBody* getCentralBody() const { return _centralBody; }
        
        /// get the particles making up the blob
        const vector<physics_particle> getPhysicsParticles() const { return _physicsParticles; }
        
        /// get the direction the blob is to travel
        virtual void setMotionDirection(dvec2 direction);
        dvec2 getMotionDirection() const { return _motionDirection; }
        
        /// set the direction in world coordinates of the current aim
        virtual void setAimDirection(dvec2 direction);
        dvec2 getAimDirection() const { return _aimDirection; }
        
        /// get the blob's tentacles
        const vector<shared_ptr<tentacle>> getTentacles() const { return _tentacles; }
        
        
    protected:
        
        void createProtoplasmic();
        cpBB updateProtoplasmic(const core::time_state &time);
        
        double estimateTotalTentacleMass() const;
        void createTentacles();
        cpBB updateTentacles(const core::time_state &time);
        
    protected:
        
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
        
    };
    
}

#endif /* BlobPhysics_hpp */
