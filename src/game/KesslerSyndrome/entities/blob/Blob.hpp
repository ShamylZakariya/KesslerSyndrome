//
//  Blob.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 8/5/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef Blob_hpp
#define Blob_hpp

#include "core/Core.hpp"
#include "elements/Components/ViewportController.hpp"
#include "elements/ParticleSystem/ParticleSystem.hpp"

namespace game {
    
    SMART_PTR(BlobPhysicsComponent);
    SMART_PTR(BlobControllerComponent);
    SMART_PTR(Blob);
    
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
                    jetpackPower(2)
            {
            }
        };
        
        enum class ShepherdingState {
            Disabled,
            Enabling,
            Enabled,
            Disabling
        };
        
        struct physics_particle
        {
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
        
    public:
        
        BlobPhysicsComponent(const config &c);
        
        // PhysicsComponent
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        void step(const core::time_state &time) override;
        cpBB getBB() const override { return _bb; }
        
        // BlobPhysicsComponent
        
        const vector<physics_particle> getPhysicsParticles() const { return _physicsParticles; }
        
        /// set the blob speed where > 0 is going "right" and < 0 is going "left"
        virtual void setSpeed(double speed);
        double getSpeed() const { return _speed; }
        
        /// set jetpack power, where +1 causes player to rise, and -1 causes player to descend forcefully
        virtual void setJetpackPower(double power);
        double getJetpackPower() const { return _jetpackPower; }
        
    protected:
        
        virtual void createProtoplasmic();
        virtual void updateProtoplasmic(const core::time_state &time);

    protected:
        
        friend class BlobDebugDrawComponent;

        cpBB _bb;
        config _config;
        cpBody *_centralBody;
        cpConstraint *_centralBodyGearConstraint;
        cpShape *_centralBodyShape;
        
        vector<physics_particle> _physicsParticles;
        double _speed, _currentSpeed, _jetpackPower, _currentJetpackPower, _lifecycle, _particleMass;
        dvec2 _jetpackForceDir;
        core::seconds_t _age;
        
    };
    
    class BlobControllerComponent : public core::InputComponent {
    public:
        
        BlobControllerComponent(core::GamepadRef gamepad);
        
        // component
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        void update(const core::time_state &time) override;
        
    protected:

        BlobPhysicsComponentWeakRef _physics;
        core::GamepadRef _gamepad;

    };

    class Blob : public core::Entity {
    public:
        
        struct config {
            core::HealthComponent::config health;
            BlobPhysicsComponent::config physics;
            
            // particle texture for pre-composite particle system rendering. should be square texture with roughly circular, blurred shape
            gl::Texture2dRef particle;
            
            // tonemap texture which maps particle alpha to output color - expects texture shaped something like 128 wide by 1 tall.
            // output color is the color in the tonemap where x is particle alpha scaled [0,1]
            gl::Texture2dRef tonemap;
        };
        
        static BlobRef create(string name, config c, core::GamepadRef gamepad);
        
    public:
        
        Blob(string name);
        const BlobPhysicsComponentRef &getBlobPhysicsComponent() const { return _physics; }
        const BlobControllerComponentRef &getBlobControllerComponent() const { return _input; }
        
        // Entity
        void onHealthChanged(double oldHealth, double newHealth) override;
        
    protected:

        
        config _config;
        BlobPhysicsComponentRef _physics;
        BlobControllerComponentRef _input;
        
    };
    
    
}

#endif /* Blob_hpp */
