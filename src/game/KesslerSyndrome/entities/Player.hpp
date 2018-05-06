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

namespace game {
    
    SMART_PTR(PlayerPhysicsComponent);
    SMART_PTR(JetpackUnicyclePlayerPhysicsComponent);
    SMART_PTR(PlayerInputComponent);
    SMART_PTR(PlayerDrawComponent);
    SMART_PTR(Player);
    
#pragma mark - PlayerPhysicsComponent
    
    class PlayerPhysicsComponent : public core::PhysicsComponent {
    public:
        
        struct config {
            // initial position of player in world units
            dvec2 position;
            
            double width;
            double height;
            double density;
            double footFriction;
            double bodyFriction;
            
            double jetpackAntigravity;
            double jetpackFuelMax;
            double jetpackFuelConsumptionPerSecond;
            double jetpackFuelRegenerationPerSecond;
        };
        
    public:
        
        PlayerPhysicsComponent(config c);
        
        virtual ~PlayerPhysicsComponent();
        
        // PhysicsComponent
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        
        // PlayerPhysicsComponent
        const config &getConfig() const {
            return _config;
        }
        
        virtual dvec2 getPosition() const = 0;
        
        virtual dvec2 getUp() const = 0;
        
        virtual dvec2 getGroundNormal() const = 0;
        
        virtual bool isTouchingGround() const = 0;
        
        virtual cpBody *getBody() const = 0;
        
        virtual cpBody *getFootBody() const = 0;
        
        virtual cpShape *getBodyShape() const = 0;
        
        virtual cpShape *getFootShape() const = 0;
        
        virtual double getJetpackFuelLevel() const = 0;
        
        virtual double getJetpackFuelMax() const = 0;
        
        virtual dvec2 getJetpackThrustDirection() const = 0;
        
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
        
    protected:
        
        dvec2 _getGroundNormal() const;
        
        bool _isTouchingGround(cpShape *shape) const;
        
        
    protected:
        
        config _config;
        bool _flying;
        double _speed;
        
    };
    
#pragma mark - JetpackUnicyclePlayerPhysicsComponent
    
    class JetpackUnicyclePlayerPhysicsComponent : public PlayerPhysicsComponent {
    public:
        
        JetpackUnicyclePlayerPhysicsComponent(config c);
        
        virtual ~JetpackUnicyclePlayerPhysicsComponent();
        
        // PhysicsComponent
        cpBB getBB() const override;
        
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        
        void step(const core::time_state &timeState) override;
        
        // PlayerPhysicsComponent
        dvec2 getPosition() const override;
        
        dvec2 getUp() const override;
        
        dvec2 getGroundNormal() const override;
        
        bool isTouchingGround() const override;
        
        cpBody *getBody() const override;
        
        cpBody *getFootBody() const override;
        
        cpShape *getBodyShape() const override;
        
        cpShape *getFootShape() const override;
        
        double getJetpackFuelLevel() const override;
        
        double getJetpackFuelMax() const override;
        
        dvec2 getJetpackThrustDirection() const override;
        
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
        
        
    private:
        
        cpBody *_body, *_wheelBody;
        cpShape *_bodyShape, *_wheelShape, *_groundContactSensorShape;
        cpConstraint *_wheelMotor, *_orientationConstraint;
        double _wheelRadius, _wheelFriction, _touchingGroundAcc, _totalMass;
        double _jetpackFuelLevel, _jetpackFuelMax, _lean;
        dvec2 _up, _groundNormal, _jetpackForceDir;
        PlayerInputComponentWeakRef _input;

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
        
        void drawScreen(const core::render_state &renderState) override;
                
    protected:
        
        void drawPlayer(const core::render_state &renderState);
        
        void drawCharge(const core::render_state &renderState);
        
    private:
        
        JetpackUnicyclePlayerPhysicsComponentWeakRef _physics;
        
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
        static PlayerRef create(string name, ci::DataSourceRef playerXmlFile, dvec2 position);
        
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
        PlayerInputComponentRef _input;
        core::HealthComponentRef _health;
        
    };
    
    
}

#endif /* Player_hpp */