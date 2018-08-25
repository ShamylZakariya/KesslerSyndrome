//
//  Blob.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 8/5/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "Blob.hpp"

#include "game/KesslerSyndrome/GameConstants.hpp"

using namespace core;
namespace game {
    
#pragma mark - BlobControllerComponent

    /*
     core::GamepadRef _gamepad;
     dvec2 _motionDir, _aimDir;
     */
    
    BlobControllerComponent::BlobControllerComponent(core::GamepadRef gamepad):
            InputComponent(0),
            _gamepad(gamepad),
            _motionDir(0,0),
            _aimDir(0,0)
    {}
    
    void BlobControllerComponent::update(const core::time_state &time) {
        InputComponent::update(time);

        _motionDir.x = _motionDir.y = 0;
        _aimDir.x = _aimDir.y = 0;

        //
        //  Handle motion direction
        //

        if (isKeyDown(app::KeyEvent::KEY_a)) {
            _motionDir.x += -1;
        }

        if (isKeyDown(app::KeyEvent::KEY_d)) {
            _motionDir.x += +1;
        }
        
        if (isKeyDown(app::KeyEvent::KEY_w)) {
            _motionDir.y += 1;
        }

        if (isKeyDown(app::KeyEvent::KEY_s)) {
            _motionDir.y -= 1;
        }
        
        
        //
        //  Handle aiming direction
        //
        
        if (isKeyDown(app::KeyEvent::KEY_LEFT)) {
            _aimDir.x -= 1;
        }

        if (isKeyDown(app::KeyEvent::KEY_RIGHT)) {
            _aimDir.x += 1;
        }

        if (isKeyDown(app::KeyEvent::KEY_UP)) {
            _aimDir.y += 1;
        }
        
        if (isKeyDown(app::KeyEvent::KEY_DOWN)) {
            _aimDir.y -= 1;
        }

        //
        //  Query gamepad
        //

        if (_gamepad) {
            _motionDir += _gamepad->getLeftStick();
            _motionDir += _gamepad->getDPad();
            _aimDir += _gamepad->getRightStick();
        }
        
        // sanitize

        if (lengthSquared(_motionDir) > 0) {
            _motionDir = normalize(_motionDir);
        }
        
        if (lengthSquared(_aimDir) > 0) {
            _aimDir = normalize(_aimDir);
        }
    }
    
#pragma mark - Blob

    BlobRef Blob::create(string name, config c, GamepadRef gamepad) {
        
        // load the default components of the Blob
        auto physics = make_shared<BlobPhysicsComponent>(c.physics);
        auto input = make_shared<BlobControllerComponent>(gamepad);
        auto health = make_shared<HealthComponent>(c.health);
        
        BlobParticleSystemDrawComponent::config psdc;
        gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
        if (!c.particle) {
            auto image = loadImage(app::loadAsset("kessler/textures/blob_particle.png"));
            psdc.textureAtlas = gl::Texture2d::create(image, fmt);
        } else {
            psdc.textureAtlas = c.particle;
        }
        
        if (!c.tonemap) {
            auto image = loadImage(app::loadAsset("kessler/textures/blob_compositor_tonemap.png"));
            c.tonemap = gl::Texture2d::create(image, fmt);
        }
        
        if (!c.tentacleTexture) {
            auto image = loadImage(app::loadAsset("kessler/textures/tentacle.png"));
            c.tentacleTexture = gl::Texture2d::create(image, gl::Texture2d::Format().wrap(GL_REPEAT).mipmap());
        }
        
        // build the particle system components - note, since this isn't a
        // classical ParticleSystem we have to do a little more handholding here

        psdc.tonemap = c.tonemap;
        psdc.background = c.background;
        psdc.backgroundRepeat = c.backgroundRepeat;
        psdc.highlightColor = c.highlightColor;
        psdc.atlasType = elements::Atlas::None;
        psdc.drawLayer = DrawLayers::PLAYER;

        auto psSim = make_shared<BlobParticleSimulation>(physics);
        psSim->setParticleCount(c.physics.numParticles);

        auto psDrawer = make_shared<BlobParticleSystemDrawComponent>(psdc);
        psDrawer->setSimulation(psSim);
        
        // build the tentacle drawing component
        BlobTentacleDrawComponent::config btdc;
        btdc.drawLayer = psdc.drawLayer - 1;
        btdc.tentacleColor = c.tentacleColor;
        btdc.tentacleTexture = c.tentacleTexture;
        auto tentacleDrawer = make_shared<BlobTentacleDrawComponent>(btdc);

        // build the Blob
        auto blob = Entity::create<Blob>(name, { physics, input, health, psSim, psDrawer, tentacleDrawer });
        
        blob->_config = c;
        blob->_physics = physics;
        blob->_input = input;
        
        if (c.drawDebugOverlay) {
            BlobDebugDrawComponent::config ddcc;
            ddcc.drawLayer = DrawLayers::PLAYER + 10;
            blob->addComponent(make_shared<BlobDebugDrawComponent>(ddcc));
        }

        return blob;
    }
    
    Blob::Blob(string name):
            Entity(name)
    {
    }
    
    void Blob::update(const core::time_state &time) {
        Entity::update(time);
        _physics->setMotionDirection(_input->getMotionDirection());
        _physics->setAimDirection(_input->getAimDirection());
    }
    
    void Blob::onHealthChanged(double oldHealth, double newHealth) {
        Entity::onHealthChanged(oldHealth,newHealth);
    }
    
    dvec2 Blob::getTrackingPosition() const {
        return _physics->getTrackingPosition();
    }

    dvec2 Blob::getTrackingUp() const {
        return _physics->getTrackingUp();
    }
    
#pragma mark - BlobViewportController

    /*
     BlobRef _blob;
     PlanetRef _planet;
     */
    
    BlobViewportController::BlobViewportController(core::ViewportRef viewport, BlobRef blob, PlanetRef planet):
    elements::ViewportController(viewport),
    _blob(blob),
    _planet(planet)
    {
        setTrackableTarget(blob);
    }
    
    void BlobViewportController::onReady(core::ObjectRef parent, core::StageRef stage) {
        ViewportController::onReady(parent, stage);
    }

    void BlobViewportController::firstUpdate(const core::time_state &time) {
        getViewport()->setLook(getTarget());
    }

    void BlobViewportController::update(const core::time_state &time) {
        auto planet = _planet.lock();
        if (planet) {
            auto blob = _blob.lock();
            if (blob) {
                const dvec2 trackingPosition = blob->getTrackingPosition();
                const double planetRadius = planet->getSurfaceConfig().radius;
                const double distanceFromPlanet = max(length(trackingPosition - planet->getOrigin()) - (0.5*planetRadius), 0.01 * planetRadius);
                const double viewportSize = 0.5 * min(getViewport()->getWidth(), getViewport()->getHeight());
                const double fudge = 1;
                const double scale = fudge * viewportSize / distanceFromPlanet;
                setTargetScale(scale);
            }
        }

        ViewportController::update(time);
    }

}
