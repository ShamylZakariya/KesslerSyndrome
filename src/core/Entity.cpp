//
//  Entity.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 5/26/17.
//
//

#include "core/Entity.hpp"

namespace core {

    /*
     config _config;
     bool _died;
     */

    HealthComponent::config HealthComponent::loadConfig(const XmlTree &node) {
        config c;
        c.maxHealth = util::xml::readNumericAttribute<double>(node, "max_health", 100);
        c.health = util::xml::readNumericAttribute<double>(node, "health", c.maxHealth);
        c.regenerationRate = util::xml::readNumericAttribute<double>(node, "regeneration_rate", 1);
        return c;
    }

    HealthComponent::HealthComponent(config c) :
            _config(c),
            _died(false) {
    }

    HealthComponent::~HealthComponent() {
    }

    void HealthComponent::setHealth(double health) {
        if (!_died) {
            const double previousHealth = _config.health;
            _config.health = min(health, _config.maxHealth);

            onHealthChanged(previousHealth, _config.health);

            if (_config.health <= 0) {
                die();
            }
        }
    }

    void HealthComponent::takeInjury(double lossOfHealth) {
        setHealth(_config.health - lossOfHealth);
    }

    // Component
    void HealthComponent::update(const time_state &time) {
        if (!_died) {

            if (_config.regenerationRate > 0 && _config.health < _config.maxHealth) {
                const auto previousHealth = _config.health;
                _config.health = min(_config.health + _config.regenerationRate * time.deltaT, _config.maxHealth);
                onHealthChanged(previousHealth, _config.health);
            }
        }
    }

    void HealthComponent::die() {
        _died = true;
        onDeath();
    }

#pragma mark - EntityDrawComponent

    /*
     double _healthiness, _deathCycleProgress;
     bool _alive;
     */

    EntityDrawComponent::EntityDrawComponent(int drawLayer, VisibilityDetermination::style visibilityDetermination) :
            DrawComponent(drawLayer, visibilityDetermination),
            _healthiness(0),
            _deathCycleProgress(0),
            _alive(false)
    {
    }

    EntityDrawComponent::~EntityDrawComponent() {
    }

#pragma mark - Entity
    
    /*
     HealthComponentRef _healthComponent;
     set<EntityDrawComponentRef> _entityDrawComponents;
     */

    Entity::Entity(string name) :
            Object(name) {
    }

    Entity::~Entity() {
    }

    void Entity::onHealthChanged(double oldHealth, double newHealth) {
        CI_LOG_D(getName() << " -- oldHealth: " << oldHealth << " newHealth: " << newHealth);
    }

    void Entity::onDeath() {
        CI_LOG_D(getName() << " -- DEATH!");
        setFinished();
    }

    void Entity::update(const time_state &time) {
        Object::update(time);

        for (const auto &edc : _entityDrawComponents) {
            edc->_alive = _healthComponent->isAlive();
            edc->_healthiness = _healthComponent->getHealthiness();
        }
    }

    void Entity::onFinishing(seconds_t secondsLeft, double amountFinished) {
        Object::onFinishing(secondsLeft, amountFinished);

        for (const auto &edc : _entityDrawComponents) {
            edc->_deathCycleProgress = amountFinished;
        }
    }

    void Entity::addComponent(ComponentRef component) {

        auto edc = dynamic_pointer_cast<EntityDrawComponent>(component);
        if (edc) {
            _entityDrawComponents.insert(edc);
        }

        Object::addComponent(component);

        if (auto hc = dynamic_pointer_cast<HealthComponent>(component)) {
            CI_ASSERT_MSG(!_healthComponent, "Can't assign more than one HealthComponent to an Entity");
            _healthComponent = hc;
            _healthComponent->onDeath.connect(this, &Entity::_onDeath);
            _healthComponent->onHealthChanged.connect(this, &Entity::_onHealthChanged);
        }
    }

    void Entity::removeComponent(ComponentRef component) {
        Object::removeComponent(component);
        if (component == _healthComponent) {
            _healthComponent->onDeath.disconnect(this);
            _healthComponent->onHealthChanged.disconnect(this);
            _healthComponent.reset();
        } else {
            auto edc = dynamic_pointer_cast<EntityDrawComponent>(component);
            if (edc) {
                _entityDrawComponents.erase(edc);
            }
        }
    }

    void Entity::_onHealthChanged(double previousHealth, double newHealth) {
        onHealthChanged(previousHealth, newHealth);
    }

    void Entity::_onDeath() {
        onDeath();
    }

} // namespace core
