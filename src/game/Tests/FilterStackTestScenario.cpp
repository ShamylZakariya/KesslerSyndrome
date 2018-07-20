//
//  FilterStackTestScenario.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 7/18/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "game/Tests/FilterStackTestScenario.hpp"
#include "elements/Components/DevComponents.hpp"
#include "core/util/GlslProgLoader.hpp"

namespace {

    class ImageDrawer : public core::DrawComponent {
    public:
        
        ImageDrawer(gl::Texture2dRef image, dvec2 topLeft):
                DrawComponent(1, VisibilityDetermination::ALWAYS_DRAW),
                _image(image),
                _topLeft(topLeft)
        {}
        
        void draw(const render_state &renderState) override {
            gl::ScopedBlendAlpha sba;
            gl::ScopedColor sc(ColorA(1,1,1,1));
            
            const auto bounds = _image->getBounds();
            const auto dest = Rectf(_topLeft.x, _topLeft.y, _topLeft.x + bounds.getWidth(), _topLeft.y - bounds.getHeight());
            gl::draw(_image, bounds, dest);
        }
        
    private:
        
        gl::Texture2dRef _image;
        dvec2 _topLeft;
        
    };
    
    class SimpleGlslFilter : public Filter {
    public:
        SimpleGlslFilter(const gl::GlslProgRef &shader):
                _shader(shader)
        {}
        
    protected:
        
        virtual void _bindUniforms(const render_state &state, const gl::FboRef &input) {
            _shader->uniform("ColorTex", 0);
        }
        
        void _render( const render_state &state, const gl::FboRef &input ) override {
            if (!_blitter) {
                _blitter = _createBlitter(_shader);
            }
            _bindUniforms(state, input);

            gl::ScopedTextureBind stb(input->getColorTexture(), 0);
            _blitter->draw();
        }
        
    protected:

        gl::GlslProgRef _shader;
        gl::BatchRef _blitter;

    };
    
    class ColorshiftFilter : public SimpleGlslFilter {
    public:
        ColorshiftFilter(ColorA offset = ColorA(0,0,0,0), ColorA multiplier = ColorA(1,1,1,1)):
                SimpleGlslFilter(util::loadGlslAsset("test_filters/colorshift.glsl")),
                _colorOffset(offset),
                _colorMultiplier(multiplier)
        {}
        
        void setColorOffset(ColorA offset) { _colorOffset = offset; }
        ColorA getColorOffset() const { return _colorOffset; }

        void setColorMultiplier(ColorA offset) { _colorMultiplier = offset; }
        ColorA getColorMultiplier() const { return _colorMultiplier; }

    protected:
        
        void _bindUniforms(const render_state &state, const gl::FboRef &input) override {
            SimpleGlslFilter::_bindUniforms(state, input);
            _shader->uniform("Offset", _colorOffset);
            _shader->uniform("Multiplier", _colorMultiplier);
        }
        
    private:
        
        ColorA _colorOffset, _colorMultiplier;
        
    };
    
    class PixelateFilter : public SimpleGlslFilter {
    public:
        PixelateFilter(int pixelSize):
                SimpleGlslFilter(util::loadGlslAsset("test_filters/pixelate.glsl")),
                _pixelSize(pixelSize)
        {}
        
        void setPixelSize(int ps) { _pixelSize = max<int>(ps, 1); }
        int getPixelSize() const { return _pixelSize; }
        
    protected:
        
        void _bindUniforms(const render_state &state, const gl::FboRef &input) override {
            SimpleGlslFilter::_bindUniforms(state, input);
            _shader->uniform("PixelSize", static_cast<float>(_pixelSize));
        }

    protected:
        
        int _pixelSize;

    };
    
}

FilterStackTestScenario::FilterStackTestScenario() {
}

FilterStackTestScenario::~FilterStackTestScenario() {
    
}

void FilterStackTestScenario::setup() {
    setStage(make_shared<Stage>("FilterStack Test"));

    // build background grid
    auto grid = elements::WorldCartesianGridDrawComponent::create(1);
    grid->setGridColor(ColorA(0.2,0.2,0.7,0.5));
    grid->setAxisColor(ColorA(0.2,0.2,0.7,1));
    grid->setAxisIntensity(1);
    grid->setFillColor(ColorA(0.95,0.95,0.96,1));
    getStage()->addObject(Object::with("Grid", { grid }));
    
    auto viewport = getMainViewport<Viewport>();

    // build viewport controller
    auto viewportController = make_shared<elements::ViewportController>(viewport);
    getStage()->addObject(Object::with("ViewportControl", {
        viewportController,
        make_shared<elements::MouseViewportControlComponent>(viewportController)
    }));

    getStage()->addObject(Object::with("Zelda", {
        make_shared<ImageDrawer>(gl::Texture2d::create(loadImage(app::loadAsset("tests/zelda.png"))), dvec2(0,0))
    }));
    
    // add an SVG to stage just so we have something to look at
    auto doc = util::svg::Group::loadSvgDocument(app::loadAsset("svg_tests/eggsac.svg"), 1);
    getStage()->addObject(Object::with("Eggsac", {make_shared<util::svg::SvgDrawComponent>(doc)}));
    
    auto composer = getViewportComposer();
    auto compositor = dynamic_pointer_cast<ViewportCompositor>(composer->getCompositor());
    
    auto colorshiftFilter = make_shared<ColorshiftFilter>(ColorA(0,0.2,0,0), ColorA(0.5,0.6,1.5,1));
    auto pixelateFilter = make_shared<PixelateFilter>(16);
    compositor->getFilterStack()->push({ colorshiftFilter, pixelateFilter });


    // track 'r' for resetting scenario
    getStage()->addObject(Object::with("InputDelegation",{
        elements::KeyboardDelegateComponent::create(0,{ cinder::app::KeyEvent::KEY_RIGHTBRACKET,cinder::app::KeyEvent::KEY_LEFTBRACKET })->onPress([pixelateFilter, colorshiftFilter](int keyCode){
            switch (keyCode) {
                case cinder::app::KeyEvent::KEY_RIGHTBRACKET:
                    pixelateFilter->setPixelSize(pixelateFilter->getPixelSize() + 1);
                    break;
                case cinder::app::KeyEvent::KEY_LEFTBRACKET:
                    pixelateFilter->setPixelSize(pixelateFilter->getPixelSize() - 1);
                    break;
                default: break;
            }
        }),
    }));

}

void FilterStackTestScenario::cleanup() {
    setStage(nullptr);
}

void FilterStackTestScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.2, 0.2));
}

void FilterStackTestScenario::drawScreen(const render_state &state) {
    
    //
    // NOTE: we're in screen space, with coordinate system origin at top left
    //
    
    float fps = App::get()->getAverageFps();
    float sps = App::get()->getAverageSps();
    string info = core::strings::format("%.1f %.1f", fps, sps);
    gl::drawString(info, vec2(10, 10), Color(0.2,0.2,0.7));
    
    auto viewport = getMainViewport<Viewport>();
    Viewport::look look = viewport->getLook();
    double scale = viewport->getScale();
    
    stringstream ss;
    ss << std::setprecision(3) << "world (" << look.world.x << ", " << look.world.y << ") scale: " << scale << " up: ("
    << look.up.x << ", " << look.up.y << ")";
    gl::drawString(ss.str(), vec2(10, 40), Color(0.2,0.2,0.7));
    
}

bool FilterStackTestScenario::keyDown(const app::KeyEvent &event) {
    if (event.getChar() == 'r') {
        reset();
        return true;
    }
    return false;
}

void FilterStackTestScenario::reset() {
    cleanup();
    setup();
}
