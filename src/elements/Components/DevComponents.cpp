//
//  DevComponents.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/14/17.
//
//

#include "elements/Components/DevComponents.hpp"

using namespace core;
namespace elements {
    
#pragma mark - WorldCartesianGridDrawComponent
    
    /*
     gl::TextureRef _texture;
     double _basePeriod;
     double _periodStep;
     gl::GlslProgRef _shader;
     gl::BatchRef _batch;
     */
    
    shared_ptr<WorldCartesianGridDrawComponent> WorldCartesianGridDrawComponent::create(double basePeriod, double periodStep) {
        auto tex = gl::Texture2d::create(loadImage(app::loadAsset("cartesian_grid.png")), gl::Texture2d::Format().mipmap());
        return make_shared<WorldCartesianGridDrawComponent>(tex, basePeriod, periodStep);
    }
    
    WorldCartesianGridDrawComponent::WorldCartesianGridDrawComponent(gl::TextureRef tex, double basePeriod, double periodStep):
    DrawComponent(-999999, VisibilityDetermination::ALWAYS_DRAW),
    _texture(tex),
    _basePeriod(basePeriod),
    _periodStep(periodStep),
    _axisIntensity(0),
    _fillColor(0, 0, 0, 0),
    _gridColor(0.5, 0.5, 0.5, 1),
    _axisColor(1, 0, 0, 1) {
        
        _texture->setWrap(GL_REPEAT, GL_REPEAT);
        _texture->setMagFilter(GL_NEAREST);
        _texture->setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
        
        auto vsh = CI_GLSL(150,
                           uniform
                           mat4 ciModelViewProjection;
                           uniform
                           mat4 ciModelMatrix;
                           
                           in
                           vec4 ciPosition;
                           
                           out
                           vec2 worldPosition;
                           
                           void main(void) {
                               gl_Position = ciModelViewProjection * ciPosition;
                               worldPosition = (ciModelMatrix * ciPosition).xy;
                           }
                           );
        
        auto fsh = CI_GLSL(150,
                           uniform float AlphaPeriod;
                           uniform float AlphaPeriodTexelSize;
                           uniform float AlphaPeriodAlpha;
                           uniform float BetaPeriod;
                           uniform float BetaPeriodTexelSize;
                           uniform float BetaPeriodAlpha;
                           uniform vec4 BackgroundColor;
                           uniform vec4 GridColor;
                           uniform vec4 AxisColor;
                           uniform float AxisIntensity;
                           uniform sampler2D Tex0;
                           
                           in vec2 worldPosition;
                           
                           out vec4 oColor;
                           
                           /*
                            TODO: Fix the haloing in the onAxis determination. It might make sense to switch from
                            attemping to do this in texel space (right now we're using the 0th texel in S & T when
                            world position is at zero. This blurs because of mipmapping. Disabling mip interpolation fixes
                            the issue... but causes shit moire artifacting.
                            The correct fix is likely to be to detect fragment size (think dFdx, dFdy) and light up
                            fragments with axis color when inside N fragments of axis lines.
                            */
                           
                           vec2 getGridForPeriod(float period, float texelSize) {
                               vec2 texCoord = worldPosition / period;
                               vec2 aTexCoord = abs(texCoord);
                               float alpha = texture(Tex0, texCoord).a;
                               
                               // determine if texCoord is on either the x or y axis, "binary" in that value is 0, 1 or 2 for being on both axes
                               // then clamp that to 0->1
                               float onAxis = (1.0 - step(texelSize, aTexCoord.s)) + (1.0 - step(texelSize, aTexCoord.t));
                               onAxis = clamp(onAxis, 0.0, 1.0);
                               
                               alpha = mix(alpha, 1, AxisIntensity * onAxis);
                               
                               return vec2(alpha, onAxis);
                           }
                           
                           void main(void) {
                               vec2 alphaValue = getGridForPeriod(AlphaPeriod, AlphaPeriodTexelSize);
                               vec2 betaValue = getGridForPeriod(BetaPeriod, BetaPeriodTexelSize);
                               
                               float gridAlpha = ((alphaValue.r * AlphaPeriodAlpha) + (betaValue.r * BetaPeriodAlpha));
                               vec4 gridColor = mix(GridColor, AxisColor, betaValue.g);
                               
                               oColor = mix(BackgroundColor, gridColor, gridAlpha);
                           }
                           );
        
        _shader = gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));
        _batch = gl::Batch::create(geom::Rect().rect(Rectf(-1, -1, 1, 1)), _shader);
    }
    
    void WorldCartesianGridDrawComponent::onReady(ObjectRef parent, StageRef stage) {
    }
    
    void WorldCartesianGridDrawComponent::draw(const render_state &state) {
        setupShaderUniforms();
        
        // set up a scale to fill viewport with plane
        cpBB frustum = state.viewport->getFrustum();
        dvec2 centerWorld = v2(cpBBCenter(frustum));
        dvec2 scale = centerWorld - dvec2(frustum.l, frustum.t);
        
        gl::ScopedModelMatrix smm;
        dmat4 modelMatrix = glm::translate(dvec3(centerWorld, 0)) * glm::scale(dvec3(scale, 1));
        gl::multModelMatrix(modelMatrix);
        
        gl::ScopedTextureBind stb(_texture, 0);
        _shader->uniform("Tex0", 0);
        
        _batch->draw();
    }
    
    namespace {
        inline double calcTexelScale(double scale, double period, double textureSize) {
            return period * scale / textureSize;
        }
    }
    
    void WorldCartesianGridDrawComponent::setupShaderUniforms() {
        ViewportRef vp = getStage()->getMainViewport();
        
        // find largest period where rendered texel scale (ratio of one texel to one pixel) is less than or equal to maxTexelScale
        const double textureSize = _texture->getWidth();
        const double viewportScale = vp->getScale();
        const double maxTexelScale = 3;
        const double minTexelScale = 1 / _periodStep;
        
        double period = _basePeriod;
        double texelScale = calcTexelScale(viewportScale, period, textureSize);
        
        while (texelScale < maxTexelScale) {
            period *= _periodStep;
            texelScale = calcTexelScale(viewportScale, period, textureSize);
        }
        
        // step back one since the above loop oversteps
        period /= _periodStep;
        texelScale /= _periodStep;
        
        const double betaPeriod = period;
        const double betaPeriodTexelScale = texelScale;
        const double alphaPeriod = betaPeriod / _periodStep;
        const double alphaPeriodTexelScale = texelScale / _periodStep;
        
        // alpha of beta period falls as it approaches max texel size, and alpha period is the remainder
        const double betaPeriodAlpha = 1 - (betaPeriodTexelScale - minTexelScale) / (maxTexelScale - minTexelScale);
        const double alphaPeriodAlpha = 1 - betaPeriodAlpha;
        
        // compute the mip stage so we can pass texel size [0...1] to the shader.
        // This is used by shader to detect axis edges
        double alphaPeriodMip = max(ceil(log2(1 / alphaPeriodTexelScale)), 0.0);
        double betaPeriodMip = max(ceil(log2(1 / betaPeriodTexelScale)), 0.0);
        
        // note we bump mip up to handle slop
        double alphaPeriodTexelSize = pow(2, alphaPeriodMip + 1) / _texture->getWidth();
        double betaPeriodTexelSize = pow(2, betaPeriodMip + 1) / _texture->getWidth();
        
        //	app::console() << " viewportScale: " << viewportScale
        //		<< "scale: " << dvec2(alphaPeriodTexelScale, betaPeriodTexelScale)
        //		<< " mip: " << dvec2(alphaPeriodMip, betaPeriodMip)
        //		<< " texelSize: " << dvec2(alphaPeriodTexelSize, betaPeriodTexelSize)
        //		<< endl;
        
        _shader->uniform("Tex0", 0);
        _shader->uniform("AlphaPeriod", static_cast<float>(alphaPeriod));
        _shader->uniform("AlphaPeriodTexelSize", static_cast<float>(alphaPeriodTexelSize));
        _shader->uniform("AlphaPeriodAlpha", static_cast<float>(alphaPeriodAlpha));
        _shader->uniform("BetaPeriod", static_cast<float>(betaPeriod));
        _shader->uniform("BetaPeriodTexelSize", static_cast<float>(betaPeriodTexelSize));
        _shader->uniform("BetaPeriodAlpha", static_cast<float>(betaPeriodAlpha));
        _shader->uniform("BackgroundColor", _fillColor);
        _shader->uniform("GridColor", _gridColor);
        _shader->uniform("AxisColor", _axisColor);
        _shader->uniform("AxisIntensity", static_cast<float>(_axisIntensity));
    }
    
#pragma mark - MouseViewportControlComponent
    
    /*
     bool _handleRotation;
     vec2 _mouseScreen, _mouseWorld;
     ViewportControllerRef _viewportController;
     */
    
    MouseViewportControlComponent::MouseViewportControlComponent(ViewportControllerRef viewportController, bool handleRotation, int dispatchReceiptIndex):
            InputComponent(dispatchReceiptIndex),
            _handleRotation(handleRotation),
            _viewportController(viewportController)
    {
    }
    
    void MouseViewportControlComponent::step(const time_state &time) {
        if (_handleRotation) {
            const bool fast = (isKeyDown(app::KeyEvent::KEY_LSHIFT)||isKeyDown(app::KeyEvent::KEY_RSHIFT));
            const double radsPerSec = (fast ? 100 : 10) * M_PI / 180;
            if (isKeyDown(app::KeyEvent::KEY_q)) {
                Viewport::look target = _viewportController->getTarget();
                target.up = glm::rotate(target.up, -radsPerSec * time.deltaT);
                _viewportController->setTarget(target);
            } else if (isKeyDown(app::KeyEvent::KEY_e)) {
                Viewport::look target = _viewportController->getTarget();
                target.up = glm::rotate(target.up, +radsPerSec * time.deltaT);
                _viewportController->setTarget(target);
            }
        }
    }
    
    bool MouseViewportControlComponent::onMouseDown(const app::MouseEvent &event) {
        _mouseScreen = event.getPos();
        _mouseWorld = getStage()->getMainViewport()->screenToWorld(_mouseScreen);
        
        //CI_LOG_D("screen: " << _mouseScreen << " world: " << _mouseWorld << " SPACE: " << isKeyDown(app::KeyEvent::KEY_SPACE) << " ALT: " << event.isAltDown());
        
        // capture alt key for re-centering
        if (event.isAltDown()) {
            _viewportController->setTarget(_mouseWorld);
            return true;
        }
        
        // capture space key for dragging
        if (isKeyDown(app::KeyEvent::KEY_SPACE)) {
            return true;
        }
        
        return false;
    }
    
    bool MouseViewportControlComponent::onMouseUp(const app::MouseEvent &event) {
        return false;
    }
    
    bool MouseViewportControlComponent::onMouseMove(const app::MouseEvent &event, const ivec2 &delta) {
        _mouseScreen = event.getPos();
        _mouseWorld = getStage()->getMainViewport()->screenToWorld(_mouseScreen);
        return false;
    }
    
    bool MouseViewportControlComponent::onMouseDrag(const app::MouseEvent &event, const ivec2 &delta) {
        _mouseScreen = event.getPos();
        _mouseWorld = getStage()->getMainViewport()->screenToWorld(_mouseScreen);
        
        if (isKeyDown(app::KeyEvent::KEY_SPACE)) {
            dvec2 deltaWorld = _viewportController->getViewport()->screenToWorldDir(dvec2(delta));
            Viewport::look target = _viewportController->getTarget();
            target.world -= deltaWorld;
            _viewportController->setTarget(target);
            return true;
        }
        
        return false;
    }
    
    bool MouseViewportControlComponent::onMouseWheel(const app::MouseEvent &event) {
        const float zoom = _viewportController->getTarget().scale,
        wheelScale = 0.1 * zoom,
        dz = (event.getWheelIncrement() * wheelScale);
        
        _viewportController->scaleAboutScreenPoint(zoom + dz, dvec2(event.getX(), event.getY()));
        
        return true;
    }
    
#pragma mark - KeyboardViewportControlComponent
    /*
     elements::ViewportControllerRef _viewportController;
     dvec2 _panRate;
     double _fastScalar;
     double _rotateRate;
     double _scaleRate;
     */
    KeyboardViewportControlComponent::KeyboardViewportControlComponent(elements::ViewportControllerRef viewportController, int dispatchReceiptIndex):
    InputComponent(dispatchReceiptIndex),
    _viewportController(viewportController),
    _panRate(10),
    _fastScalar(10),
    _rotateRate(10 * M_PI / 180),
    _scaleRate(1)
    {
    }
    
    
    // InputComponent
    void KeyboardViewportControlComponent::step(const core::time_state &time) {
        using namespace app;
        
        const bool fast = (isKeyDown(app::KeyEvent::KEY_LSHIFT)||isKeyDown(app::KeyEvent::KEY_RSHIFT));
        const double scale = (fast ? _fastScalar : 1) * time.deltaT;
        
        Viewport::look target = _viewportController->getTarget();
        
        if (isKeyDown(KeyEvent::KEY_RIGHTBRACKET)) {
            target.scale += _scaleRate * scale;
        }
        
        if (isKeyDown(KeyEvent::KEY_LEFTBRACKET)) {
            target.scale -= _scaleRate * scale;
        }
        
        if (isKeyDown(KeyEvent::KEY_q)) {
            target.up = rotate(target.up, -_rotateRate * scale);
        }
        
        if (isKeyDown(KeyEvent::KEY_e)) {
            target.up = rotate(target.up, +_rotateRate * scale);
        }
        
        if (isKeyDown(KeyEvent::KEY_w)) {
            target.world.y += (_panRate.y * scale) / target.scale;
        }
        
        if (isKeyDown(KeyEvent::KEY_s)) {
            target.world.y -= (_panRate.y * scale) / target.scale;
        }
        
        if (isKeyDown(KeyEvent::KEY_d)) {
            target.world.x += (_panRate.x * scale) / target.scale;
        }
        
        if (isKeyDown(KeyEvent::KEY_a)) {
            target.world.x -= (_panRate.x * scale) / target.scale;
        }
        
        _viewportController->setTarget(target);
    }
        
#pragma mark - MousePickComponent
    
    MousePickComponent::MousePickComponent(cpShapeFilter pickFilter, int dispatchReceiptIndex) :
    InputComponent(dispatchReceiptIndex),
    _pickFilter(pickFilter),
    _mouseBody(nullptr),
    _draggingBody(nullptr),
    _mouseJoint(nullptr) {
    }
    
    MousePickComponent::~MousePickComponent() {
        releaseDragConstraint();
        cpBodyFree(_mouseBody);
    }
    
    void MousePickComponent::onReady(ObjectRef parent, StageRef stage) {
        InputComponent::onReady(parent, stage);
        _mouseBody = cpBodyNewKinematic();
    }
    
    void MousePickComponent::step(const time_state &time) {
        cpVect mouseBodyPos = cpv(_mouseWorld);
        cpVect newMouseBodyPos = cpvlerp(cpBodyGetPosition(_mouseBody), mouseBodyPos, 0.75);
        
        cpBodySetVelocity(_mouseBody, cpvmult(cpvsub(newMouseBodyPos, cpBodyGetPosition(_mouseBody)), time.deltaT));
        cpBodySetPosition(_mouseBody, newMouseBodyPos);
    }
    
    bool MousePickComponent::onMouseDown(const app::MouseEvent &event) {
        releaseDragConstraint();
        
        _mouseScreen = event.getPos();
        _mouseWorld = getStage()->getMainViewport()->screenToWorld(_mouseScreen);
        
        if (isKeyDown(app::KeyEvent::KEY_SPACE)) {
            return false;
        }
        
        const float distance = 1.f;
        cpPointQueryInfo info = {};
        cpShape *pick = cpSpacePointQueryNearest(getStage()->getSpace()->getSpace(), cpv(_mouseWorld), distance, _pickFilter, &info);
        if (pick) {
            cpBody *pickBody = cpShapeGetBody(pick);
            
            if (pickBody && cpBodyGetType(pickBody) == CP_BODY_TYPE_DYNAMIC) {
                cpVect nearest = (info.distance > 0.0f ? info.point : cpv(_mouseWorld));
                
                _draggingBody = pickBody;
                _mouseJoint = cpPivotJointNew2(_mouseBody, _draggingBody, cpvzero, cpBodyWorldToLocal(_draggingBody, nearest));
                
                getStage()->getSpace()->addConstraint(_mouseJoint);
                
                return true;
            }
        }
        
        return false;
    }
    
    bool MousePickComponent::onMouseUp(const app::MouseEvent &event) {
        releaseDragConstraint();
        return false;
    }
    
    bool MousePickComponent::onMouseMove(const app::MouseEvent &event, const ivec2 &delta) {
        _mouseScreen = event.getPos();
        _mouseWorld = getStage()->getMainViewport()->screenToWorld(_mouseScreen);
        return false;
    }
    
    bool MousePickComponent::onMouseDrag(const app::MouseEvent &event, const ivec2 &delta) {
        _mouseScreen = event.getPos();
        _mouseWorld = getStage()->getMainViewport()->screenToWorld(_mouseScreen);
        return false;
    }
    
    void MousePickComponent::releaseDragConstraint() {
        if (_mouseJoint) {
            cpCleanupAndFree(_mouseJoint);
            _draggingBody = nullptr;
        }
    }
    
#pragma mark - MousePickDrawComponent
    
    MousePickDrawComponent::MousePickDrawComponent(ColorA color, float radius) :
    DrawComponent(numeric_limits<int>::max(), VisibilityDetermination::ALWAYS_DRAW),
    _color(color),
    _radius(radius) {
    }
    
    void MousePickDrawComponent::onReady(ObjectRef parent, StageRef stage) {
        DrawComponent::onReady(parent, stage);
        _pickComponent = getSibling<MousePickComponent>();
    }
    
    void MousePickDrawComponent::draw(const render_state &renderState) {
        if (shared_ptr<MousePickComponent> pc = _pickComponent.lock()) {
            if (pc->isDragging()) {
                const float radius = _radius * renderState.viewport->getReciprocalScale();
                gl::color(_color);
                gl::drawSolidCircle(pc->getDragPositionWorld(), radius);
            }
        }
    }
    
#pragma mark - KeyboardDelegateComponent
    
    KeyboardDelegateComponentRef KeyboardDelegateComponent::create(int dispatchReceiptIndex) {
        return make_shared<KeyboardDelegateComponent>(dispatchReceiptIndex);
    }
    
    KeyboardDelegateComponent::KeyboardDelegateComponent(int dispatchReceiptIndex):
    InputComponent(dispatchReceiptIndex) {
    }
    
    KeyboardDelegateComponentRef KeyboardDelegateComponent::onPress(KeyHandler h) {
        _downHandler = h;
        return shared_from_this_as<KeyboardDelegateComponent>();
    }
    
    KeyboardDelegateComponentRef KeyboardDelegateComponent::onRelease(KeyHandler h) {
        _upHandler = h;
        return shared_from_this_as<KeyboardDelegateComponent>();
    }
    
    bool KeyboardDelegateComponent::onKeyDown(const app::KeyEvent &event) {
        if (_downHandler) {
            return _downHandler(event.getCode());
        }
        return false;
    }
    
    bool KeyboardDelegateComponent::onKeyUp(const app::KeyEvent &event) {
        if (_upHandler) {
            return _upHandler(event.getCode());
        }
        return false;
    }
    
#pragma mark - MouseDelegateComponent
    
    /*
     MousePressHandler _pressHandler;
     MouseReleaseHandler _releaseHandler;
     MouseMoveHandler _moveHandler;
     MouseDragHandler _dragHandler;
     */
    
    MouseDelegateComponentRef MouseDelegateComponent::create(int dispatchReceiptIndex) {
        return make_shared<MouseDelegateComponent>(dispatchReceiptIndex);
    }
    
    MouseDelegateComponent::MouseDelegateComponent(int dispatchReceiptIndex) :
    InputComponent(dispatchReceiptIndex) {
    }
    
    MouseDelegateComponentRef MouseDelegateComponent::onPress(MousePressHandler h) {
        _pressHandler = h;
        return shared_from_this_as<MouseDelegateComponent>();
    }
    
    MouseDelegateComponentRef MouseDelegateComponent::onRelease(MouseReleaseHandler h) {
        _releaseHandler = h;
        return shared_from_this_as<MouseDelegateComponent>();
    }
    
    MouseDelegateComponentRef MouseDelegateComponent::onMove(MouseMoveHandler h) {
        _moveHandler = h;
        return shared_from_this_as<MouseDelegateComponent>();
    }
    
    MouseDelegateComponentRef MouseDelegateComponent::onDrag(MouseDragHandler h) {
        _dragHandler = h;
        return shared_from_this_as<MouseDelegateComponent>();
    }
    
    MouseDelegateComponentRef MouseDelegateComponent::onWheel(MouseWheelHandler h) {
        _wheelHandler = h;
        return shared_from_this_as<MouseDelegateComponent>();
    }
    
    bool MouseDelegateComponent::onMouseDown(const app::MouseEvent &event) {
        if (_pressHandler) {
            const dvec2 screen = event.getPos();
            const dvec2 world = getStage()->getMainViewport()->screenToWorld(screen);
            
            return _pressHandler(screen, world, event);
        }
        return false;
    }
    
    bool MouseDelegateComponent::onMouseUp(const app::MouseEvent &event) {
        if (_releaseHandler) {
            const dvec2 screen = event.getPos();
            const dvec2 world = getStage()->getMainViewport()->screenToWorld(screen);
            
            return _releaseHandler(screen, world, event);
        }
        return false;
    }
    
    bool MouseDelegateComponent::onMouseMove(const app::MouseEvent &event, const ivec2 &delta) {
        if (_moveHandler) {
            const dvec2 screen = event.getPos();
            const dvec2 world = getStage()->getMainViewport()->screenToWorld(screen);
            const dvec2 worldDelta = getStage()->getMainViewport()->screenToWorldDir(delta);
            
            return _moveHandler(screen, world, delta, worldDelta, event);
        }
        
        return false;
    }
    
    bool MouseDelegateComponent::onMouseDrag(const app::MouseEvent &event, const ivec2 &delta) {
        if (_dragHandler) {
            const dvec2 screen = event.getPos();
            const dvec2 world = getStage()->getMainViewport()->screenToWorld(screen);
            const dvec2 worldDelta = getStage()->getMainViewport()->screenToWorldDir(delta);
            
            return _dragHandler(screen, world, delta, worldDelta, event);
        }
        
        return false;
    }
    
    bool MouseDelegateComponent::onMouseWheel(const app::MouseEvent &event) {
        if (_wheelHandler) {
            const dvec2 screen = event.getPos();
            const dvec2 world = getStage()->getMainViewport()->screenToWorld(screen);
            const double deltaWheel = event.getWheelIncrement();
            return _wheelHandler(screen, world, deltaWheel, event);
        }
        
        return false;
    }
    
#pragma mark - SvgAttachmentAdapter
    
    SvgAttachmentAdapter::SvgAttachmentAdapter(terrain::AttachmentRef attachment, util::svg::GroupRef svgDoc):
    AttachmentAdapter(attachment),
    _svgDoc(svgDoc),
    _lastPosition(0,0),
    _lastAngle(0),
    _friction(0.025)
    {}
    
    void SvgAttachmentAdapter::update(const time_state &timeState) {
        AttachmentAdapter::update(timeState);
        if (isOrphaned()) {
            double fade = 1 - _friction;
            _svgDoc->setPosition(_svgDoc->getPosition() + _linearVel * timeState.deltaT);
            _svgDoc->setAngle(_svgDoc->getAngle() + _angularVel * timeState.deltaT);
            _svgDoc->setScale(_svgDoc->getScale() * fade);
            _svgDoc->setOpacity(_svgDoc->getOpacity() * fade);
            
            _linearVel *= fade;
            _angularVel *= fade;
            
            if (_svgDoc->getOpacity() < ALPHA_EPSILON) {
                getObject()->setFinished();
            }
        }
    }
    
    void SvgAttachmentAdapter::updatePosition(const time_state &timeState, dvec2 position, dvec2 rotation, dmat4 transform) {
        _lastPosition = _svgDoc->getPosition();
        _lastAngle = _svgDoc->getAngle();
        
        _svgDoc->setPosition(position);
        _svgDoc->setRotation(rotation);
        
        _linearVel = (position - _lastPosition) / timeState.deltaT;
        _angularVel = (_svgDoc->getAngle() - _lastAngle) / timeState.deltaT;
    }
    
#pragma mark - PerformanceDrawComponent
    
    void PerformanceDisplayComponent::drawScreen(const render_state &renderState) {
        stringstream ss;
        ss << setprecision(2)
           << "fps: " << core::App::get()->getAverageFps()
           << " sps: " << core::App::get()->getAverageSps();
        
        gl::ScopedBlendAlpha sba;
        gl::drawString(ss.str(), _topLeft, _color);
    }
    
} // end namespace elements

