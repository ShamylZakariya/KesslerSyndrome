//
//  Player.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 5/4/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef Player_hpp
#define Player_hpp

#include "Core.hpp"
#include <cinder/Perlin.h>

namespace game {
    
    SMART_PTR(LegPhysics);
    SMART_PTR(LegDrawer);
    SMART_PTR(PlayerPhysicsComponent);
    SMART_PTR(PlayerInputComponent);
    SMART_PTR(PlayerDrawComponent);
    SMART_PTR(PlayerUIDrawComponent);
    SMART_PTR(PlayerPhysicsComponent);
    SMART_PTR(Player);
    
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
            ci::Perlin *unownedPerlin;
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
        
        friend class LegDrawer;
        bool _canMaintainGroundContact(const core::time_state &time);
        bool _probeForGroundContact();
        cpVect _getLocalProbeDir() const;

    protected:
        
        cpBody *_unownedParentBody;
        ci::Perlin *_unownedPerlin;
        cpVect _localOrigin, _localEnd, _localDir, _localRest;
        cpVect _worldOrigin, _worldEnd, _worldContact;
        double _maxLength, _restLength, _rotationExtent, _cosRotationExtent, _cycleScale, _cycleOffset;
        Phase _phase;
    };
    
#pragma mark - LegDrawer
    
    class LegDrawer {
    public:
        
        LegDrawer(LegPhysicsRef leg):
        _leg(leg)
        {
            _bezierControlPoints.resize(4);
        }
        
        void draw(const core::render_state &state);
        
    private:
        
        LegPhysicsWeakRef _leg;
        vector<dvec2> _bezierControlPoints;
        
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
        
        virtual dvec2 getPosition() const;
        
        virtual dvec2 getUp() const;
        
        virtual dvec2 getGroundNormal() const;
        
        virtual bool isTouchingGround() const;
        
        virtual cpBody *getBody() const;
        
        virtual cpBody *getFootBody() const;
        
        virtual cpShape *getBodyShape() const;
        
        virtual cpShape *getFootShape() const;
        
        virtual double getJetpackFuelLevel() const;
        
        virtual double getJetpackFuelMax() const;
        
        virtual dvec2 getJetpackThrustDirection() const;
        
        // Control inputs, called by Player in Player::update
        virtual void setSpeed(double vel) {
            _speed = vel;
        }
        
        double getSpeed() const {
            return _speed;
        }
        
        virtual void setFlying(bool j) {
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
        
        ci::Perlin _perlin;
        cpBody *_body, *_wheelBody;
        cpShape *_bodyShape, *_wheelShape, *_groundContactSensorShape;
        cpConstraint *_wheelMotor, *_orientationConstraint;
        double _wheelRadius, _wheelFriction, _touchingGroundAcc, _totalMass;
        double _jetpackFuelLevel, _jetpackFuelMax, _lean;
        dvec2 _up, _groundNormal, _jetpackForceDir;
        PlayerInputComponentWeakRef _input;
        
        vector<LegPhysicsRef> _legSimulations;
    };
    
#pragma mark - PlayerInputComponent
    
    class PlayerInputComponent : public core::InputComponent {
    public:
        
        PlayerInputComponent();
        
        virtual ~PlayerInputComponent();
        
        // actions
        bool isRunning() const;
        
        bool isGoingRight() const;
        
        bool isGoingLeft() const;
        
        bool isJumping() const;
        
        bool isCrouching() const;
                
    private:
        
        int _keyRun, _keyLeft, _keyRight, _keyJump, _keyCrouch;
        
    };
    
#pragma mark - PlayerDrawComponent
        
    class PlayerDrawComponent : public core::EntityDrawComponent {
    public:
        
        PlayerDrawComponent();
        
        virtual ~PlayerDrawComponent();
        
        // DrawComponent
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        
        cpBB getBB() const override;
        
        void draw(const core::render_state &renderState) override;
        
        
    protected:
        
        void drawPlayer(const core::render_state &renderState);
        
        
    private:
        
        PlayerPhysicsComponentWeakRef _physics;
        vector<LegDrawerRef> _legDrawers;
        
    };
    
    class PlayerUIDrawComponent : public core::ScreenDrawComponent {
    public:
        
        PlayerUIDrawComponent();
        virtual ~PlayerUIDrawComponent();
        
        // ScreenDrawComponent
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        void drawScreen(const core::render_state &renderState) override;

    protected:
        
        void drawCharge(const core::render_state &renderState);

    private:
        
        PlayerPhysicsComponentWeakRef _physics;
        
    };
    
#pragma mark - Player
    
    class Player : public core::Entity, public core::Trackable {
    public:
        
        struct config {
            PlayerPhysicsComponent::config physics;
            core::HealthComponent::config health;
            
            // TODO: Add a PlayerDrawComponent::config for appearance control
            
            double walkSpeed;
            double runMultiplier;
            
            config() :
            walkSpeed(1), // 1mps
            runMultiplier(3) {
            }
        };
        
        /**
         Create a player configured via the XML in playerXmlFile, at a given position in world units
         */
        static PlayerRef create(string name, ci::DataSourceRef playerXmlFile, dvec2 position, dvec2 localUp);
        
    public:
        Player(string name);
        
        virtual ~Player();
        
        const config &getConfig() const {
            return _config;
        }
        
        // Entity
        void onHealthChanged(double oldHealth, double newHealth) override;
        
        void onDeath() override;
        
        // Object
        void update(const core::time_state &time) override;
        
        // Tracking
        dvec2 getTrackingPosition() const override;
        
        const PlayerPhysicsComponentRef &getPhysics() const {
            return _physics;
        }
        
        const PlayerInputComponentRef &getInput() const {
            return _input;
        }
        
    protected:
        
        virtual void build(config c);
        
    private:
        
        config _config;
        PlayerPhysicsComponentRef _physics;
        PlayerDrawComponentRef _drawing;
        PlayerUIDrawComponentRef _uiDrawing;
        PlayerInputComponentRef _input;
        core::HealthComponentRef _health;
        
    };
    
    
}

#endif /* Player_hpp */
