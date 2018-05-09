//
//  GameStage.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "GameStage.hpp"

#include "DevComponents.hpp"
#include "PlanetGreebling.hpp"


using namespace core;
using namespace elements;

namespace game {

    namespace {

        SMART_PTR(ExplosionForceCalculator);

        class ExplosionForceCalculator : public RadialGravitationCalculator {
        public:
            static ExplosionForceCalculatorRef create(dvec2 origin, double magnitude, double falloffPower, seconds_t lifespanSeconds = 2) {
                return make_shared<ExplosionForceCalculator>(origin, magnitude, falloffPower, lifespanSeconds);
            }

        public:
            ExplosionForceCalculator(dvec2 origin, double magnitude, double falloffPower, seconds_t lifespanSeconds) :
                    RadialGravitationCalculator(GravitationLayers::EXPLOSION, origin, -abs(magnitude), falloffPower),
                    _startSeconds(time_state::now()),
                    _lifespanSeconds(lifespanSeconds),
                    _magnitudeScale(0) {
            }

            void update(const time_state &time) override {
                seconds_t age = time.time - _startSeconds;
                double life = age / _lifespanSeconds;
                if (life <= 1) {
                    _magnitudeScale = sin(life);
                } else {
                    _magnitudeScale = 0;
                    setFinished(true);
                }
            }

        private:

            seconds_t _startSeconds, _lifespanSeconds;
            double _magnitudeScale;

        };
    }


    /*
     BackgroundRef _background;
     PlanetRef _planet;
     PlayerRef _player;
     vector <CloudLayerParticleSystemRef> _cloudLayers;
     core::RadialGravitationCalculatorRef _gravity;
     elements::ParticleEmitterRef _explosionEmitter;
     elements::ParticleEmitterRef _dustEmitter;
     elements::ViewportControllerRef _viewportController;
     
     double _planetRadius;
     */

    GameStage::GameStage() :
            Stage("Unnamed"),
            _planetRadius(0) {
    }

    GameStage::~GameStage() {
    }

    void GameStage::load(DataSourceRef stageXmlData) {
        auto root = XmlTree(stageXmlData);
        auto prefabsNode = root.getChild("prefabs");
        auto stageNode = root.getChild("stage");

        setName(stageNode.getAttribute("name").getValue());
        
        //
        //  Set up viewport trauma effects
        //
        
        _viewportController = make_shared<ViewportController>(getMainViewport());
        _viewportController->getTraumaConfig().shakeTranslation = dvec2(40,40);
        _viewportController->getTraumaConfig().shakeRotation = 10 * M_PI / 180;
        _viewportController->getTraumaConfig().shakeFrequency = 8;
        addObject(Object::with("ViewportController", { _viewportController }));

        //
        //	Load some basic stage properties
        //

        auto spaceNode = util::xml::findElement(stageNode, "space");
        CI_ASSERT_MSG(spaceNode, "Expect a <space> node in <stage> definition");
        applySpaceAttributes(*spaceNode);

        //
        //	Apply gravity
        //

        for (const auto &childNode : stageNode.getChildren()) {
            if (childNode->getTag() == "gravity") {
                buildGravity(*childNode);
            }
        }

        //
        //	Load background
        //

        auto backgroundNode = util::xml::findElement(stageNode, "background");
        CI_ASSERT_MSG(backgroundNode, "Expect <background> node in stage XML");
        loadBackground(*backgroundNode);

        //
        //	Load Planet
        //

        auto planetNode = util::xml::findElement(stageNode, "planet");
        CI_ASSERT_MSG(planetNode, "Expect <planet> node in stage XML");
        loadPlanet(*planetNode);

        //
        //	Load cloud layers
        //

        auto cloudLayerNode = util::xml::findElement(stageNode, "cloudLayer", "id", "background");
        if (cloudLayerNode) {
            _cloudLayers.push_back(loadCloudLayer(*cloudLayerNode, DrawLayers::PLANET - 1));
        }

        cloudLayerNode = util::xml::findElement(stageNode, "cloudLayer", "id", "foreground");
        if (cloudLayerNode) {
            _cloudLayers.push_back(loadCloudLayer(*cloudLayerNode, DrawLayers::EFFECTS));
        }
        
        auto greebleSystemsNode = util::xml::findElement(*planetNode, "greebleSystems");
        if (greebleSystemsNode) {
            for (size_t i = 0;;i++) {
                auto greebleSystemNode = util::xml::XmlMultiTree(*greebleSystemsNode).getChild("greebleSystem",i);
                if (!greebleSystemNode) {
                    break;
                }
                
                loadGreebleSystem(greebleSystemNode);
            }
        }

        buildExplosionParticleSystem();
        buildDustParticleSystem();
        
        //
        //    Load the player
        //
        auto playersNode = util::xml::findElement(stageNode, "players");
        if (playersNode) {
            for (const auto &child : playersNode->getChildren()) {
                loadPlayer(*child);
            }
        }
        
        //
        //  Build development control components
        //

        if (true) {
            
            //
            // build camera controller, dragger, and cutter, with input dispatch indices 0,1,2 meaning CC gets input first
            //
            
            addObject(Object::with("CameraController", {
                make_shared<MouseViewportControlComponent>(_viewportController, 0)
            }));
            
            addObject(Object::with("Dragger", {
                make_shared<MousePickComponent>(ShapeFilters::GRABBABLE, 1),
                make_shared<MousePickDrawComponent>()
            }));
            
            addObject(Object::with("Cutter", {
                make_shared<terrain::MouseCutterComponent>(getPlanet(), 4, 2),
                make_shared<terrain::MouseCutterDrawComponent>()
            }));
            
            //
            // When user hits command-mouse-down explode a bomb
            //
            
            addObject(Object::with("Mouse", MouseDelegateComponent::create(0)->onPress([this](dvec2 screen, dvec2 world, const app::MouseEvent &event) {
                if (event.isMetaDown()) {
                    performExplosion(world);
                    return true;
                }
                return false;
            })));
        }
    }

    void GameStage::onReady() {
        Stage::onReady();
        
        // respond to TERRAIN|TERRAIN contact
        addCollisionPostSolveHandler(CollisionType::TERRAIN, CollisionType::TERRAIN, [this](const Stage::collision_type_pair &ctp, const ObjectRef &a, const ObjectRef &b, cpArbiter *arbiter) {
            this->handleTerrainTerrainContact(arbiter);
        });

    }

    void GameStage::update(const core::time_state &time) {
        Stage::update(time);
    }

    bool GameStage::onCollisionBegin(cpArbiter *arb) {
        return true;
    }

    bool GameStage::onCollisionPreSolve(cpArbiter *arb) {
        return true;
    }

    void GameStage::onCollisionPostSolve(cpArbiter *arb) {
    }

    void GameStage::onCollisionSeparate(cpArbiter *arb) {
    }

    void GameStage::applySpaceAttributes(const XmlTree &spaceNode) {
        if (spaceNode.hasAttribute("damping")) {
            double damping = util::xml::readNumericAttribute<double>(spaceNode, "damping", 0.95);
            damping = clamp(damping, 0.0, 1.0);
            cpSpaceSetDamping(getSpace()->getSpace(), damping);
        }
    }

    void GameStage::buildGravity(const XmlTree &gravityNode) {
        string type = gravityNode.getAttribute("type").getValue();
        if (type == "radial") {
            dvec2 origin = util::xml::readPointAttribute(gravityNode, "origin", dvec2(0, 0));
            double magnitude = util::xml::readNumericAttribute<double>(gravityNode, "strength", 10);
            double falloffPower = util::xml::readNumericAttribute<double>(gravityNode, "falloff_power", 0);
            auto gravity = RadialGravitationCalculator::create(GravitationLayers::GLOBAL, origin, magnitude, falloffPower);
            addGravity(gravity);

            if (gravityNode.getAttribute("primary") == "true") {
                _gravity = gravity;
            }

        } else if (type == "directional") {
            dvec2 dir = util::xml::readPointAttribute(gravityNode, "dir", dvec2(0, 0));
            double magnitude = util::xml::readNumericAttribute<double>(gravityNode, "strength", 10);
            addGravity(DirectionalGravitationCalculator::create(GravitationLayers::GLOBAL, dir, magnitude));
        }
    }

    void GameStage::loadBackground(const XmlTree &backgroundNode) {
        _background = Background::create(backgroundNode);
        addObject(_background);
    }

    void GameStage::loadPlanet(const XmlTree &planetNode) {
        double friction = util::xml::readNumericAttribute<double>(planetNode, "friction", 1);
        double density = util::xml::readNumericAttribute<double>(planetNode, "density", 1);
        double collisionShapeRadius = 0.1;

        const double minDensity = 1e-3;
        density = max(density, minDensity);

        const double minSurfaceArea = 2;
        const Color terrainColor = util::xml::readColorAttribute(planetNode, "color", Color(1, 0, 1));
        const Color coreColor = util::xml::readColorAttribute(planetNode, "coreColor", Color(0, 1, 1));


        const terrain::material terrainMaterial(density, friction, collisionShapeRadius, ShapeFilters::TERRAIN, CollisionType::TERRAIN, minSurfaceArea, terrainColor);
        const terrain::material anchorMaterial(1, friction, collisionShapeRadius, ShapeFilters::ANCHOR, CollisionType::ANCHOR, minSurfaceArea, coreColor);
        auto world = make_shared<terrain::World>(getSpace(), terrainMaterial, anchorMaterial);

        _planet = Planet::create("Planet", world, planetNode, DrawLayers::PLANET);
        addObject(_planet);

        //
        //	Look for a center of mass for gravity
        //

        if (_gravity) {
            _gravity->setCenterOfMass(_planet->getOrigin());
        }
    }
    
    void GameStage::loadPlayer(const XmlTree &playerNode) {
        dvec2 position(0,0);
        dvec2 localUp(1,0);

        const string positionDescription = playerNode.getAttribute("position");
        const bool isRadians = positionDescription.find("radians") != string::npos;
        const bool isDegrees = positionDescription.find("degrees") != string::npos;

        if (isRadians || isDegrees) {
            size_t leftParen = positionDescription.find("(");
            size_t rightParen = positionDescription.find(")");
            if (leftParen == string::npos || rightParen == string::npos) {
                CI_ASSERT_MSG(false, "Missing left or right paren. Expected form of \"radians(NUMBER)\" for <player> position attribute value.");
            }
            string value = positionDescription.substr(leftParen+1, rightParen - (leftParen+1));
            double radians = 0;
            if (isRadians) {
                radians = strtod(value.c_str(), nullptr);
            } else {
                radians = toRadians(strtod(value.c_str(), nullptr));
            }
            
            // perform raycast to find a start position
            dvec2 origin = getPlanet()->getOrigin();
            double raycastDistance = getPlanet()->getSurfaceConfig().radius * 4;
            dvec2 raycastOrigin = origin + (raycastDistance * dvec2(cos(M_PI_2+radians), sin(M_PI_2+radians)));
            
            auto result = querySegment(raycastOrigin, origin, 1, CP_SHAPE_FILTER_ALL);
            if (result) {
                localUp = normalize(raycastOrigin - origin);
                position = result.point;
            } else {
                CI_ASSERT_MSG(false, "Unable to find a start position for player given position");
                return;
            }
        } else {
            CI_ASSERT_MSG(false, "Only radial positioning supported for <player> valid position attribute value is \"radians(...)\"");
            return;
        }
        
        const int idx = util::xml::readNumericAttribute<int>(playerNode, "id", 0);
        const string name = "player_" + str(idx);
        const ci::DataSourceRef playerTemplate = app::loadAsset(playerNode.getAttribute("template").getValue());

        _player = Player::create(name, playerTemplate, position, localUp);
        addObject(_player);
    }

    CloudLayerParticleSystemRef GameStage::loadCloudLayer(const XmlTree &cloudLayer, int drawLayer) {
        CloudLayerParticleSystem::config config = CloudLayerParticleSystem::config::parse(cloudLayer);
        config.drawConfig.drawLayer = drawLayer;
        auto cl = CloudLayerParticleSystem::create(config);
        addObject(cl);
        return cl;
    }
    
    void GameStage::loadGreebleSystem(const util::xml::XmlMultiTree &greebleNode) {
        auto config = GreeblingParticleSystem::config::parse(greebleNode);
        if (config) {
            const auto &attachments = _planet->getAttachments();
            auto pos = attachments.find(config->attachmentBatchId);
            if (pos != attachments.end()) {
                auto greebles = GreeblingParticleSystem::create("greeble_" + str(config->attachmentBatchId), *config, pos->second);
                addObject(greebles);
            }
        }
    }

    void GameStage::buildExplosionParticleSystem() {
        using namespace elements;

        auto image = loadImage(app::loadAsset("kessler/textures/Explosion.png"));
        gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);

        ParticleSystem::config config;
        config.maxParticleCount = 500;
        config.keepSorted = true;
        config.drawConfig.drawLayer = DrawLayers::EFFECTS;
        config.drawConfig.textureAtlas = gl::Texture2d::create(image, fmt);
        config.drawConfig.atlasType = Atlas::TwoByTwo;
        config.kinematicParticleGravitationLayerMask = GravitationLayers::GLOBAL;

        auto ps = ParticleSystem::create("Explosion ParticleSystem", config);
        addObject(ps);

        //
        // build fire, smoke and spark templates
        //

        particle_prototype fire;
        fire.atlasIdx = 0;
        fire.lifespan = 1;
        fire.radius = {0, 32, 16, 0};
        fire.damping = {0, 0, 0.1};
        fire.additivity = 0.5;
        fire.mass = {0};
        fire.initialVelocity = 60;
        fire.gravitationLayerMask = GravitationLayers::GLOBAL;
        fire.color = ColorA(1, 0.8, 0.1, 1);

        particle_prototype smoke;
        smoke.atlasIdx = 0;
        smoke.lifespan = 2;
        smoke.radius = {0, 0, 32, 16, 16, 0};
        smoke.damping = {0, 0, 0, 0, 0, 0.02};
        smoke.additivity = 0;
        smoke.mass = {0};
        smoke.initialVelocity = 60;
        smoke.gravitationLayerMask = GravitationLayers::GLOBAL;
        smoke.color = ColorA(0.9, 0.9, 0.9, 1);

        particle_prototype spark;
        spark.atlasIdx = 1;
        spark.lifespan = 6;
        spark.radius = {0, 16, 0};
        spark.damping = {0.0, 0.02};
        spark.additivity = {1, 0};
        spark.mass = 10;
        spark.orientToVelocity = true;
        spark.initialVelocity = 200;
        spark.minVelocity = 60;
        spark.color = {ColorA(1, 0.5, 0.5, 1), ColorA(0.5, 0.5, 0.5, 0)};
        fire.gravitationLayerMask = GravitationLayers::GLOBAL;
        spark.kinematics = particle_prototype::kinematics_prototype(1, 1, 0.2, ShapeFilters::TERRAIN);

        _explosionEmitter = ps->createEmitter();
        _explosionEmitter->add(fire, ParticleEmitter::Source(2, 1, 0.3), 1);
        _explosionEmitter->add(smoke, ParticleEmitter::Source(2, 1, 0.3), 2);
        _explosionEmitter->add(spark, ParticleEmitter::Source(1, 1, 0.15), 1);
    }
    
    void GameStage::buildDustParticleSystem() {
        using namespace elements;
        
        auto image = loadImage(app::loadAsset("kessler/textures/Explosion.png"));
        gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
        
        ParticleSystem::config config;
        config.maxParticleCount = 4096;
        config.keepSorted = false;
        config.drawConfig.drawLayer = DrawLayers::EFFECTS;
        config.drawConfig.textureAtlas = gl::Texture2d::create(image, fmt);
        config.drawConfig.atlasType = Atlas::TwoByTwo;
        config.kinematicParticleGravitationLayerMask = GravitationLayers::GLOBAL;
        
        auto ps = ParticleSystem::create("Dust ParticleSystem", config);
        addObject(ps);
        
        //
        // build dust template
        //
        
        particle_prototype dust;
        dust.atlasIdx = 0;
        dust.lifespan = 1;
        dust.radius = {0, 1, 4, 3, 2, 1, 0};
        dust.damping = {0};
        dust.additivity = 0;
        dust.mass = {0};
        dust.initialVelocity = 0;
        dust.gravitationLayerMask = GravitationLayers::GLOBAL;
        dust.color = { ColorA(0.9, 0.9, 0.9, 1) };
        
        auto source = ParticleEmitter::Source(10, 1, 0.5);
        
        _dustEmitter = ps->createEmitter();
        _dustEmitter->add(dust, source);
    }

    void GameStage::cullRubble() {
        size_t count = _planet->getWorld()->cullDynamicGroups(128, 0.75);
        CI_LOG_D("Culled " << count << " bits of rubble");
    }

    void GameStage::makeSleepersStatic() {
        size_t count = _planet->getWorld()->makeSleepingDynamicGroupsStatic(5, 0.75);
        CI_LOG_D("Static'd " << count << " bits of sleeping rubble");
    }

    void GameStage::performExplosion(dvec2 world) {
        // create a radial crack and cut the world with it. note, to reduce tiny fragments
        // we set a fairly high min surface area for the cut.

        double minSurfaceAreaThreshold = 64;
        int numSpokes = 7;
        int numRings = 3;
        double radius = 75;
        double thickness = 2;
        double variance = 100;

        auto crack = make_shared<RadialCrackGeometry>(world, numSpokes, numRings, radius, thickness, variance);
        _planet->getWorld()->cut(crack->getPolygon(), crack->getBB(), minSurfaceAreaThreshold);

        // get the closest point on terrain surface and use that to place explosive charge
        if (auto r = queryNearest(world, ShapeFilters::TERRAIN_PROBE)) {

            dvec2 origin = world + radius * normalize(_planet->getOrigin() - r.point);
            auto gravity = ExplosionForceCalculator::create(origin, 4000, 0.5, 0.5);
            addGravity(gravity);

            for (auto &cls : _cloudLayers) {
                cls->getSimulation<CloudLayerParticleSimulation>()->addGravityDisplacement(gravity);
            }
        }

        dvec2 emissionDir = normalize(world - _planet->getOrigin());
        _explosionEmitter->emit(world, emissionDir, 1.0, 140, elements::ParticleEmitter::Sawtooth);
        
        _viewportController->addTrauma(0.5);
    }
    
    void GameStage::handleTerrainTerrainContact(cpArbiter *arbiter) {

        const float emitProbability = 0.025;

        const int count = cpArbiterGetCount(arbiter);
        cpBody *a = nullptr, *b = nullptr;
        cpArbiterGetBodies(arbiter, &a, &b);

        for (int i = 0; i < count; i++) {
            const cpVect contact = cpArbiterGetPointA(arbiter, i);
            const dvec2 velA = v2(cpBodyGetVelocityAtWorldPoint(a, contact));
            const dvec2 velB = v2(cpBodyGetVelocityAtWorldPoint(b, contact));
            const dvec2 slip = velB - velA;
            double slipVel = length(slip);
            if (slipVel > 0.5) {
                int emitCount = ceil(slipVel);
                _dustEmitter->emitBurst(v2(contact), dvec2(0,0), emitCount, emitProbability);
            }
        }
    }

} // namespace game
