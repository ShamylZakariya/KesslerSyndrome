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
#include "ViewportController.hpp"

#include "PlayerInputComponents.hpp"
#include "PlayerPhysicsComponents.hpp"
#include "PlayerDrawingComponents.hpp"

namespace game {

    SMART_PTR(Player);
    SMART_PTR(PlayerViewportController);

#pragma mark - Player
    
    class Player : public core::Entity, public core::Trackable {
    public:
        
        struct config {
            PlayerPhysicsComponent::config physics;
            core::HealthComponent::config health;
            
            // TODO: Add a PlayerDrawComponent::config for appearance control
            
            double walkSpeed;
            double runMultiplier;
            
            dvec2 planetPosition;
            
            config() :
            walkSpeed(1), // 1mps
            runMultiplier(3) {
            }
        };
        
        /**
         Create a player configured via the XML in playerXmlFile, at a given position in world units
         */
        static PlayerRef create(string name, ci::DataSourceRef playerXmlFile, dvec2 position, dvec2 localUp, dvec2 planetPosition);
        
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
        dvec2 getTrackingUp() const override;

        // Convenience accessors
        
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
    
#pragma mark - PlayerViewportController
    
    class PlayerViewportController : public elements::ViewportController {
    public:

        PlayerViewportController(core::ViewportRef viewport, dvec2 planetPosition, double planetRadius);
        
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        void firstUpdate(const core::time_state &time) override;
        void update(const core::time_state &time) override;
        
    private:
        
        PlayerWeakRef _player;
        dvec2 _planetPosition;
        double _planetRadius;
        
    };
    
}





#endif /* Player_hpp */
