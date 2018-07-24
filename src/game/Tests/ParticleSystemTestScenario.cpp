//
//  ParticleSystemTestScenario.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/2/17.
//

#include "game/Tests/ParticleSystemTestScenario.hpp"
#include "elements/Components/DevComponents.hpp"

#include "game/KesslerSyndrome/elements/CloudLayerParticleSystem.hpp"
#include "core/filters/Filters.hpp"

using namespace core;
using namespace elements;

namespace {

    const double CollisionShapeRadius = 0;
    const double MinSurfaceArea = 2;
    const Color TerrainColor(0.3, 0.3, 0.3);
    const Color AnchorColor(0, 0, 0);

    namespace CollisionType {

        /*
         The high 16 bits are a mask, the low are a type_id, the actual type is the logical OR of the two.
         */

        namespace is {
            enum bits {
                SHOOTABLE = 1 << 16,
                TOWABLE = 1 << 17
            };
        }

        enum type_id {
            TERRAIN = 1 | is::SHOOTABLE | is::TOWABLE,
            ANCHOR = 2 | is::SHOOTABLE,
        };
    }

    namespace ShapeFilters {

        enum Categories {
            _TERRAIN = 1 << 0,
            _GRABBABLE = 1 << 1,
            _ANCHOR = 1 << 2,
        };

        cpShapeFilter TERRAIN = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN, _TERRAIN | _GRABBABLE | _ANCHOR);
        cpShapeFilter ANCHOR = cpShapeFilterNew(CP_NO_GROUP, _ANCHOR, _TERRAIN);
        cpShapeFilter GRABBABLE = cpShapeFilterNew(CP_NO_GROUP, _GRABBABLE, _TERRAIN);
    }

    namespace DrawLayers {
        enum Layer {
            TERRAIN = 1,
        };
    }

    namespace GravitationLayers {
        enum Layer {
            GLOBAL = 1 << 0
        };
    }

    const terrain::material TerrainMaterial(1, 0.5, CollisionShapeRadius, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MinSurfaceArea, TerrainColor);
    const terrain::material AnchorMaterial(1, 1, CollisionShapeRadius, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MinSurfaceArea, AnchorColor);


    PolyLine2d rect(float left, float bottom, float right, float top) {
        PolyLine2d pl;
        pl.push_back(vec2(left, bottom));
        pl.push_back(vec2(right, bottom));
        pl.push_back(vec2(right, top));
        pl.push_back(vec2(left, top));
        pl.setClosed();
        return pl;
    }
}

/*
 ParticleSystemRef _explosionPs;
 particle_prototype smoke, spark, rubble;
 ParticleEmitterRef _explosionEmitter;
 ParticleEmitter::emission_id _explostionEmissionId;
 */


ParticleSystemTestScenario::ParticleSystemTestScenario() :
        _explosionEmissionId(0) {
}

ParticleSystemTestScenario::~ParticleSystemTestScenario() {
}

void ParticleSystemTestScenario::setup() {
    setStage(make_shared<Stage>("Particle System Tests"));

    _viewportController = make_shared<ViewportController>(getMainViewport<Viewport>());
    _viewportController->setTrackingConfig(ViewportController::tracking_config(0.99,0.99,1));
    
    auto trauma = ViewportController::trauma_config();
    trauma.shakeTranslation = dvec2(30,30);
    trauma.shakeRotation = 5 * M_PI / 180;
    trauma.traumaDecayRate = 1;
    trauma.shakeFrequency = 8;
    _viewportController->setTraumaConfig(trauma);

    
    auto keyboardViewportController = make_shared<KeyboardViewportControlComponent>(_viewportController);
    keyboardViewportController->setPanRate(50);

    getStage()->addObject(Object::with("ViewportControl", {
        _viewportController,
        make_shared<elements::MouseViewportControlComponent>(_viewportController),
        keyboardViewportController,
    }));

    auto grid = WorldCartesianGridDrawComponent::create(1);
    grid->setFillColor(ColorA(0.2, 0.22, 0.25, 1.0));
    grid->setGridColor(ColorA(1, 1, 1, 0.1));
    getStage()->addObject(Object::with("Grid", {grid}));

    //
    //	Build some dynamic geometry
    //

    vector <terrain::ShapeRef> shapes = {
            terrain::Shape::fromContour(rect(-100, 0, 100, 20)),
            terrain::Shape::fromContour(rect(-100, 20, -80, 100)),
            terrain::Shape::fromContour(rect(80, 20, 100, 100)),
    };

    vector <terrain::AnchorRef> anchors = {
            terrain::Anchor::fromContour(rect(-5, 5, 5, 15))
    };

    auto world = make_shared<terrain::World>(getStage()->getSpace(), TerrainMaterial, AnchorMaterial);
    world->build(shapes, anchors);

    auto terrain = terrain::TerrainObject::create("Terrain", world, DrawLayers::TERRAIN);
    getStage()->addObject(terrain);


    //
    //	Build particle systems
    //

    buildCloudLayerPs();
    buildExplosionPs();


    auto mdc = MouseDelegateComponent::create(10)
            ->onPress([this](dvec2 screen, dvec2 world, const app::MouseEvent &event) {
                if (event.isMetaDown()) {
                    _explosionEmitter->emit(world, dvec2(0, 1), 1, 120, ParticleEmitter::Sawtooth);
                    _viewportController->addTrauma(0.75);
                } else {
                    _explosionEmissionId = _explosionEmitter->emit(world, dvec2(0, 1), 120);
                }
                return false;
            })
            ->onRelease([this](dvec2 screen, dvec2 world, const app::MouseEvent &event) {
                _explosionEmitter->cancel(_explosionEmissionId);
                _explosionEmissionId = 0;
                return false;
            })
            ->onMove([this](dvec2 screen, dvec2 world, dvec2 deltaScreen, dvec2 deltaWorld, const app::MouseEvent &event) {
                return false;
            })
            ->onDrag([this](dvec2 screen, dvec2 world, dvec2 deltaScreen, dvec2 deltaWorld, const app::MouseEvent &event) {
                dvec2 dir = normalize(deltaWorld);
                _explosionEmitter->setEmissionPosition(_explosionEmissionId, world, dir);
                return false;
            });

    getStage()->addObject(Object::with("Mouse Handling", {mdc}));
}

void ParticleSystemTestScenario::cleanup() {
    setStage(nullptr);
}

void ParticleSystemTestScenario::update(const time_state &time) {
    if (_explosionEmissionId != 0) {
        _viewportController->setTraumaBaseline(0.25);
    }
}

void ParticleSystemTestScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.2, 0.2));
}

void ParticleSystemTestScenario::drawScreen(const render_state &state) {

    //
    // NOTE: we're in screen space, with coordinate system origin at top left
    //

    float fps = App::get()->getAverageFps();
    float sps = App::get()->getAverageSps();
    string info = core::strings::format("perf: %.1f %.1f", fps, sps);
    gl::drawString(info, vec2(10, 10), Color(1, 1, 1));

    auto vp = getMainViewport<Viewport>();
    Viewport::look look = vp->getLook();
    double scale = vp->getScale();

    stringstream ss;
    ss << std::setprecision(3) << "world (" << look.world.x << ", " << look.world.y << ") scale: " << scale << " up: ("
    << look.up.x << ", " << look.up.y << ") trauma: " << _viewportController->getCurrentTraumaLevel();
    gl::drawString(ss.str(), vec2(10, 40), Color(1, 1, 1));
}

bool ParticleSystemTestScenario::keyDown(const app::KeyEvent &event) {
    if (event.getChar() == 'r') {
        reset();
        return true;
    }
    return false;
}

void ParticleSystemTestScenario::reset() {
    cleanup();
    setup();
}

#pragma mark - Tests

void ParticleSystemTestScenario::buildCloudLayerPs() {
    auto cloudLayerXmlAsset = app::loadAsset("tests/cloud_layer_ps.xml");
    auto cloudLayer = XmlTree(cloudLayerXmlAsset).getChild("cloudLayer");
    
    game::CloudLayerParticleSystem::config config = game::CloudLayerParticleSystem::config::parse(cloudLayer);
    config.drawConfig.drawLayer = 0;
    
    auto cl = game::CloudLayerParticleSystem::create(config);
    getStage()->addObject(cl);

    // now add a filter stack; note we need to set a clear color or else we get previous frame data because PS doesn't fill the buffer with its output
    auto clearColor = ColorA(config.simulationConfig.particle.color, 0);
    auto dc = cl->getComponent<ParticleSystemDrawComponent>();
    auto fs = make_shared<FilterStack>();
    dc->setFilterStack(fs, clearColor);

    auto pixelateFilter = make_shared<filters::PixelateFilter>(16);
    pixelateFilter->setClearsColorBuffer(true);
    pixelateFilter->setClearColor(clearColor);
    fs->push(pixelateFilter);

}

void ParticleSystemTestScenario::buildExplosionPs() {
    auto image = loadImage(app::loadAsset("kessler/textures/Explosion.png"));
    gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);

    ParticleSystem::config config;
    config.maxParticleCount = 500;
    config.keepSorted = true;
    config.drawConfig.drawLayer = 1000;
    config.drawConfig.textureAtlas = gl::Texture2d::create(image, fmt);
    config.drawConfig.atlasType = Atlas::TwoByTwo;

    _explosionPs = ParticleSystem::create("_explosionPs", config);

    auto stage = getStage();
    stage->addObject(_explosionPs);
    stage->addGravity(DirectionalGravitationCalculator::create(GravitationLayers::GLOBAL, dvec2(0, -1), 9.8 * 10));

    //
    // build fire, smoke and spark templates
    //

    particle_prototype fire;
    fire.atlasIdx = 0;
    fire.lifespan = 1;
    fire.radius = {0, 8, 4, 0};
    fire.damping = {0, 0, 0.1};
    fire.additivity = 0.5;
    fire.mass = {0};
    fire.initialVelocity = 15;
    fire.gravitationLayerMask = GravitationLayers::GLOBAL;
    fire.color = ColorA(1, 0.8, 0.1, 1);

    particle_prototype smoke;
    smoke.atlasIdx = 0;
    smoke.lifespan = 2;
    smoke.radius = {0, 0, 8, 4, 4, 0};
    smoke.damping = {0, 0, 0, 0, 0, 0.02};
    smoke.additivity = 0;
    smoke.mass = {0};
    smoke.initialVelocity = 15;
    smoke.gravitationLayerMask = GravitationLayers::GLOBAL;
    smoke.color = ColorA(0.9, 0.9, 0.9, 1);

    particle_prototype spark;
    spark.atlasIdx = 1;
    spark.lifespan = 2;
    spark.radius = {0, 4, 0};
    spark.damping = {0.0, 0.02};
    spark.additivity = {1, 0};
    spark.mass = 10;
    spark.orientToVelocity = true;
    spark.initialVelocity = 100;
    spark.minVelocity = 15;
    spark.color = ColorA(1, 0.75, 0.5, 1);
    spark.kinematics = particle_prototype::kinematics_prototype(1, 1, 0.2, ShapeFilters::TERRAIN);

    // build a "rubble" particle template
    particle_prototype rubble;
    rubble.atlasIdx = 2;
    rubble.lifespan = 10;
    rubble.radius = {0.1, 2.0, 2.0, 2.0, 0.1};
    rubble.damping = 0.02;
    rubble.additivity = 0.0;
    rubble.mass = 10.0;
    rubble.color = TerrainColor;
    rubble.initialVelocity = 30;
    rubble.kinematics = particle_prototype::kinematics_prototype(1, 0, ShapeFilters::TERRAIN);

    _explosionEmitter = _explosionPs->createEmitter();
    _explosionEmitter->add(fire, ParticleEmitter::Source(2, 1, 0.3), 4);
    _explosionEmitter->add(smoke, ParticleEmitter::Source(2, 1, 0.3), 8);
    _explosionEmitter->add(spark, ParticleEmitter::Source(1, 1, 0.15), 4);
    _explosionEmitter->add(rubble, ParticleEmitter::Source(10, 1, 0.15), 2);
}
