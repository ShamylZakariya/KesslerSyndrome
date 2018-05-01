//
//  SvgTestScenario.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/6/17.
//
//

#include "SvgTestScenario.hpp"
#include "App.hpp"

#include "DevComponents.hpp"

SvgTestScenario::SvgTestScenario() {
}

SvgTestScenario::~SvgTestScenario() {

}

void SvgTestScenario::setup() {
    setStage(make_shared<Stage>("Hello Svg"));
    

    getStage()->addObject(Object::with("Grid", { elements::WorldCartesianGridDrawComponent::create(1) }));

    auto viewportController = make_shared<elements::ViewportController>(getMainViewport<Viewport>());
    getStage()->addObject(Object::with("ViewportControl", {
        viewportController,
        make_shared<elements::MouseViewportControlComponent>(viewportController)
    }));

    testSimpleSvgLoad();
    //testSimpleSvgGroupOriginTransforms();
}

void SvgTestScenario::cleanup() {
    setStage(nullptr);
}

void SvgTestScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.2, 0.2));
}

void SvgTestScenario::drawScreen(const render_state &state) {

    //
    // NOTE: we're in screen space, with coordinate system origin at top left
    //

    float fps = App::get()->getAverageFps();
    float sps = App::get()->getAverageSps();
    string info = core::strings::format("%.1f %.1f", fps, sps);
    gl::drawString(info, vec2(10, 10), Color(1, 1, 1));

    auto viewport = getMainViewport<Viewport>();
    Viewport::look look = viewport->getLook();
    double scale = viewport->getScale();

    stringstream ss;
    ss << std::setprecision(3) << "world (" << look.world.x << ", " << look.world.y << ") scale: " << scale << " up: ("
       << look.up.x << ", " << look.up.y << ")";
    gl::drawString(ss.str(), vec2(10, 40), Color(1, 1, 1));

}

bool SvgTestScenario::keyDown(const ci::app::KeyEvent &event) {
    if (event.getChar() == 'r') {
        reset();
        return true;
    } else if (event.getCode() == ci::app::KeyEvent::KEY_SPACE) {

        return true;
    } else if (event.getCode() == ci::app::KeyEvent::KEY_BACKQUOTE) {
        setRenderMode(RenderMode::mode((int(getRenderMode()) + 1) % RenderMode::COUNT));
    }
    return false;
}

void SvgTestScenario::reset() {
    cleanup();
    setup();
}

#pragma mark - Tests

void SvgTestScenario::testSimpleSvgLoad() {
    auto doc = util::svg::Group::loadSvgDocument(app::loadAsset("svg_tests/eggsac.svg"), 1);
    doc->trace();
    getStage()->addObject(Object::with("Hello SVG", {make_shared<util::svg::SvgDrawComponent>(doc)}));
}

void SvgTestScenario::testSimpleSvgGroupOriginTransforms() {
    auto doc = util::svg::Group::loadSvgDocument(app::loadAsset("svg_tests/group_origin_test.svg"), 1);
    auto root = doc->find("Page-1/group_origin_test/root");
    auto green = root->find("green");
    auto green1 = green->find("green-1");
    auto green2 = green->find("green-2");
    auto purple = root->find("purple");
    auto purple1 = purple->find("purple-1");
    auto purple2 = purple->find("purple-2");

    root->setPosition(-root->getDocumentSize() / 2.0);

    class Wiggler : public core::Component {
    public:
        Wiggler(vector<util::svg::GroupRef> elements) :
                _elements(elements) {
        }

        void step(const time_state &time) override {
            double cycle = cos(time.time) * 0.75;
            int dir = 0;
            for (auto group : _elements) {
                group->setAngle(cycle * (dir % 2 == 0 ? 1 : -1));
                dir++;
            }
        }

    private:
        vector<util::svg::GroupRef> _elements;
    };


    auto drawComponent = make_shared<util::svg::SvgDrawComponent>(doc);
    vector<util::svg::GroupRef> elements = {root, green, purple, green1, green2, purple1, purple2};
    auto wiggleComponent = make_shared<Wiggler>(elements);

    auto obj = Object::with("Hello SVG", {drawComponent, wiggleComponent});
    getStage()->addObject(obj);
}
