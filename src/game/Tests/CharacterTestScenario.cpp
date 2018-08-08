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
    
    game::Blob::config create_protoplasmic_blob_config() {
        game::Blob::config c;
        return c;
    }
    
    game::Blob::config create_amorphous_blob_config() {
        game::Blob::config c;
        c.physics.type = game::BlobPhysicsComponent::FluidType::AMORPHOUS;
        return c;
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

    stage->addGravity(DirectionalGravitationCalculator::create(game::GravitationLayers::GLOBAL, dvec2(0, -1), 9.8));

    
    auto terrain = terrain::TerrainObject::create("Terrain", loadLevelSvg(), DrawLayers::TERRAIN);
    stage->addObject(terrain);
    
    _viewportController = make_shared<ViewportController>(getMainViewport<Viewport>());
    stage->addObject(Object::with("ViewportControl", {
        _viewportController,
        make_shared<MouseViewportControlComponent>(_viewportController)
    }));
    
    // build background grid
    auto grid = elements::WorldCartesianGridDrawComponent::create(1);
    grid->setGridColor(ColorA(0.2,0.2,0.7,0.5));
    grid->setAxisColor(ColorA(0.2,0.2,0.7,1));
    grid->setAxisIntensity(1);
    grid->setFillColor(ColorA(0.95,0.95,0.96,1));
    stage->addObject(Object::with("Grid", { grid }));
    
    // build a blob character
    const bool amorphous = true;
    game::Blob::config blobConfig = amorphous ? create_amorphous_blob_config() : create_protoplasmic_blob_config();
    blobConfig.physics.position = dvec2(512,256);
    auto gamepad = InputDispatcher::get()->getGamepads().empty() ? nullptr : InputDispatcher::get()->getGamepads().front();
    auto blob = game::Blob::create("Blob", blobConfig, gamepad);
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
    const terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    const terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(partitionedShapes, anchors, elements);
    
    return world;
}

