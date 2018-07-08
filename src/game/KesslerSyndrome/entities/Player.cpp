//
//  Player.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 5/4/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "Player.hpp"

using namespace core;

namespace game {
    
    
#pragma mark - Player
    
    /**
     config _config;
     PlayerPhysicsComponentRef _physics;
     PlayerDrawComponentRef _drawing;
     PlayerInputComponentRef _input;
     */
    PlayerRef Player::create(string name, ci::DataSourceRef playerXmlFile, dvec2 position, dvec2 localUp, dvec2 planetPosition) {
        Player::config config;
        
        XmlTree playerNode = XmlTree(playerXmlFile).getChild("player");
        config.walkSpeed = util::xml::readNumericAttribute<double>(playerNode, "walkSpeed", 1);
        config.runMultiplier = util::xml::readNumericAttribute<double>(playerNode, "runMultiplier", 10);

        config.planetPosition = planetPosition;
        
        //
        //    Physics
        //
        
        XmlTree physicsNode = playerNode.getChild("physics");
        
        config.physics.position = position;
        config.physics.localUp = localUp;
        config.physics.width = util::xml::readNumericAttribute<double>(physicsNode, "width", 5);
        config.physics.height = util::xml::readNumericAttribute<double>(physicsNode, "height", 20);
        config.physics.density = util::xml::readNumericAttribute<double>(physicsNode, "density", 1);
        config.physics.footFriction = util::xml::readNumericAttribute<double>(physicsNode, "footFriction", 1);
        config.physics.footElasticity = util::xml::readNumericAttribute<double>(physicsNode, "footElasticity", 1);
        config.physics.bodyFriction = util::xml::readNumericAttribute<double>(physicsNode, "bodyFriction", 0.2);
        
        XmlTree jetpackNode = physicsNode.getChild("jetpack");
        config.physics.jetpackAntigravity = util::xml::readNumericAttribute<double>(jetpackNode, "antigravity", 10);
        config.physics.jetpackFuelMax = util::xml::readNumericAttribute<double>(jetpackNode, "fuelMax", 1);
        config.physics.jetpackFuelConsumptionPerSecond = util::xml::readNumericAttribute<double>(jetpackNode, "consumption", 0.3);
        config.physics.jetpackFuelRegenerationPerSecond = util::xml::readNumericAttribute<double>(jetpackNode, "regeneration", 0.3);
        
        //
        //    Health
        //
        
        config.health = HealthComponent::loadConfig(playerNode.getChild("health"));
        
        //
        //    Construct
        //
        
        PlayerRef player = make_shared<Player>(name);
        player->build(config);
        
        return player;
    }
    
    Player::Player(string name) :
    Entity(name) {
    }
    
    Player::~Player() {
    }
    
    void Player::onHealthChanged(double oldHealth, double newHealth) {
        Entity::onHealthChanged(oldHealth, newHealth);
    }
    
    void Player::onDeath() {
        Entity::onDeath();
    }
    
    void Player::update(const time_state &time) {
        Entity::update(time);
        
        //
        //    Synchronize - dispatch player input to physics component
        //
        
        if (_health->isAlive()) {
            double direction = 0;
            
            if (_input->isGoingRight()) {
                direction += 1;
            }
            
            if (_input->isGoingLeft()) {
                direction -= 1;
            }
            
            _physics->setSpeed(direction * _config.walkSpeed * (_input->isRunning() ? _config.runMultiplier : 1.0));
            _physics->setFlying(_input->isJumping());
        }
    }
    
    dvec2 Player::getTrackingPosition() const {
        return _physics->getPosition();
    }
    
    dvec2 Player::getTrackingUp() const {
        return normalize(_physics->getPosition() - _config.planetPosition);
    }
    
    void Player::build(config c) {
        _config = c;
        _input = make_shared<PlayerInputComponent>();
        _drawing = make_shared<PlayerDrawComponent>();
        _uiDrawing = make_shared<PlayerUIDrawComponent>();
        _physics = make_shared<PlayerPhysicsComponent>(c.physics);
        _health = make_shared<HealthComponent>(c.health);
        
        addComponent(_physics);
        addComponent(_input);
        addComponent(_health);
        addComponent(_drawing);
        addComponent(_uiDrawing);
    }
    
#pragma mark - PlayerViewportController
    
    /*
     PlayerWeakRef _player;
     dvec2 _planetPosition;
     double _planetRadius;
     */
    
    PlayerViewportController::PlayerViewportController(core::ViewportRef viewport, dvec2 planetPosition, double planetRadius):
    ViewportController(viewport),
    _planetPosition(planetPosition),
    _planetRadius(planetRadius)
    {
        getTraumaConfig().shakeTranslation = dvec2(40,40);
        getTraumaConfig().shakeRotation = 10 * M_PI / 180;
        getTraumaConfig().shakeFrequency = 8;
    }
    
    void PlayerViewportController::onReady(core::ObjectRef parent, core::StageRef stage) {
        Component::onReady(parent, stage);
        auto player = getObjectAs<Player>();
        _player = player;
        setTrackableTarget(player);
    }
    
    void PlayerViewportController::firstUpdate(const core::time_state &time) {
        getViewport()->setLook(getTarget());
    }
    
    void PlayerViewportController::update(const core::time_state &time) {
        // compute a scale factor that allows the planet origin to be visible
        const auto player = _player.lock();
        const dvec2 playerPosition = player->getTrackingPosition();
        const double distanceFromPlanet = max(length(playerPosition - _planetPosition) - (0.5*_planetRadius), 0.01 * _planetRadius);
        const double viewportSize = 0.5 * min(getViewport()->getWidth(), getViewport()->getHeight());
        const double fudge = 1;
        const double scale = fudge * viewportSize / distanceFromPlanet;

        setTargetScale(scale);
        
        ViewportController::update(time);
    }
    
}
