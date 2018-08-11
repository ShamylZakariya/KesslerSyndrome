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

namespace game {
    
    SMART_PTR(BlobPhysicsComponent);
    SMART_PTR(BlobDrawComponent);
    SMART_PTR(BlobControllerComponent);
    SMART_PTR(Blob);
    
    class BlobPhysicsComponent : public core::PhysicsComponent {
    public:
        
        struct config {
            dvec2 position;
            double radius;
            double stiffness;
            double damping;
            double elasticity;
            double particleDensity;
            int numParticles;
            
            core::seconds_t introTime;
            core::seconds_t extroTime;
            core::seconds_t pulsePeriod;
            double pulseMagnitude;
            
            cpCollisionType collisionType;
            cpShapeFilter shapeFilter;
            double friction;
            double maxSpeed; // max units per second we can move linearly
            
            config():
                    introTime(1),
                    extroTime(1),
                    position(0,0),
                    radius(20),
                    stiffness(0.5),
                    damping(1),
                    elasticity(0),
                    particleDensity(1),
                    numParticles(24),
                    pulsePeriod(4),
                    pulseMagnitude(0.125),
                    collisionType(0),
                    shapeFilter(CP_SHAPE_FILTER_ALL),
                    friction(0.5),
                    maxSpeed(50)
            {
            }
        };
        
        struct physics_particle
        {
            double radius, scale;
            cpBody *body, *wheelBody;
            cpShape *wheelShape;
            cpConstraint *wheelMotor;
            
            physics_particle():
            radius(0),
            scale(1),
            body(nullptr),
            wheelBody(nullptr),
            wheelShape(nullptr),
            wheelMotor(nullptr)
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
        
    protected:
        
        virtual void createProtoplasmic();
        virtual void updateProtoplasmic(const core::time_state &time);

    protected:
        
        friend class BlobDrawComponent;

        cpBB _bb;
        config _config;
        cpBody *_centralBody;
        cpConstraint *_centralBodyMotorConstraint, *_centralBodyGearConstraint;
        cpShape *_centralBodyShape;
        
        vector<physics_particle> _physicsParticles;
        double _speed, _currentSpeed, _lifecycle;
        core::seconds_t _age;
        
    };
    
    class BlobDrawComponent : public core::EntityDrawComponent {
    public:
        
        struct config {
            
        };
        
    public:
        
        BlobDrawComponent(const config &c);
        
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        cpBB getBB() const override { return _bb; }
        void update(const core::time_state &timeState) override;
        void draw(const core::render_state &renderState) override;
        
    protected:
        
        BlobPhysicsComponentWeakRef _physics;
        cpBB _bb;
        
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
            BlobDrawComponent::config draw;
        };
        
        static BlobRef create(string name, const config &c, core::GamepadRef gamepad);
        
    public:
        
        Blob(string name);
        const BlobPhysicsComponentRef &getBlobPhysicsComponent() const { return _physics; }
        const BlobDrawComponentRef &getBlobDrawComponent() const { return _drawer; }
        const BlobControllerComponentRef &getBlobControllerComponent() const { return _input; }
        
        // Entity
        void onHealthChanged(double oldHealth, double newHealth) override;
        
    protected:

        
        config _config;
        BlobPhysicsComponentRef _physics;
        BlobDrawComponentRef _drawer;
        BlobControllerComponentRef _input;
        
    };
    
    
}

#endif /* Blob_hpp */
