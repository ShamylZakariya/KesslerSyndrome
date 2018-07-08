//
//  PlayerPhysicsComponents.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/8/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef PlayerPhysicsComponents_hpp
#define PlayerPhysicsComponents_hpp

#include "Core.hpp"
#include "PlayerInputComponents.hpp"

namespace game {
    
    SMART_PTR(LegPhysics);
    SMART_PTR(PlayerPhysicsComponent);

    
#pragma mark - LegPhysics
    
    /**
     Simulates leg dynamics for the player. Is not a full Component because it is simple,
     and can be a lightweight thing owned by the PlayerPhysicsComponent
     */
    class LegPhysics {
    public:
        
        enum Phase {
            ProbingForContact,
            Contact,
            Retracting
        };
        
        struct config {
            cpBody *unownedParentBody;
            dvec2 localOrigin;
            dvec2 localDir;
            double rotationExtent;
            double maxLength;
            double restLength;
            double cycleScale;
            double cycleOffset;
        };
        
    public:
        
        LegPhysics(config c);
        
        void step(const core::time_state &time);
        
        dvec2 getWorldOrigin() const { return v2(_worldOrigin); }
        dvec2 getWorldEnd() const { return v2(_worldEnd); }
        dvec2 getWorldUp() const { return v2(cpBodyLocalToWorld(_unownedParentBody, cpv(0,1))); }
        double getMaxLength() const { return _maxLength; }
        Phase getPhase() const { return _phase; }
        
    protected:
        
        friend class LegTessellator;
        bool _canMaintainGroundContact(const core::time_state &time);
        bool _probeForGroundContact();
        cpVect _getLocalProbeDir() const;
        
    protected:
        
        cpBody *_unownedParentBody;
        cpVect _localOrigin, _localEnd, _localDir, _localRest;
        cpVect _worldOrigin, _worldEnd, _worldContact;
        double _maxLength, _restLength, _rotationExtent, _cosRotationExtent, _cycleScale, _cycleOffset;
        Phase _phase;
    };
    
#pragma mark - PlayerPhysicsComponent
    
    class PlayerPhysicsComponent : public core::PhysicsComponent {
    public:
        
        struct config {
            // initial position of player in world units
            dvec2 position;
            // local up vector of world at position (used to position player)
            dvec2 localUp;
            
            double width;
            double height;
            double density;
            double footFriction;
            double bodyFriction;
            double footElasticity;
            
            double jetpackAntigravity;
            double jetpackFuelMax;
            double jetpackFuelConsumptionPerSecond;
            double jetpackFuelRegenerationPerSecond;
            
            size_t numLegs;
            double maxLegExtension;
        };
        
    public:
        
        PlayerPhysicsComponent(config c);
        
        virtual ~PlayerPhysicsComponent();
        
        // PhysicsComponent
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        
        cpBB getBB() const override;
        
        void step(const core::time_state &timeState) override;
        
        // PlayerPhysicsComponent
        const config &getConfig() const {
            return _config;
        }
        
        dvec2 getPosition() const;
        
        dvec2 getUp() const;
        
        dvec2 getGroundNormal() const;
        
        dvec2 getLinearVelocity() const;
        
        bool isTouchingGround() const;
        
        cpBody *getBody() const;
        
        cpBody *getFootBody() const;
        
        cpShape *getBodyShape() const;
        
        cpShape *getFootShape() const;
        
        double getJetpackFuelLevel() const;
        
        double getJetpackFuelMax() const;
        
        dvec2 getJetpackThrustDirection() const;
        
        // Control inputs, called by Player in Player::update
        void setSpeed(double vel) {
            _speed = vel;
        }
        
        double getSpeed() const {
            return _speed;
        }
        
        void setFlying(bool j) {
            _flying = j;
        }
        
        bool isFlying() const {
            return _flying;
        }
        
        struct capsule {
            dvec2 a, b;
            double radius;
        };
        
        capsule getBodyCapsule() const;
        
        struct wheel {
            dvec2 position;
            double radius;
            double radians;
        };
        
        wheel getFootWheel() const;
        
        const vector<LegPhysicsRef> &getLegs() const { return _legSimulations; }
        
    protected:
        
        dvec2 _getGroundNormal() const;
        
        bool _isTouchingGround(cpShape *shape) const;
        
    protected:
        
        config _config;
        bool _flying;
        double _speed;
        
        cpBody *_body, *_wheelBody;
        cpShape *_bodyShape, *_wheelShape, *_groundContactSensorShape;
        cpConstraint *_wheelMotor, *_orientationConstraint;
        double _wheelRadius, _wheelFriction, _touchingGroundAcc, _totalMass;
        double _jetpackFuelLevel, _jetpackFuelMax, _lean;
        dvec2 _up, _groundNormal, _jetpackForceDir;
        PlayerInputComponentWeakRef _input;
        
        vector<LegPhysicsRef> _legSimulations;
    };
    
}

#endif /* PlayerPhysicsComponents_hpp */
