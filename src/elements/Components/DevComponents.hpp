//
//  DevComponents.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/14/17.
//
//

#ifndef DevComponents_hpp
#define DevComponents_hpp

#include "Core.hpp"
#include "Terrain.hpp"
#include "Svg.hpp"

SMART_PTR(WorldCartesianGridDrawComponent);

class WorldCartesianGridDrawComponent : public core::DrawComponent {
public:

    static shared_ptr<WorldCartesianGridDrawComponent> create(double basePeriod = 10, double periodStep = 10);

public:

    WorldCartesianGridDrawComponent(gl::TextureRef tex, double basePeriod, double periodStep);

    void onReady(core::ObjectRef parent, core::StageRef stage) override;

    cpBB getBB() const override {
        return cpBBInfinity;
    }

    void draw(const core::render_state &state) override;

    core::VisibilityDetermination::style getVisibilityDetermination() const override {
        return core::VisibilityDetermination::ALWAYS_DRAW;
    }

    int getLayer() const override {
        return -1;
    }

    // set color of background fill
    void setFillColor(ci::ColorA fillColor) {
        _fillColor = fillColor;
    }

    ci::ColorA getFillColor() const {
        return _fillColor;
    }

    // set the color of the grid
    void setGridColor(ci::ColorA color) {
        _gridColor = color;
    }

    ci::ColorA getGridColor() const {
        return _gridColor;
    }
    
    // set the color of the grid, where it overlaps with x==0 || y== 0
    void setAxisColor(ci::ColorA axisColor) {
        _axisColor = axisColor;
    }
    
    ci::ColorA getAxisColor() const {
        return _axisColor;
    }
    
    // set the "intensity", e.g boldness/thickness of the grid where it overlaps with x==0 || y==0.
    // default value of 0 causes axis lines to be drawn same as any other grid line. Value of 1 is very bold.
    void setAxisIntensity(double intensity) {
        _axisIntensity = saturate<float>(intensity);
    }
    
    double getAxisIntensity() const {
        return _axisIntensity;
    }

protected:

    void setupShaderUniforms();

private:

    gl::TextureRef _texture;
    double _basePeriod;
    double _periodStep;
    double _axisIntensity;
    gl::GlslProgRef _shader;
    gl::BatchRef _batch;
    ci::ColorA _fillColor, _gridColor, _axisColor;

};

SMART_PTR(MouseViewportControlComponent);

class MouseViewportControlComponent : public core::InputComponent {
public:

    MouseViewportControlComponent(core::ViewportControllerRef viewportController, int dispatchReceiptIndex = 1000);

    // InputComponent
    void step(const core::time_state &time) override;

    bool mouseDown(const ci::app::MouseEvent &event) override;

    bool mouseUp(const ci::app::MouseEvent &event) override;

    bool mouseMove(const ci::app::MouseEvent &event, const ivec2 &delta) override;

    bool mouseDrag(const ci::app::MouseEvent &event, const ivec2 &delta) override;

    bool mouseWheel(const ci::app::MouseEvent &event) override;

private:

    vec2 _mouseScreen, _mouseWorld;
    core::ViewportControllerRef _viewportController;

};

SMART_PTR(KeyboardViewportControlComponent);

class KeyboardViewportControlComponent : public core::InputComponent {
public:
    
    KeyboardViewportControlComponent(core::ViewportControllerRef viewportController, int dispatchReceiptIndex = 1000);
    
    // InputComponent
    void step(const core::time_state &time) override;
    
    void setPanRate(double panRate) { _panRate = dvec2(panRate,panRate); }
    void setPanRate(dvec2 panRate) { _panRate = panRate; }
    dvec2 getPanRate() const { return _panRate; }
    
    void setRotateRate(double rotateRate) { _rotateRate = rotateRate; }
    double getRotateRate() const { return _rotateRate; }
    
    void setSaleRate(double scaleRate) { _scaleRate = scaleRate; }
    double getScaleRate() const { return _scaleRate; }
    
    void setFastScalar(double fastScalar) { _fastScalar = fastScalar; }
    double getFastScalar() const { return _fastScalar; }

private:
    
    core::ViewportControllerRef _viewportController;
    dvec2 _panRate;
    double _fastScalar;
    double _rotateRate;
    double _scaleRate;
    
};

SMART_PTR(TargetTrackingViewportControlComponent);

class TargetTrackingViewportControlComponent : public core::InputComponent {
public:

    struct tracking {
        // target to look at
        dvec2 world;
        // up vector
        dvec2 up;
        // scale will adjust to fit everything within this radius of look to fit in viewport
        double radius;

        tracking(const dvec2 &w, const dvec2 &u, double r) :
                world(w),
                up(u),
                radius(r) {
        }

        tracking(const tracking &c) :
                world(c.world),
                up(c.up),
                radius(c.radius) {
        }

    };

    class TrackingTarget {
    public:

        virtual tracking getViewportTracking() const = 0;

    };

public:

    TargetTrackingViewportControlComponent(shared_ptr<TrackingTarget> target, core::ViewportControllerRef viewportController, int dispatchReceiptIndex = 1000);

    TargetTrackingViewportControlComponent(core::ViewportControllerRef viewportController, int dispatchReceiptIndex = 1000);

    virtual void setTrackingTarget(shared_ptr<TrackingTarget> target);

    shared_ptr<TrackingTarget> getTrackingTarget() const {
        return _trackingTarget.lock();
    }

    void setTrackingRegion(double trackingAreaRadius, double deadZoneRadius, double correctiveRampPower = 2, double correctiveTightness = 0.99);

    double getTrackingRegionRadius() const {
        return _trackingAreaRadius;
    }

    double getTrackingRegionDeadZoneRadius() const {
        return _trackingAreaDeadZoneRadius;
    }

    double getTrackingRegionCorrectiveRampPower() const {
        return _trackingAreaCorrectiveRampPower;
    }

    // InputComponent
    void step(const core::time_state &time) override;

    bool mouseWheel(const ci::app::MouseEvent &event) override;

protected:

    virtual void _trackNoSlop(const tracking &t, const core::time_state &time);

    virtual void _track(const tracking &t, const core::time_state &time);

private:

    core::ViewportControllerRef _viewportController;
    weak_ptr<TrackingTarget> _trackingTarget;
    double _scale, _trackingAreaRadius, _trackingAreaDeadZoneRadius, _trackingAreaCorrectiveRampPower, _trackingAreaCorrectiveTightness;


};

SMART_PTR(MousePickComponent);

class MousePickComponent : public core::InputComponent {
public:

    MousePickComponent(cpShapeFilter pickFilter, int dispatchReceiptIndex = 0);

    virtual ~MousePickComponent();

    void onReady(core::ObjectRef parent, core::StageRef stage) override;

    void step(const core::time_state &time) override;

    bool mouseDown(const ci::app::MouseEvent &event) override;

    bool mouseUp(const ci::app::MouseEvent &event) override;

    bool mouseMove(const ci::app::MouseEvent &event, const ivec2 &delta) override;

    bool mouseDrag(const ci::app::MouseEvent &event, const ivec2 &delta) override;

    bool isDragging() const {
        return _mouseJoint != nullptr;
    }

    dvec2 getDragPositionWorld() const {
        return v2(cpBodyGetPosition(_mouseBody));
    }

protected:

    void releaseDragConstraint();


private:

    cpShapeFilter _pickFilter;
    cpBody *_mouseBody, *_draggingBody;
    cpConstraint *_mouseJoint;
    vec2 _mouseScreen, _mouseWorld;

};

SMART_PTR(MousePickDrawComponent);

class MousePickDrawComponent : public core::DrawComponent {
public:

    MousePickDrawComponent(ColorA color = ColorA(1, 1, 1, 0.5), float radius = 4);

    void onReady(core::ObjectRef parent, core::StageRef stage) override;

    cpBB getBB() const override {
        return cpBBInfinity;
    }

    void draw(const core::render_state &renderState) override;

    core::VisibilityDetermination::style getVisibilityDetermination() const override {
        return core::VisibilityDetermination::ALWAYS_DRAW;
    }

    int getLayer() const override {
        return numeric_limits<int>::max();
    };

private:

    ColorA _color;
    float _radius;
    weak_ptr<MousePickComponent> _pickComponent;

};

SMART_PTR(KeyboardDelegateComponent);

class KeyboardDelegateComponent : public core::InputComponent {
public:

    typedef function<void(int keyCode)> KeyHandler;

    static KeyboardDelegateComponentRef create(int dispatchReceiptIndex, const initializer_list<int> keycodes);

public:
    KeyboardDelegateComponent(int dispatchReceiptIndex, const initializer_list<int> keycodes);

    KeyboardDelegateComponentRef onPress(KeyHandler h);

    KeyboardDelegateComponentRef onRelease(KeyHandler h);

    void monitoredKeyDown(int keyCode) override;

    void monitoredKeyUp(int keyCode) override;

private:
    function<void(int keyCode)> _upHandler, _downHandler;
};

SMART_PTR(MouseDelegateComponent);

class MouseDelegateComponent : public core::InputComponent {
public:

    typedef function<bool(dvec2 screen, dvec2 world, const ci::app::MouseEvent &event)> MousePressHandler;
    typedef function<bool(dvec2 screen, dvec2 world, const ci::app::MouseEvent &event)> MouseReleaseHandler;
    typedef function<bool(dvec2 screen, dvec2 world, ivec2 deltaScreen, dvec2 deltaWorld, const ci::app::MouseEvent &event)> MouseMoveHandler;
    typedef function<bool(dvec2 screen, dvec2 world, ivec2 deltaScreen, dvec2 deltaWorld, const ci::app::MouseEvent &event)> MouseDragHandler;
    typedef function<bool(dvec2 screen, dvec2 world, double deltaWheel, const ci::app::MouseEvent &event)> MouseWheelHandler;

    static MouseDelegateComponentRef create(int dispatchReceiptIndex);

public:

    MouseDelegateComponent(int dispatchReceiptIndex);

    MouseDelegateComponentRef onPress(MousePressHandler);

    MouseDelegateComponentRef onRelease(MouseReleaseHandler);

    MouseDelegateComponentRef onMove(MouseMoveHandler);

    MouseDelegateComponentRef onDrag(MouseDragHandler);
    
    MouseDelegateComponentRef onWheel(MouseWheelHandler);

    bool mouseDown(const ci::app::MouseEvent &event) override;

    bool mouseUp(const ci::app::MouseEvent &event) override;

    bool mouseMove(const ci::app::MouseEvent &event, const ivec2 &delta) override;

    bool mouseDrag(const ci::app::MouseEvent &event, const ivec2 &delta) override;
    
    bool mouseWheel(const ci::app::MouseEvent &event) override;


private:

    MousePressHandler _pressHandler;
    MouseReleaseHandler _releaseHandler;
    MouseMoveHandler _moveHandler;
    MouseDragHandler _dragHandler;
    MouseWheelHandler _wheelHandler;

};

class SvgAttachmentAdapter : public terrain::AttachmentAdapter {
public:
    SvgAttachmentAdapter(terrain::AttachmentRef attachment, core::util::svg::GroupRef svgDoc);
    
    core::util::svg::GroupRef getSvgDoc() const { return _svgDoc; }
    
    void update(const core::time_state &timeState) override;
    
    void updatePosition(const core::time_state &timeState, dvec2 position, dvec2 rotation, dmat4 transform) override;
        
private:
    
    core::util::svg::GroupRef _svgDoc;
    dvec2 _lastPosition, _linearVel;
    double _lastAngle, _angularVel, _friction;
    
};

#endif /* DevComponents_hpp */
