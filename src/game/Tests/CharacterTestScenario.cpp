//
//  CharacterTestScenario.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 8/6/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "CharacterTestScenario.hpp"

#include <cinder/Rand.h>

#include "game/KesslerSyndrome/GameConstants.hpp"
#include "game/KesslerSyndrome/entities/blob/Blob.hpp"
#include "elements/Components/DevComponents.hpp"

using namespace core;
using namespace elements;

namespace {
    
    const double COLLISION_SHAPE_RADIUS = 0;
    const double MIN_SURFACE_AREA = 2;
    const Color TERRAIN_COLOR(0.2, 0.2, 0.2);
    const Color ANCHOR_COLOR(1, 0, 1);
    
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
        enum layer {
            TERRAIN = 1,
        };
    }
    
}

/*
 terrain::TerrainObjectRef _terrain;
 */

CharacterTestScenario::CharacterTestScenario() {
}

CharacterTestScenario::~CharacterTestScenario() {
}

void CharacterTestScenario::setup() {
    auto stage = make_shared<Stage>("Character Test Scenario");
    setStage(stage);

    stage->addGravity(DirectionalGravitationCalculator::create(game::GravitationLayers::GLOBAL, dvec2(0, -1), 9.8 * 10));

    auto terrain = terrain::TerrainObject::create("Terrain", loadLevelSvg(), DrawLayers::TERRAIN);
    stage->addObject(terrain);
    
    stage->addObject(Object::with("Dragger", {
        make_shared<MousePickComponent>(ShapeFilters::GRABBABLE),
        make_shared<MousePickDrawComponent>()
    }));
    
    stage->addObject(Object::with("Cutter", {
        make_shared<terrain::MouseCutterComponent>(terrain, 4),
        make_shared<terrain::MouseCutterDrawComponent>()
    }));

    // build background grid
    auto grid = elements::WorldCartesianGridDrawComponent::create(1);
    grid->setGridColor(ColorA(0.2,0.2,0.7,0.5));
    grid->setAxisColor(ColorA(0.2,0.2,0.7,1));
    grid->setAxisIntensity(1);
    grid->setFillColor(ColorA(0.95,0.95,0.96,1));
    stage->addObject(Object::with("Grid", { grid }));
    
    // build a blob character
    game::Blob::config blobConfig;
    blobConfig.drawDebugOverlay = false;
    blobConfig.physics.friction = 0.5;
    blobConfig.physics.position = dvec2(512,256);

    auto image = loadImage(app::loadAsset("kessler/textures/starfield_0_squared.jpg"));
    auto backgroundFormat = gl::Texture2d::Format().mipmap(false).wrap(GL_REPEAT);
    blobConfig.background = gl::Texture2d::create(image, backgroundFormat);
    blobConfig.backgroundRepeat = 1;

    auto gamepad = InputDispatcher::get()->getGamepads().empty() ? nullptr : InputDispatcher::get()->getGamepads().front();
    auto blob = game::Blob::create("Blob", blobConfig, gamepad);
    
    auto vc = make_shared<game::BlobViewportController>(stage->getMainViewport(), blob, nullptr);
    blob->addComponent(vc);
    
    stage->addObject(blob);
    
    stage->addObject(Object::with("InputDelegation",{
        elements::KeyboardDelegateComponent::create(0)->onPress([&](int keyCode)->bool{
            switch(keyCode) {
                    // track 'r' for resetting scenario
                case app::KeyEvent::KEY_r:
                    this->reset();
                    return true;
                default:
                    return false;
            }
        })
    }));
    
}

void CharacterTestScenario::cleanup() {
    setStage(nullptr);
}

void CharacterTestScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.225, 0.25));
}

void CharacterTestScenario::drawScreen(const render_state &state) {
    // draw fpf/sps
    float fps = App::get()->getAverageFps();
    float sps = App::get()->getAverageSps();
    string info = strings::format("%.1f %.1f", fps, sps);
    gl::drawString(info, vec2(10, 10), Color(1, 1, 1));
}

void CharacterTestScenario::reset() {
    cleanup();
    setup();
}


terrain::WorldRef CharacterTestScenario::loadLevelSvg() {
    
    // load shapes and anchors from SVG
    vector<terrain::ShapeRef> shapes;
    vector<terrain::AnchorRef> anchors;
    vector<terrain::ElementRef> elements;
    terrain::World::loadSvg(app::loadAsset("tests/blob_world.svg"), dmat4(), shapes, anchors, elements, true);
    
    // partition
    auto partitionedShapes = terrain::World::partition(shapes, 500);
    
    // construct
    const terrain::material terrainMaterial(1, 3, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    const terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(partitionedShapes, anchors, elements);
    
    return world;
}

