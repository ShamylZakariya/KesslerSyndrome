//
//  TerrainTestScenario.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 3/5/17.
//
//

#include <cinder/Rand.h>

#include "game/Tests/TerrainTestScenario.hpp"
#include "core/util/SpatialIndex.hpp"
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

    PolyLine2d rect(float left, float bottom, float right, float top) {
        PolyLine2d pl;
        pl.push_back(vec2(left, bottom));
        pl.push_back(vec2(right, bottom));
        pl.push_back(vec2(right, top));
        pl.push_back(vec2(left, top));
        pl.setClosed();
        return pl;
    }

    PolyLine2d rect(vec2 center, vec2 size) {
        return rect(center.x - size.x / 2, center.y - size.y / 2, center.x + size.x / 2, center.y + size.y / 2);
    }

    terrain::ShapeRef boxShape(vec2 center, vec2 size) {
        return terrain::Shape::fromContour(rect(center.x - size.x / 2, center.y - size.y / 2, center.x + size.x / 2, center.y + size.y / 2));
    }

    struct cut {
        vec2 a, b;
        float radius;

        cut(vec2 A, vec2 B, float R) :
                a(A),
                b(B),
                radius(R) {
        }

        cut(string desc) {
            desc = strings::removeSet(desc, "\t[]toradius:,");

            const char *it = desc.c_str();
            char *end = nullptr;
            int idx = 0;
            for (float f = strtod(it, &end);; f = strtod(it, &end)) {
                if (it == end) {
                    break;
                }

                switch (idx) {
                    case 0:
                        a.x = f;
                        break;
                    case 1:
                        a.y = f;
                        break;
                    case 2:
                        b.x = f;
                        break;
                    case 3:
                        b.y = f;
                        break;
                    case 4:
                        radius = f;
                        break;
                }

                it = end;
                idx++;
            }
        }

        static vector<cut> parse(string cutsDesc) {
            auto cutDescs = strings::split(cutsDesc, "\n");
            vector<cut> cuts;
            for (auto cutDesc : cutDescs) {
                cutDesc = strings::strip(cutDesc);
                if (!cutDesc.empty()) {
                    cuts.emplace_back(cutDesc);
                }
            }
            return cuts;
        }

    };

}

/*
 terrain::TerrainObjectRef _terrain;
 */

TerrainTestScenario::TerrainTestScenario() {
}

TerrainTestScenario::~TerrainTestScenario() {
}

void TerrainTestScenario::setup() {
    setRenderStateGizmoMask(Gizmos::LABELS | Gizmos::ANCHORS);
    
    setStage(make_shared<Stage>("Terrain Test Stage"));

    //auto world = testDistantTerrain();
    //auto world = testBasicTerrain();
    //auto world = testComplexTerrain();
    //auto world = testSimpleAnchors();
    //auto world = testComplexAnchors();
    //auto world = testSimplePartitionedTerrain();
    //auto world = testComplexPartitionedTerrainWithAnchors();
    //auto world = testFail();
    //auto world = testSimpleSvgLoad();
    auto world = testComplexSvgLoad();

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

    _viewportController = make_shared<ViewportController>(getMainViewport<Viewport>());
    auto cameraController = Object::with("ViewportControl", {
        _viewportController,
        make_shared<MouseViewportControlComponent>(_viewportController)
    });

    auto gridDc = WorldCartesianGridDrawComponent::create();
    gridDc->setFillColor(ColorA(0.6,0.6,0.6,1));
    gridDc->setGridColor(ColorA(0.9,0.9,0.9,1));
    auto grid = Object::with("Grid", {gridDc});

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

void TerrainTestScenario::cleanup() {
    _terrain.reset();
    setStage(nullptr);
}

void TerrainTestScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.225, 0.25));
}

void TerrainTestScenario::drawScreen(const render_state &state) {
    // draw fpf/sps
    float fps = App::get()->getAverageFps();
    float sps = App::get()->getAverageSps();
    string info = strings::format("%.1f %.1f", fps, sps);
    gl::drawString(info, vec2(10, 10), Color(1, 1, 1));
}

void TerrainTestScenario::reset() {
    cleanup();
    setup();
}

terrain::WorldRef TerrainTestScenario::testDistantTerrain() {


    const vec2 origin(30000, 30000);

    _viewportController->setTarget(origin);


    vector<terrain::ShapeRef> shapes = {terrain::Shape::fromContour(rect(origin.x - 200, origin.y - 200, origin.x + 200, origin.y + 200))};
    shapes = terrain::World::partition(shapes, 30);

    const terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);

    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(shapes);

    return world;
}

terrain::WorldRef TerrainTestScenario::testBasicTerrain() {

    cpSpaceSetDamping(getStage()->getSpace()->getSpace(), 0.5);

    _viewportController->setTarget(vec2(0, 0));

    vector<terrain::ShapeRef> shapes = {
            terrain::Shape::fromContour(rect(0, 0, 100, 50)),        // 0
            terrain::Shape::fromContour(rect(100, 0, 150, 50)),        // 1 to right of 0 - binds to 0
            terrain::Shape::fromContour(rect(-100, 0, 0, 50)),        // 2 to left of 0 - binds to 0
            terrain::Shape::fromContour(rect(-10, 50, 110, 100)),    // 3 above 0, but wider
            terrain::Shape::fromContour(rect(-10, 100, 110, 200)), // 4 above 3, binds to 3

            //terrain::Shape::fromContour(rect(200,0,210,0)),		// empty rect - should be garbage collected
    };

    const terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);

    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(shapes);
    return world;
}

terrain::WorldRef TerrainTestScenario::testComplexTerrain() {
    _viewportController->setTarget(vec2(0, 0));

    const vec2 boxSize(50, 50);
    auto boxPos = [boxSize](float x, float y) -> vec2 {
        return vec2(x * boxSize.x + boxSize.x / 2, y * boxSize.y + boxSize.y / 2);
    };

    vector<terrain::ShapeRef> shapes = {
            boxShape(boxPos(0, 0), boxSize),
            boxShape(boxPos(1, 0), boxSize),
            boxShape(boxPos(2, 0), boxSize),
            boxShape(boxPos(3, 0), boxSize),
            boxShape(boxPos(3, 1), boxSize),
            boxShape(boxPos(3, 2), boxSize),
            boxShape(boxPos(3, 3), boxSize),
            boxShape(boxPos(2, 3), boxSize),
            boxShape(boxPos(1, 3), boxSize),
            boxShape(boxPos(0, 3), boxSize),
            boxShape(boxPos(0, 2), boxSize),
            boxShape(boxPos(0, 1), boxSize),
    };

    terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);

    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(shapes);

    return world;
}

terrain::WorldRef TerrainTestScenario::testSimpleAnchors() {
    _viewportController->setTarget(vec2(0, 0));


    vector<terrain::ShapeRef> shapes = {
            terrain::Shape::fromContour(rect(0, 0, 100, 50)),        // 0
            terrain::Shape::fromContour(rect(100, 0, 150, 50)),        // 1 to right of 0 - binds to 0
            terrain::Shape::fromContour(rect(-100, 0, 0, 50)),        // 2 to left of 0 - binds to 0
    };

    vector<terrain::AnchorRef> anchors = {
            terrain::Anchor::fromContour(rect(40, 20, 60, 30)),
            terrain::Anchor::fromContour(rect(200, 20, 260, 30))
    };

    const terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    const terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);

    world->build(shapes, anchors);
    return world;
}

terrain::WorldRef TerrainTestScenario::testComplexAnchors() {
    _viewportController->setTarget(vec2(0, 0));

    const vec2 boxSize(50, 50);
    auto boxPos = [boxSize](float x, float y) -> vec2 {
        return vec2(x * boxSize.x + boxSize.x / 2, y * boxSize.y + boxSize.y / 2);
    };

    vector<terrain::ShapeRef> shapes = {
            boxShape(boxPos(0, 0), boxSize),
            boxShape(boxPos(1, 0), boxSize),
            boxShape(boxPos(2, 0), boxSize),
            boxShape(boxPos(3, 0), boxSize),
            boxShape(boxPos(3, 1), boxSize),
            boxShape(boxPos(3, 2), boxSize),
            boxShape(boxPos(3, 3), boxSize),
            boxShape(boxPos(2, 3), boxSize),
            boxShape(boxPos(1, 3), boxSize),
            boxShape(boxPos(0, 3), boxSize),
            boxShape(boxPos(0, 2), boxSize),
            boxShape(boxPos(0, 1), boxSize),

            boxShape(boxSize * 2.f, boxSize)
    };


    vector<terrain::AnchorRef> anchors = {
            terrain::Anchor::fromContour(rect(vec2(25, 25), vec2(10, 10)))
    };

    const terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    const terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);

    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(shapes, anchors);

    return world;
}

terrain::WorldRef TerrainTestScenario::testSimplePartitionedTerrain() {

    Rand rng;

    auto ring = [&rng](vec2 center, float radius, int subdivisions, float wobbleRange) -> PolyLine2d {
        PolyLine2d polyLine;
        for (int i = 0; i < subdivisions; i++) {
            double j = static_cast<double>(i) / static_cast<double>(subdivisions);
            double r = j * M_PI * 2;
            vec2 p = center + (vec2(cos(r), sin(r)) * (radius + rng.nextFloat(-wobbleRange, +wobbleRange)));
            polyLine.push_back(p);
        }
        polyLine.setClosed();
        return polyLine;
    };

    _viewportController->setTarget(vec2(0, 0));

    auto rings = vector<PolyLine2d> {
            ring(vec2(0, 0), 500, 600, 0),
            ring(vec2(0, 0), 400, 600, 0)
    };

    auto shapes = terrain::Shape::fromContours(rings);
    //auto shapes = vector<terrain::ShapeRef> { boxShape(vec2(0,0), vec2(500,500)) };

    auto partitionedShapes = terrain::World::partition(shapes, 130);

    const auto terrainMaterial = terrain::material(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);

    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(partitionedShapes);

    return world;
}

terrain::WorldRef TerrainTestScenario::testComplexPartitionedTerrainWithAnchors() {

    Rand rng;

    auto ring = [&rng](vec2 center, float radius, int subdivisions, float wobbleRange) -> PolyLine2d {
        PolyLine2d polyLine;
        for (int i = 0; i < subdivisions; i++) {
            double j = static_cast<double>(i) / static_cast<double>(subdivisions);
            double r = j * M_PI * 2;
            vec2 p = center + (vec2(cos(r), sin(r)) * (radius + rng.nextFloat(-wobbleRange, +wobbleRange)));
            polyLine.push_back(p);
        }
        polyLine.setClosed();
        return polyLine;
    };

    _viewportController->setTarget(vec2(0, 0));

    auto rings = vector<PolyLine2d> {
            ring(vec2(0, 0), 500, 600, 0),
            ring(vec2(0, 0), 400, 600, 0)
    };

    auto shapes = terrain::Shape::fromContours(rings);
    auto partitionedShapes = terrain::World::partition(shapes, 130);


    vector<terrain::AnchorRef> anchors = {
            terrain::Anchor::fromContour(rect(vec2(0, -450), vec2(10, 10)))
    };

    const terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    const terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(partitionedShapes, anchors);

    return world;
}

terrain::WorldRef TerrainTestScenario::testSimpleSvgLoad() {

    // load shapes and anchors from SVG
    vector<terrain::ShapeRef> shapes;
    vector<terrain::AnchorRef> anchors;
    vector<terrain::ElementRef> elements;
    terrain::World::loadSvg(app::loadAsset("svg_tests/world_anchor_test.svg"), dmat4(), shapes, anchors, elements, true);

    // partition
    //auto partitionedShapes = terrain::World::partition(shapes, dvec2(0,0), 50);

    // construct
    const terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    const terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(shapes, anchors, elements);

    return world;
}

terrain::WorldRef TerrainTestScenario::testComplexSvgLoad() {

    // load shapes and anchors from SVG
    vector<terrain::ShapeRef> shapes;
    vector<terrain::AnchorRef> anchors;
    vector<terrain::ElementRef> elements;
    terrain::World::loadSvg(app::loadAsset("svg_tests/complex_world_test.svg"), dmat4(), shapes, anchors, elements, true);

    // partition
    auto partitionedShapes = terrain::World::partition(shapes, 500);

    // construct
    const terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    const terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(partitionedShapes, anchors, elements);

    return world;

}

terrain::WorldRef TerrainTestScenario::testFail() {
    Rand rng;

    auto ring = [&rng](dvec2 center, float radius, int subdivisions, float wobbleRange) -> PolyLine2d {
        PolyLine2d polyLine;
        for (int i = 0; i < subdivisions; i++) {
            double j = static_cast<double>(i) / static_cast<double>(subdivisions);
            double r = j * M_PI * 2;
            dvec2 p = center + (dvec2(cos(r), sin(r)) * static_cast<double>(radius + rng.nextFloat(-wobbleRange, +wobbleRange)));
            polyLine.push_back(p);
        }
        polyLine.setClosed();
        return polyLine;
    };

    _viewportController->setTarget(vec2(0, 0));

    auto rings = vector<PolyLine2d> {
            ring(vec2(0, 0), 500, 600, 0),
            ring(vec2(0, 0), 400, 600, 0)
    };

    auto shapes = terrain::Shape::fromContours(rings);
    auto partitionedShapes = terrain::World::partition(shapes, 130);


    vector<terrain::AnchorRef> anchors = {
            terrain::Anchor::fromContour(rect(vec2(0, -450), vec2(10, 10)))
    };

    const terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    const terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);

    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(partitionedShapes, anchors);


    // this sequence of cuts caused trouble
    string cutDescriptions = R"(
	[ -155.994,   84.944] to [ -483.285,  336.033] radius: 2.49663
	[ -269.808,  242.250] to [ -417.349,  337.285] radius: 1.04977
	[ -282.409,  262.727] to [ -387.420,  333.609] radius: 1.04977
	[ -273.484,  267.452] to [ -384.795,  360.387] radius: 1.04977
	[ -258.257,  279.004] to [ -382.695,  379.289] radius: 1.04977
	[ -238.839,  283.203] to [ -354.876,  391.365] radius: 1.04977
	[ -242.515,  270.077] to [ -318.648,  404.491] radius: 1.04977
	[ -244.090,  291.604] to [ -319.173,  461.722] radius: 1.04977
	[ -229.388,  295.805] to [ -280.319,  426.018] radius: 1.04977
	[ -219.937,  313.656] to [ -261.417,  435.994] radius: 1.04977
	[ -217.312,  325.208] to [ -256.166,  439.145] radius: 1.04977
	[ -209.961,  324.158] to [ -250.916,  442.295] radius: 1.04977
	[ -204.710,  325.727] to [ -245.139,  445.965] radius: 1.04977
	[ -201.462,  334.242] to [ -238.086,  443.261] radius: 0.852035
	[ -197.629,  340.204] to [ -232.550,  456.889] radius: 0.852035
	[ -189.538,  335.093] to [ -221.903,  460.722] radius: 0.852035
	[ -333.052,  208.613] to [ -432.703,  278.028] radius: 0.852035
	[ -339.440,  203.503] to [ -448.034,  272.492] radius: 0.852035
	)";

    auto cuts = cut::parse(cutDescriptions);

    // we know the last cut causes weirdness so cut all but the last
    for (int i = 0; i < cuts.size() - 1; i++) {
        world->cut(cuts[i].a, cuts[i].b, cuts[i].radius);
    }

    cut lastCut = cuts[cuts.size() - 1];
    world->cut(lastCut.a, lastCut.b, lastCut.radius);


    //	// this leaves shape 31 improperly cut - there's a tiny tab on lower left which should be excised but isn't
    //	world->cut(vec2(-339.440,  203.503), vec2(-448.034,  272.492), 0.852035, ShapeFilters::TERRAIN_PROBE);

    return world;
}

void TerrainTestScenario::timeSpatialIndex() {

    Rand rng;
    using util::SpatialIndex;

    auto generator = [&rng](cpBB bounds, int count) -> vector<cpBB> {
        vector<cpBB> out;

        const float sizeMin = (bounds.r - bounds.l) / 50;
        const float sizeMax = sizeMin * 2;

        for (int i = 0; i < count; i++) {
            vec2 origin = vec2(rng.nextFloat(bounds.l, bounds.r), rng.nextFloat(bounds.b, bounds.t));
            float size = rng.nextFloat(sizeMin, sizeMax);

            cpBB bb = cpBBNew(origin.x - size / 2, origin.y - size / 2, origin.x + size / 2, origin.y + size / 2);
            out.push_back(bb);
        }

        return out;
    };

    auto timeUsingSpatialIndex = [](const vector<cpBB> bbs, SpatialIndex<int> &index) -> double {
        StopWatch timer;
        index.clear();

        int tick = 0;
        for (auto bb : bbs) {
            index.insert(bb, tick++);
        }

        int count = 0;

        for (auto queryBB : bbs) {
            auto neighbors = index.sweep(queryBB);
            count += neighbors.size();
        }

        return timer.mark();
    };

    auto timeUsingBruteForce = [](const vector<cpBB> bbs) -> double {
        StopWatch timer;

        int count = 0;
        const auto last = bbs.end();
        for (auto it = bbs.begin(); it != last; ++it) {
            for (auto it2 = bbs.begin(); it2 != last; ++it2) {
                if (it != it2 && cpBBIntersects(*it, *it2)) {
                    count++;
                }
            }
        }

        double m = timer.mark();
        app::console() << "timeUsingBruteForce count: " << count << std::endl;
        return m;
    };

    auto performTimingRun = [&generator, &timeUsingSpatialIndex, &timeUsingBruteForce](int count) {
        const cpBB bounds = cpBBNew(-1000, -1000, 1000, 10000);
        vector<cpBB> bbs = generator(bounds, count);
        SpatialIndex<int> index(50);
        const int runs = 10;

        double sumSpatialIndex = 0;
        double sumBruteForce = 0;

        for (int i = 0; i < runs; i++) {
            sumSpatialIndex += timeUsingSpatialIndex(bbs, index);
            sumBruteForce += timeUsingBruteForce(bbs);
        }

        const double spatialIndexAverageTime = sumSpatialIndex / runs;
        const double bruteForceAverageTime = sumBruteForce / runs;

        app::console() << "For " << runs << " runs over " << count << " items:" << endl;
        app::console() << "\tspatialIndex average time: " << spatialIndexAverageTime << " seconds" << endl;
        app::console() << "\tbruteForce average time: " << bruteForceAverageTime << " seconds" << endl;
        app::console() << "\tratio spatialIndex/bruteForce: " << (spatialIndexAverageTime / bruteForceAverageTime)
                       << endl;
        app::console() << endl << endl;
    };

    app::console() << "------------------------------------" << endl << "PERFORMING PERF MEASUREMENTS" << endl;
    performTimingRun(450);
}
