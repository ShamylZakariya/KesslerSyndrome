//
//  TerrainAttachmentsTestScenario.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 2/8/18.
//

#include <cinder/Rand.h>

#include "game/Tests/TerrainAttachmentsTestScenario.hpp"
#include "elements/Components/DevComponents.hpp"
#include "core/util/Svg.hpp"

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
            ATTACHMENTS = 2
        };
    }
    
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
 terrain::TerrainObjectRef _terrain;
 */

TerrainAttachmentsTestScenario::TerrainAttachmentsTestScenario() {
}

TerrainAttachmentsTestScenario::~TerrainAttachmentsTestScenario() {
}

void TerrainAttachmentsTestScenario::setup() {
    // go debug for rendering
    setRenderStateGizmoMask(Gizmos::AABBS | Gizmos::LABELS | Gizmos::LABELS);
    
    setStage(make_shared<Stage>("Terrain Test Stage"));
    
//    auto world = testBasicTerrain();
    auto world = testBasicAttachmentAdapter();
//    auto world = testComplexSvgLoad();
    
    _terrain = terrain::TerrainObject::create("Terrain", world, DrawLayers::TERRAIN);
    getStage()->addObject(_terrain);
    
    auto dragger = Object::with("Dragger", {
        make_shared<MousePickComponent>(ShapeFilters::GRABBABLE),
        make_shared<MousePickDrawComponent>()
    });
    
    auto cutter = Object::with("Cutter", {
        make_shared<terrain::MouseCutterComponent>(_terrain, 4),
        make_shared<terrain::MouseCutterDrawComponent>()
    });
    
    auto viewportController = make_shared<ViewportController>(getMainViewport<Viewport>());
    auto cameraController = Object::with("ViewportControlComponent", {
        viewportController,
        make_shared<MouseViewportControlComponent>(viewportController)
    });
    
    auto gridDc = WorldCartesianGridDrawComponent::create();
    gridDc->setFillColor(ColorA(0.30,0.30,0.30,1));
    gridDc->setGridColor(ColorA(0.50,0.50,0.50,1));
    gridDc->setAxisColor(ColorA(0.50,0.90,0.90,1));
    gridDc->setAxisIntensity(0);
    auto grid = Object::with("Grid", { gridDc });
    
    getStage()->addObject(dragger);
    getStage()->addObject(cutter);
    getStage()->addObject(grid);
    getStage()->addObject(cameraController);
    
    getStage()->addObject(Object::with("InputDelegation",{
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

void TerrainAttachmentsTestScenario::cleanup() {
    _terrain.reset();
    setStage(nullptr);
}

void TerrainAttachmentsTestScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.225, 0.25));
}

void TerrainAttachmentsTestScenario::drawScreen(const render_state &state) {
    // draw fpf/sps
    float fps = App::get()->getAverageFps();
    float sps = App::get()->getAverageSps();
    string info = strings::format("%.1f %.1f", fps, sps);
    gl::drawString(info, vec2(10, 10), Color(1, 1, 1));
}

void TerrainAttachmentsTestScenario::reset() {
    cleanup();
    setup();
}

terrain::WorldRef TerrainAttachmentsTestScenario::testBasicTerrain() {
    using namespace terrain;
    cpSpaceSetDamping(getStage()->getSpace()->getSpace(), 0.5);
    
    vector<ShapeRef> shapes = {
        Shape::fromContour(rect(0, 0, 100, 50)),          // 0
        Shape::fromContour(rect(100, 0, 150, 50)),        // 1 to right of 0 - binds to 0
        Shape::fromContour(rect(-100, 0, 0, 50)),         // 2 to left of 0 - binds to 0
        Shape::fromContour(rect(-10, 50, 110, 100)),      // 3 above 0, but wider
        Shape::fromContour(rect(-10, 100, 110, 200)),     // 4 above 3, binds to 3
    };
    
    vector<AnchorRef> anchors = {
        Anchor::fromContour(rect(20, 120, 30, 130))       // 5 inside 4, anchors 4 and 3
    };
    
    const material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    
    auto world = make_shared<World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(shapes, anchors);
    
    // make some attachments
    auto makeAttachment = [&world](dvec2 position, bool shouldBeDynamic) -> bool {
        AttachmentRef attachment = make_shared<Attachment>();
        if (world->addAttachment(attachment, position)) {
            GroupBaseRef group = attachment->getGroup();
            CI_ASSERT_MSG(group->isDynamic() == shouldBeDynamic, (shouldBeDynamic ? "Group added to should be dynamic" : "Group added to should have been static"));
            CI_LOG_D("Added attachment @ " << position << " to group: " << group << " position: " << group->getPosition());
            CI_LOG_D("\tAttachment world: " << attachment->getWorldPosition() << " local: " << attachment->getLocalPosition());
            return true;
        }
        return false;
    };
    
    CI_ASSERT_MSG(makeAttachment(dvec2(5,5), true), "Should have been successfully added");
    CI_ASSERT_MSG(makeAttachment(dvec2(5,55), false), "Should have been successfully added");
    CI_ASSERT_MSG(makeAttachment(dvec2(-100,-100), false) == false, "Should have NOT been added");
    
    return world;
}

terrain::WorldRef TerrainAttachmentsTestScenario::testBasicAttachmentAdapter() {
    using namespace terrain;
    cpSpaceSetDamping(getStage()->getSpace()->getSpace(), 0.5);
        
    vector<ShapeRef> shapes = {
        Shape::fromContour(rect(0, 0, 100, 50)),          // 0
        Shape::fromContour(rect(100, 0, 150, 50)),        // 1 to right of 0 - binds to 0
        Shape::fromContour(rect(-100, 0, 0, 50)),         // 2 to left of 0 - binds to 0
        Shape::fromContour(rect(-10, 50, 110, 100)),      // 3 above 0, but wider
        Shape::fromContour(rect(-10, 100, 110, 200)),     // 4 above 3, binds to 3
    };
    
    vector<AnchorRef> anchors = {
        Anchor::fromContour(rect(20, 120, 30, 130))       // 5 inside 4, anchors 4 and 3
    };
    
    const material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    
    auto world = make_shared<World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(shapes, anchors);
    
    // make some attachments
    
    auto makeDingus = [&](dvec2 position) {
        AttachmentRef attachment = make_shared<Attachment>();
        if (world->addAttachment(attachment, position)) {
            auto svg = util::svg::Group::loadSvgDocument(app::loadAsset("svg_tests/dingus.svg"), 1);
            getStage()->addObject(Object::with("Dingus", {
                make_shared<util::svg::SvgDrawComponent>(svg, DrawLayers::ATTACHMENTS),
                make_shared<SvgAttachmentAdapter>(attachment, svg)
            }));
        }
    };

    makeDingus(dvec2(5,5));
    makeDingus(dvec2(25,5));
    makeDingus(dvec2(45,5));
    makeDingus(dvec2(5,55));

    return world;
}

