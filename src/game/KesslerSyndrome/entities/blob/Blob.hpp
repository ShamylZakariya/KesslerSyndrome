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
    
    class BlobPhysicsComponent : public core::Component {
    public:
        
        enum class FluidType {
            
            // creates a gelatinous, soft-body round, bounded shoggoth
            PROTOPLASMIC,
            
            // creates a sloppy amorphous sack of balls shoggoth
            AMORPHOUS
            
        };

        
        struct config {
            FluidType type;
            dvec2 position;
            double radius;
            double stiffness;
            double particleDensity;
            int numParticles;
            
            core::seconds_t introTime;
            core::seconds_t extroTime;
            core::seconds_t pulsePeriod;
            double pulseMagnitude;
            double particleScale;
            
            cpCollisionType collisionType;
            cpShapeFilter shapeFilter;
            double friction;
            double elasticity;
            
            config():
                    type(FluidType::PROTOPLASMIC),
                    introTime(1),
                    extroTime(1),
                    position(0,0),
                    radius(3),
                    stiffness(0.5),
                    particleDensity(1),
                    numParticles(24),
                    pulsePeriod(4),
                    pulseMagnitude(0.125),
                    particleScale(2),
                    collisionType(0),
                    shapeFilter(CP_SHAPE_FILTER_ALL),
                    friction(1),
                    elasticity(0.0)
            {
            }
        };
        
        struct physics_particle
        {
            double radius, slideConstraintLength, scale;
            cpBody *body;
            cpShape *shape;
            cpConstraint *slideConstraint, *springConstraint, *motorConstraint;
            dvec2 offsetPosition;
            
            physics_particle():
            radius(0),
            slideConstraintLength(0),
            scale(1),
            body(nullptr),
            shape(nullptr),
            slideConstraint(nullptr),
            springConstraint(nullptr),
            motorConstraint(nullptr)
            {}
        };
        
    public:
        
        BlobPhysicsComponent(const config &c);
        
        // Component
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        void onCleanup() override;
        void step(const core::time_state &time) override;
        
        // BlobPhysicsComponent
        
        cpBB getBB() const { return _bb; }
        const vector<physics_particle> getPhysicsParticles() const { return _physicsParticles; }
        
        /// set the blob speed where > 0 is going "right" and < 0 is going "left"
        void setSpeed(double speed) { _speed = speed; }
        double getSpeed() const { return _speed; }
        
    protected:
        
        virtual void createProtoplasmic();
        virtual void updateProtoplasmic(const core::time_state &time);

        virtual void createAmorphous();
        virtual void updateAmorphous(const core::time_state &time);

    protected:

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
        core::seconds_t _age;
        
    };
    
    class BlobDrawComponent : public core::DrawComponent {
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
            BlobPhysicsComponent::config physics;
            BlobDrawComponent::config draw;
        };
        
        BlobRef create(string name, const config &c, core::GamepadRef gamepad);
        
    public:
        
        Blob(string name);
        const BlobPhysicsComponentRef &getBlobPhysicsComponent() const { return _physics; }
        const BlobDrawComponentRef &getBlobDrawComponent() const { return _drawer; }
        const BlobControllerComponentRef &getBlobControllerComponent() const { return _input; }
        
    protected:

        
        config _config;
        BlobPhysicsComponentRef _physics;
        BlobDrawComponentRef _drawer;
        BlobControllerComponentRef _input;
        
    };
    
    
}

#endif /* Blob_hpp */
