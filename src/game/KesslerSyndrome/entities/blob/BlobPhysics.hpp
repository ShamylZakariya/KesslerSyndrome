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
            
            // length of each tentacle segment
            double tentacleSegmentLength;
            
            // width of the base tentacle segment - they narrow towards the tip
            double tentacleSegmentWidth;
            
            // density of tentacle segments
            double tentacleSegmentDensity;
            
            
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
            numTentacles(1),
            numTentacleSegments(8),
            tentacleSegmentLength(4),
            tentacleSegmentWidth(1),
            tentacleSegmentDensity(1)
            {
            }
        };
        
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
        
        struct tentacle_segment {
            cpBody *body;
            cpConstraint *joint;
            cpConstraint *rotation;
            cpConstraint *angularLimit;
            
            double width, length, angularRange, torque;
            dvec2 position, dir;
            
            tentacle_segment():
            body(nullptr),
            joint(nullptr),
            rotation(nullptr),
            angularLimit(nullptr),
            width(0),
            length(0),
            angularRange(0),
            torque(0)
            {}
        };
        
        struct tentacle {
            vector<tentacle_segment> segments;
            
            // the attachment anchor point (relative to Blob's _centralBody)
            dvec2 attachmentAnchor;
            
            // tentacles are radially attached, this is the angle of this tentacle's root
            double angleOffset;
            
            tentacle():
            angleOffset(0)
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
        
        /// set the blob speed where > 0 is going "right" and < 0 is going "left"
        virtual void setSpeed(double speed);
        double getSpeed() const { return _speed; }
        
        /// set jetpack power, where +1 causes player to rise, and -1 causes player to descend forcefully
        virtual void setJetpackPower(double power);
        double getJetpackPower() const { return _jetpackPower; }
        
        /// get the blob's tentacles
        const vector<tentacle> getTentacles() const { return _tentacles; }
        
        
    protected:
        
        void createProtoplasmic();
        cpBB updateProtoplasmic(const core::time_state &time);
        
        void createTentacles();
        cpBB updateTentacles(const core::time_state &time);
        
    protected:
        
        cpBB _bb;
        config _config;
        cpBody *_centralBody;
        cpConstraint *_centralBodyGearConstraint;
        
        vector<physics_particle> _physicsParticles;
        double _speed, _currentSpeed, _jetpackPower, _currentJetpackPower, _lifecycle, _particleMass;
        dvec2 _jetpackForceDir;
        core::seconds_t _age;
        
        vector<tentacle> _tentacles;
        
    };
    
}

#endif /* BlobPhysics_hpp */
