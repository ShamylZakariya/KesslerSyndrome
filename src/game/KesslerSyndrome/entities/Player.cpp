//
//  Player.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 5/4/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "Player.hpp"

#include "Bezier.hpp"
#include "GameConstants.hpp"
#include <cinder/Rand.h>

using namespace core;

namespace game {
    
#pragma mark - LegPhysics
    
    namespace {
        
        struct leg_contact_segment_query_data {
            double dist;
            double dist2;
            cpVect contact, start;
            
            leg_contact_segment_query_data(cpVect start):
            dist(numeric_limits<double>::max()),
            dist2(numeric_limits<double>::max()),
            start(start)
            {}
        };
        
        void legContactSegmentQueryHandler(cpShape *shape, cpVect point, cpVect normal, cpFloat alpha, void *dataPtr) {
            if (cpShapeGetCollisionType(shape) != CollisionType::PLAYER) {
                auto data = static_cast<leg_contact_segment_query_data*>(dataPtr);
                double d2 = cpvdistsq(point, data->start);
                if (d2 < data->dist2) {
                    data->dist2 = d2;
                    data->dist = sqrt(d2);
                    data->contact = point;
                }
            }
        }
    }

    /*
     cpBody *_unownedParentBody;
     cpVect _localOrigin, _localEnd, _localDirection, _localRest;
     cpVect _worldOrigin, _worldEnd, _worldDirection, _worldRest, _worldContact, _probeOrigin, _probeEnd;
     double _maxExtension, _maxDeflection, _temporalPhaseOffset, _temporalPhaseDuration;
     Phase _phase;
     */
    
    LegPhysics::LegPhysics(cpBody *unownedParentBody, vec2 originOnParentBody, vec2 directionFromParentBody, double maxExtension, double maxDeflectionRadians, double minExtension, double phaseOffset, double phaseDuration):
        _unownedParentBody(unownedParentBody),
        _localOrigin(cpv(originOnParentBody)),
        _localRest(cpvadd(_localOrigin, cpvmult(_localDirection,minExtension))),
        _localDirection(cpv(directionFromParentBody)),
        _worldOrigin(cpBodyLocalToWorld(unownedParentBody, _localOrigin)),
        _worldEnd(_worldOrigin),
        _maxExtension(maxExtension),
        _maxDeflection(cos(maxDeflectionRadians)),
        _temporalPhaseOffset(phaseOffset),
        _temporalPhaseDuration(phaseDuration),
        _phase(Phase::ProbingForContact)
    {
        _localEnd = _localRest;
    }
    
    void LegPhysics::step(const core::time_state &time) {
        _worldOrigin = cpBodyLocalToWorld(_unownedParentBody, _localOrigin);

        const auto rotation = cpBodyGetRotation(_unownedParentBody);
        _worldDirection = cpvrotate(_localDirection, rotation);

        switch(_phase) {
            case Phase::ProbingForContact:
                // while probing, our endpoint is defined in local space
                _worldEnd = cpBodyLocalToWorld(_unownedParentBody, _localEnd);

                if (_probeForGroundContact()) {
                    _phase = Phase::Contact;
                }

                break;

            case Phase::Contact:
                // when in contact with ground, endpoint is defined in world space
                _worldEnd = cpvlerp(_worldEnd, _worldContact, 25 * time.deltaT);
                _localEnd = cpBodyWorldToLocal(_unownedParentBody, _worldEnd);

                if (!_canMaintainGroundContact(time)) {
                    _phase = Phase::Retracting;
                }

                break;

            case Phase::Retracting:

                // we perform retraction in local space since we're not attached to the ground any longer
                _localEnd = cpvlerp(_localEnd, _localRest, 25 * time.deltaT);
                _worldEnd = cpBodyLocalToWorld(_unownedParentBody, _localEnd);

                if (cpvdistsq(_localEnd, _localRest) < 0.01) {
                    _phase = Phase::ProbingForContact;
                }

                break;
        }
    }
    
    bool LegPhysics::_canMaintainGroundContact(const core::time_state &time) {
        const seconds_t cycle = time.time - floor(time.time);
        if (cycle > _temporalPhaseOffset && cycle < _temporalPhaseOffset + _temporalPhaseDuration) {

            // check for hyperxtension
            if (cpvdistsq(_worldEnd, _worldOrigin) > (_maxExtension * _maxExtension)) {
                return false;
            }
            
            // check for deflection from the natural probe angle
            const auto currentLocalDir = cpvnormalize(cpvsub(_localEnd, _localOrigin));
            const auto deflection = cpvdot(currentLocalDir, _localDirection);
            if (deflection < _maxDeflection) { // maxDeflection is cached as cos(maxDeflection)
                return false;
            }

        }
        
        return true;
    }
    
    bool LegPhysics::_probeForGroundContact() {
        _probeOrigin = _worldOrigin;
        _probeEnd = cpvadd(_probeOrigin, cpvmult(_worldDirection, _maxExtension));
        
        leg_contact_segment_query_data data(_worldOrigin);
        cpSpaceSegmentQuery(cpBodyGetSpace(_unownedParentBody), _probeOrigin, _probeEnd, 1, CP_SHAPE_FILTER_ALL, legContactSegmentQueryHandler, &data);
        if (data.dist < _maxExtension * 0.7) {
            _worldContact = data.contact;
            return true;
        }
        return false;
    }
    
#pragma mark - LegDrawer
    
    /*
     LegPhysicsWeakRef _leg;
     vector<dvec2> _bezierControlPoints;
     */
    void LegDrawer::draw(const core::render_state &state) {
        const LegPhysicsRef leg = _leg.lock();

        // assign start and end points
        _bezierControlPoints[0] = leg->getWorldOrigin();
        _bezierControlPoints[3] = leg->getWorldEnd();

        // we're doing a simple bezier curve where the control points are equal. we're using
        // the a point along the leg span, offset by its perpendicular
        const auto basePoint = lrp(0.85, leg->getWorldOrigin(), leg->getWorldEnd());
        const auto len = length(leg->getWorldEnd() - leg->getWorldOrigin()) + 1e-4;
        const auto dir = (leg->getWorldEnd() - leg->getWorldOrigin()) / len;
        const auto deflectionLen = len * 1;

        if (dot(rotateCCW(dir), leg->getWorldUp()) > 0) {
            const auto cp = basePoint + (deflectionLen * rotateCCW(dir));
            _bezierControlPoints[1] = cp;
            _bezierControlPoints[2] = cp;
        } else {
            const auto cp = basePoint + (deflectionLen * rotateCW(dir));
            _bezierControlPoints[1] = cp;
            _bezierControlPoints[2] = cp;
        }
        
        
        const size_t numSegments = 10;
        const double stepSize = 1 / static_cast<double>(numSegments);
        dvec2 v = _bezierControlPoints[0];

        gl::color(0,1,1,1);
        for (auto seg = 0; seg < numSegments; seg++) {
            const auto t = (seg+1) * stepSize;
            const auto v2 = util::bezier(t, _bezierControlPoints[0], _bezierControlPoints[1], _bezierControlPoints[2], _bezierControlPoints[3]);
            gl::drawLine(v, v2);
            v = v2;
        }
        
    }

    
#pragma mark - PlayerPhysicsComponent
    
    namespace {
        
        void groundContactQuery(cpShape *shape, cpContactPointSet *points, void *data) {
            if (cpShapeGetCollisionType(shape) != CollisionType::PLAYER && !cpShapeGetSensor(shape)) {
                bool *didContact = (bool *) data;
                *didContact = true;
            }
        }
        
        struct ground_slope_segment_query_data {
            double dist, dist2;
            cpVect contact, normal, start, end;
            
            ground_slope_segment_query_data() :
            dist(numeric_limits<double>::max()),
            dist2(numeric_limits<double>::max()),
            normal(cpv(0, 1)) {
            }
        };
        
        void groundSlopeSegmentQueryHandler(cpShape *shape, cpVect point, cpVect normal, cpFloat alpha, void *dataPtr) {
            if (cpShapeGetCollisionType(shape) != CollisionType::PLAYER) {
                auto data = static_cast<ground_slope_segment_query_data *>(dataPtr);
                const double d2 = cpvdistsq(point, data->start);
                if (d2 < data->dist2) {
                    data->dist2 = d2;
                    data->dist = sqrt(d2);
                    data->contact = point;
                    data->normal = normal;
                }
            }
        }
        
        void drawCapsule(dvec2 a, dvec2 b, double radius) {
            const dvec2 center = (a + b) * 0.5;
            const double len = distance(a, b);
            const dvec2 dir = (b - a) / len;
            const double angle = atan2(dir.y, dir.x);
            
            gl::ScopedModelMatrix smm;
            mat4 M = glm::translate(dvec3(center.x, center.y, 0)) * glm::rotate(angle, dvec3(0, 0, 1));
            gl::multModelMatrix(M);
            
            gl::drawSolidRoundedRect(Rectf(-len / 2 - radius, -radius, +len / 2 + radius, +radius), radius, 8);
        }
    }
    
    /**
     config _config;
     bool _flying;
     double _speed;
     
     cpBody *_body, *_wheelBody;
     cpShape *_bodyShape, *_wheelShape, *_groundContactSensorShape;
     cpConstraint *_wheelMotor, *_orientationConstraint;
     double _wheelRadius, _wheelFriction, _touchingGroundAcc, _totalMass;
     double _jetpackFuelLevel, _jetpackFuelMax, _lean;
     dvec2 _up, _groundNormal, _jetpackForceDir;
     PlayerInputComponentWeakRef _input;
     vector<LegPhysicsRef> _legSimulations;
     */

    PlayerPhysicsComponent::PlayerPhysicsComponent(config c) :
    _config(c),
    _flying(false),
    _speed(0),
    _body(nullptr),
    _wheelBody(nullptr),
    _bodyShape(nullptr),
    _wheelShape(nullptr),
    _groundContactSensorShape(nullptr),
    _wheelMotor(nullptr),
    _orientationConstraint(nullptr),
    _wheelRadius(0),
    _wheelFriction(0),
    _touchingGroundAcc(0),
    _jetpackFuelLevel(0),
    _jetpackFuelMax(0),
    _lean(0),
    _up(0, 0),
    _groundNormal(0, 0)
    {
    }
    
    PlayerPhysicsComponent::~PlayerPhysicsComponent() {
    }
    
    cpBB PlayerPhysicsComponent::getBB() const {
        if (_body) {
            return cpBBNewForCircle(cpv(getPosition()), getConfig().height);
        } else {
            return cpBBInvalid;
        }
    }
    
    void PlayerPhysicsComponent::onReady(ObjectRef parent, StageRef stage) {
        PhysicsComponent::onReady(parent, stage);
        
        _input = getSibling<PlayerInputComponent>();
        
        const PlayerRef player = static_pointer_cast<Player>(parent);
        const PlayerPhysicsComponent::config &config = getConfig();
        
        _jetpackFuelLevel = _jetpackFuelMax = config.jetpackFuelMax;
        
        const double
            Width = getConfig().width,
            Height = getConfig().height,
            Density = getConfig().density,
            HalfWidth = Width / 2,
            HalfHeight = Height / 2,
            Mass = Width * Height * Density,
            Moment = cpMomentForBox(Mass, Width, Height),
            WheelRadius = HalfWidth * 1.25,
            WheelSensorRadius = WheelRadius * 1.1,
            WheelMass = Density * M_PI * WheelRadius * WheelRadius;
        
        const cpVect
            WheelPositionOffset = cpv(0, -HalfHeight);
        
        _totalMass = Mass + WheelMass;
        _body = add(cpBodyNew(Mass, Moment));
        cpBodySetPosition(_body, cpv(getConfig().position + getConfig().localUp * (HalfHeight + WheelRadius)));
        
        //
        // lozenge shape for body and gear joint to orient it
        //
        
        _bodyShape = add(cpSegmentShapeNew(_body, cpv(0, -(HalfHeight)), cpv(0, HalfHeight), Width / 2));
        cpShapeSetFriction(_bodyShape, config.bodyFriction);
        
        _orientationConstraint = add(cpGearJointNew(cpSpaceGetStaticBody(getSpace()->getSpace()), _body, 0, 1));
        
        //
        // make wheel at bottom of body
        //
        
        _wheelRadius = WheelRadius;
        _wheelBody = add(cpBodyNew(WheelMass, cpMomentForCircle(WheelMass, 0, _wheelRadius, cpvzero)));
        cpBodySetPosition(_wheelBody, cpvadd(cpBodyGetPosition(_body), WheelPositionOffset));
        _wheelShape = add(cpCircleShapeNew(_wheelBody, _wheelRadius, cpvzero));
        cpShapeSetFriction(_wheelShape, config.footFriction);
        cpShapeSetElasticity(_wheelShape, config.footElasticity);
        _wheelMotor = add(cpSimpleMotorNew(_body, _wheelBody, 0));
        
        //
        //    pin joint connecting wheel and body
        //
        
        add(cpPivotJointNew(_body, _wheelBody, cpBodyLocalToWorld(_wheelBody, cpvzero)));
        
        
        //
        //    create sensor for recording ground contact
        //
        
        _groundContactSensorShape = add(cpCircleShapeNew(_wheelBody, WheelSensorRadius, cpvzero));
        cpShapeSetSensor(_groundContactSensorShape, true);
        
        //
        //    Finalize
        //
        
        // note: group has to be a unique integer so the player parts don't collide with eachother
        cpShapeFilter filter = ShapeFilters::PLAYER;
        filter.group = reinterpret_cast<cpGroup>(player.get());
        
        //
        //  Build our legs
        //
        
        {
            const size_t count = 8;
            const double legAngleRange = radians(135.0);
            const double startAngle = radians(-90.0) - legAngleRange * 0.5;
            const double phaseDuration = 1 / static_cast<double>(count);
            ci::Rand rng;
            
            for (size_t i = 0; i < count; i++) {
                const double phaseOffset = i * phaseDuration;
                const double angle = startAngle + (legAngleRange * phaseOffset) + rng.nextFloat(-0.05, 0.05);
                const dvec2 dir(cos(angle), sin(angle));
                const double legDeflectionScale = (i / static_cast<double>(count-1)) * 2.0 - 1.0;
                
                const dvec2 originOnParentBody = dvec2(0, -HalfHeight) + (dir * WheelRadius * 0.75);
                const dvec2 probeDir = normalize(dir + dvec2(0.1f * rng.nextVec2()));
                const double maxLegExtension = Height * (1.5 + rng.nextFloat(-0.3, 0.3));
                const double maxLegDeflection = radians(25 + abs(legDeflectionScale) * 65 + rng.nextFloat(-10, 10));
                const double minLegExtension = Height * (0.3 + rng.nextFloat(-0.1, 0.1));

                const auto lp = make_shared<LegPhysics>(_body, originOnParentBody, probeDir, maxLegExtension, maxLegDeflection, minLegExtension, phaseOffset, phaseDuration);
                _legSimulations.push_back(lp);
            }
        }
        
        build(filter, CollisionType::PLAYER);
    }
    
    void PlayerPhysicsComponent::step(const time_state &timeState) {
        PhysicsComponent::step(timeState);

        const PlayerRef player = getObjectAs<Player>();
        const PlayerPhysicsComponent::config config = getConfig();
        
        const bool
            Alive = true,
            Restrained = false;
        
        const double
            Vel = getSpeed(),
            Dir = sign(getSpeed());
        
        const auto G = getSpace()->getGravity(v2(cpBodyGetPosition(_wheelBody)));
        
        const dvec2
            Down = G.dir,
            Right = rotateCCW(Down);
        
        //
        //    Smoothly update touching ground state as well as ground normal
        //
        
        _touchingGroundAcc = lrp<double>(0.2, _touchingGroundAcc, _isTouchingGround(_groundContactSensorShape) ? 1.0 : 0.0);
        _groundNormal = normalize(lrp(0.2, _groundNormal, _getGroundNormal()));
        
        {
            const dvec2 ActualUp = -Down;
            _up = normalize(lrp(0.2, _up, ActualUp));
            
            double target = std::atan2(_up.y, _up.x) - M_PI_2;
            double phase = cpGearJointGetPhase(_orientationConstraint);
            double turn = closest_turn_radians(phase, target);
            double newPhase = phase + turn;
            
            // add lean
            _lean = lrp<double>(0.2, _lean, Dir);
            newPhase -= _lean * M_PI * 0.05;
            
            cpGearJointSetPhase(_orientationConstraint, newPhase);
        }
        
        //
        // rotate the wheel motor
        //
        
        if (Alive && !Restrained) {
            const double
            WheelCircumference = 2 * _wheelRadius * M_PI,
            RequiredTurns = Vel / WheelCircumference,
            DesiredMotorRate = 2 * M_PI * RequiredTurns,
            CurrentMotorRate = cpSimpleMotorGetRate(_wheelMotor),
            TransitionRate = 0.1,
            MotorRate = lrp<double>(TransitionRate, CurrentMotorRate, DesiredMotorRate);
            
            cpSimpleMotorSetRate(_wheelMotor, MotorRate);
        } else {
            cpSimpleMotorSetRate(_wheelMotor, 0);
        }
        
        //
        //    Apply jetpack impulse
        //
        
        bool flying = isFlying() && _jetpackFuelLevel > 0;
        
        if (flying) {
            _jetpackForceDir = normalize((-2 * Dir * Right) + G.dir);
            dvec2 force = -config.jetpackAntigravity * _totalMass * G.magnitude * _jetpackForceDir;
            cpBodyApplyForceAtWorldPoint(_body, cpv(force), cpBodyLocalToWorld(_wheelBody, cpvzero));
            _jetpackFuelLevel -= config.jetpackFuelConsumptionPerSecond * timeState.deltaT;
        } else if (!isFlying()) {
            if (_jetpackFuelLevel < _config.jetpackFuelMax) {
                _jetpackFuelLevel += config.jetpackFuelRegenerationPerSecond * timeState.deltaT;
                _jetpackFuelLevel = min(_jetpackFuelLevel, _config.jetpackFuelMax);
            }
        }
        
        //
        //    In video games characters can 'steer' in mid-air. It's expected, even if unrealistic.
        //    So we do it here, too.
        //
        
        if (!Restrained && !isTouchingGround()) {
            const double
            ActualVel = cpBodyGetVelocity(_body).x;
            
            double
            baseImpulseToApply = 0,
            baseImpulse = flying ? 1 : 0.5;
            
            //
            //    We want to apply force so long as applying force won't take us faster
            //    than our maximum walking speed
            //
            
            if (int(sign(ActualVel)) == int(sign(Vel))) {
                if (std::abs(ActualVel) < std::abs(Vel)) {
                    baseImpulseToApply = baseImpulse;
                }
            } else {
                baseImpulseToApply = baseImpulse;
            }
            
            if (abs(baseImpulseToApply) > 1e-3) {
                dvec2 impulse = Dir * baseImpulseToApply * _totalMass * Right;
                cpBodyApplyImpulseAtWorldPoint(_body, cpv(impulse), cpBodyGetPosition(_body));
            }
        }
        
        
        //
        //    If restrained, we lose friction
        //
        
        {
            _wheelFriction = lrp<double>(0.2, _wheelFriction, Restrained ? 0 : config.footFriction);
            cpShapeSetFriction(_wheelShape, _wheelFriction);
        }
        
        //
        //    If player has died, release rotation constraint
        //
        
        if (!Alive) {
            cpConstraintSetMaxForce(_orientationConstraint, 0);
            cpConstraintSetMaxBias(_orientationConstraint, 0);
            cpConstraintSetErrorBias(_orientationConstraint, 0);
            for (cpShape *shape : getShapes()) {
                cpShapeSetFriction(shape, 0);
            }
        } else {
            // orientation constraint may have been lessened when player was crushed, we always want to ramp it back up
            const double MaxForce = cpConstraintGetMaxForce(_orientationConstraint);
            if (MaxForce < 10000) {
                cpConstraintSetMaxForce(_orientationConstraint, max(MaxForce, 10.0) * 1.1);
            } else {
                cpConstraintSetMaxForce(_orientationConstraint, INFINITY);
            }
        }
        
        //
        //  Update legs
        //
        
        for (const auto &leg : _legSimulations) {
            leg->step(timeState);
        }
        
        //
        //    Draw component needs update its BB for draw dispatch
        //
        
        notifyMoved();
    }
    
    dvec2 PlayerPhysicsComponent::getPosition() const {
        return v2(cpBodyGetPosition(_body));
    }
    
    dvec2 PlayerPhysicsComponent::getUp() const {
        return _up;
    }
    
    dvec2 PlayerPhysicsComponent::getGroundNormal() const {
        return _groundNormal;
    }
    
    bool PlayerPhysicsComponent::isTouchingGround() const {
        return _touchingGroundAcc >= 0.5;
    }
    
    cpBody *PlayerPhysicsComponent::getBody() const {
        return _body;
    }
    
    cpBody *PlayerPhysicsComponent::getFootBody() const {
        return _wheelBody;
    }
    
    cpShape *PlayerPhysicsComponent::getBodyShape() const {
        return _bodyShape;
    }
    
    cpShape *PlayerPhysicsComponent::getFootShape() const {
        return _wheelShape;
    }
    
    double PlayerPhysicsComponent::getJetpackFuelLevel() const {
        return _jetpackFuelLevel;
    }
    
    double PlayerPhysicsComponent::getJetpackFuelMax() const {
        return _jetpackFuelMax;
    }
    
    dvec2 PlayerPhysicsComponent::getJetpackThrustDirection() const {
        return _jetpackForceDir;
    }
    
    PlayerPhysicsComponent::capsule PlayerPhysicsComponent::getBodyCapsule() const {
        capsule cap;
        cap.a = v2(cpBodyLocalToWorld(cpShapeGetBody(_bodyShape), cpSegmentShapeGetA(_bodyShape)));
        cap.b = v2(cpBodyLocalToWorld(cpShapeGetBody(_bodyShape), cpSegmentShapeGetB(_bodyShape)));
        cap.radius = cpSegmentShapeGetRadius(_bodyShape);
        return cap;
    }
    
    PlayerPhysicsComponent::wheel PlayerPhysicsComponent::getFootWheel() const {
        wheel w;
        w.position = v2(cpBodyGetPosition(_wheelBody));
        w.radius = cpCircleShapeGetRadius(_wheelShape);
        w.radians = cpBodyGetAngle(_wheelBody);
        
        return w;
    }
    
    dvec2 PlayerPhysicsComponent::_getGroundNormal() const {
        const double
        HalfWidth = getConfig().width * 0.5,
        SegmentRadius = HalfWidth * 0.125,
        CastDistance = 10000;
        
        const dvec2 Position(v2(cpBodyGetPosition(getFootBody())));
        
        const auto G = getSpace()->getGravity(Position);
        
        const dvec2
        Down = G.dir,
        Right = rotateCCW(Down);
        
        const cpVect
        DownCast = cpvmult(cpv(Down), CastDistance);
        
        ground_slope_segment_query_data data;
        cpSpace *space = getSpace()->getSpace();
        
        data.start = cpv(Position + Right);
        data.end = cpvadd(data.start, DownCast);
        cpSpaceSegmentQuery(space, data.start, data.end, SegmentRadius, CP_SHAPE_FILTER_ALL, groundSlopeSegmentQueryHandler, &data);
        cpVect normal = data.normal;
        
        data.start = cpv(Position);
        data.end = cpvadd(data.start, DownCast);
        cpSpaceSegmentQuery(space, data.start, data.end, SegmentRadius, CP_SHAPE_FILTER_ALL, groundSlopeSegmentQueryHandler, &data);
        normal = cpvadd(normal, data.normal);
        
        data.start = cpv(Position - Right);
        data.end = cpvadd(data.start, DownCast);
        cpSpaceSegmentQuery(space, data.start, data.end, SegmentRadius, CP_SHAPE_FILTER_ALL, groundSlopeSegmentQueryHandler, &data);
        normal = cpvadd(normal, data.normal);
        
        return normalize(v2(normal));
    }
    
    bool PlayerPhysicsComponent::_isTouchingGround(cpShape *shape) const {
        bool touchingGroundQuery = false;
        cpSpaceShapeQuery(getSpace()->getSpace(), shape, groundContactQuery, &touchingGroundQuery);
        
        return touchingGroundQuery;
    }
    
#pragma mark - PlayerInputComponent
    
    PlayerInputComponent::PlayerInputComponent() {
        _keyRun = app::KeyEvent::KEY_LSHIFT;
        _keyLeft = app::KeyEvent::KEY_a;
        _keyRight = app::KeyEvent::KEY_d;
        _keyJump = app::KeyEvent::KEY_w;
        _keyCrouch = app::KeyEvent::KEY_s;
        
        monitorKey(_keyLeft);
        monitorKey(_keyRight);
        monitorKey(_keyJump);
        monitorKey(_keyCrouch);
        monitorKey(_keyRun);
    }
    
    PlayerInputComponent::~PlayerInputComponent() {
    }
    
    bool PlayerInputComponent::isRunning() const {
        return isMonitoredKeyDown(_keyRun);
    }
    
    bool PlayerInputComponent::isGoingRight() const {
        return isMonitoredKeyDown(_keyRight);
    }
    
    bool PlayerInputComponent::isGoingLeft() const {
        return isMonitoredKeyDown(_keyLeft);
    }
    
    bool PlayerInputComponent::isJumping() const {
        return isMonitoredKeyDown(_keyJump);
    }
    
    bool PlayerInputComponent::isCrouching() const {
        return isMonitoredKeyDown(_keyCrouch);
    }
    
#pragma mark - PlayerDrawComponent
    
    /*
     PlayerPhysicsComponentWeakRef _physics;
     vector<LegDrawerRef> _legDrawers;
     */
    
    PlayerDrawComponent::PlayerDrawComponent():
            EntityDrawComponent(DrawLayers::PLAYER, VisibilityDetermination::FRUSTUM_CULLING)
    {
    }
    
    PlayerDrawComponent::~PlayerDrawComponent() {
    }
    
    void PlayerDrawComponent::onReady(ObjectRef parent, StageRef stage) {
        DrawComponent::onReady(parent, stage);
        auto physics = getSibling<PlayerPhysicsComponent>();
        _physics = physics;
        
        for (const auto &leg : physics->getLegs()) {
            _legDrawers.push_back(make_shared<LegDrawer>(leg));
        }
    }
    
    cpBB PlayerDrawComponent::getBB() const {
        if (PlayerPhysicsComponentRef ppc = _physics.lock()) {
            return ppc->getBB();
        }
        
        return cpBBInvalid;
    }
    
    void PlayerDrawComponent::draw(const render_state &renderState) {
        drawPlayer(renderState);
    }
    
    void PlayerDrawComponent::drawPlayer(const render_state &renderState) {
        PlayerPhysicsComponentRef physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");
        
        gl::ScopedBlendAlpha sba;
        
        const PlayerPhysicsComponent::wheel FootWheel = physics->getFootWheel();
        const PlayerPhysicsComponent::capsule BodyCapsule = physics->getBodyCapsule();
        
        // draw the wheel
        gl::color(1, 1, 1);
        gl::drawSolidCircle(FootWheel.position, FootWheel.radius, 32);
        gl::color(0, 0, 0);
        gl::drawLine(FootWheel.position, FootWheel.position + FootWheel.radius * dvec2(cos(FootWheel.radians), sin(FootWheel.radians)));
        
        // draw the capsule
        gl::color(1, 1, 1);
        drawCapsule(BodyCapsule.a, BodyCapsule.b, BodyCapsule.radius);
        
        // draw the ground normal indicator
        gl::color(1, 0, 0);
        gl::drawLine(physics->getPosition(), physics->getPosition() + physics->getGroundNormal() * 10.0);
        gl::drawSolidCircle(physics->getPosition(), 1, 12);
        
        // draw the jetpack thrust
        if (physics->isFlying()) {
            const auto thrustDir = physics->getJetpackThrustDirection();
            const auto angle = atan2(thrustDir.y, thrustDir.x) + M_PI_2;
            const auto pos = FootWheel.position;
            
            gl::ScopedModelMatrix smm;
            gl::multModelMatrix(glm::translate(dvec3(pos, 0)) * glm::rotate(angle, dvec3(0, 0, 1)));
            
            gl::color(1, 0, 0);
            gl::drawSolidTriangle(dvec2(-FootWheel.radius, 0), dvec2(FootWheel.radius, 0), dvec2(0, -4 * FootWheel.radius));
        }
        
        for (const auto &ld : _legDrawers) {
            ld->draw(renderState);
        }
    }
    
#pragma mark - PlayerUIDrawComponent
    
    /*
     PlayerPhysicsComponentWeakRef _physics;
     */
    
    PlayerUIDrawComponent::PlayerUIDrawComponent():
            ScreenDrawComponent(ScreenDrawLayers::PLAYER)
    {}
    
    PlayerUIDrawComponent::~PlayerUIDrawComponent()
    {}
    
    void PlayerUIDrawComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        ScreenDrawComponent::onReady(parent, stage);
        _physics = getSibling<PlayerPhysicsComponent>();
    }
    
    void PlayerUIDrawComponent::drawScreen(const core::render_state &renderState) {
        drawCharge(renderState);
    }
    
    void PlayerUIDrawComponent::drawCharge(const core::render_state &renderState) {
        auto physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");

        Rectd bounds = renderState.viewport->getBounds();        
        int w = 20;
        int h = 60;
        int p = 10;
        Rectf chargeRectFrame(bounds.getWidth() - p, p, bounds.getWidth() - p - w, p + h);
        
        gl::color(0, 1, 1, 1);
        gl::drawStrokedRect(chargeRectFrame);
        
        double charge = saturate(physics->getJetpackFuelLevel() / physics->getJetpackFuelMax());
        int fillHeight = static_cast<int>(ceil(charge * h));
        Rectf chargeRectFill(bounds.getWidth() - p, p + h - fillHeight, bounds.getWidth() - p - w, p + h);
        gl::drawSolidRect(chargeRectFill);
    }

    
#pragma mark - Player
    
    /**
     config _config;
     PlayerPhysicsComponentRef _physics;
     PlayerDrawComponentRef _drawing;
     PlayerInputComponentRef _input;
     */
    PlayerRef Player::create(string name, ci::DataSourceRef playerXmlFile, dvec2 position, dvec2 localUp) {
        Player::config config;
        
        XmlTree playerNode = XmlTree(playerXmlFile).getChild("player");
        config.walkSpeed = util::xml::readNumericAttribute<double>(playerNode, "walkSpeed", 1);
        config.runMultiplier = util::xml::readNumericAttribute<double>(playerNode, "runMultiplier", 10);
        
        //
        //    Physics
        //
        
        XmlTree physicsNode = playerNode.getChild("physics");
        
        config.physics.position = position;
        config.physics.localUp = localUp;
        config.physics.width = util::xml::readNumericAttribute<double>(physicsNode, "width", 5);
        config.physics.height = util::xml::readNumericAttribute<double>(physicsNode, "height", 20);
        config.physics.density = util::xml::readNumericAttribute<double>(physicsNode, "density", 1);
        config.physics.footFriction = util::xml::readNumericAttribute<double>(physicsNode, "footFriction", 1);
        config.physics.footElasticity = util::xml::readNumericAttribute<double>(physicsNode, "footElasticity", 1);
        config.physics.bodyFriction = util::xml::readNumericAttribute<double>(physicsNode, "bodyFriction", 0.2);
        
        XmlTree jetpackNode = physicsNode.getChild("jetpack");
        config.physics.jetpackAntigravity = util::xml::readNumericAttribute<double>(jetpackNode, "antigravity", 10);
        config.physics.jetpackFuelMax = util::xml::readNumericAttribute<double>(jetpackNode, "fuelMax", 1);
        config.physics.jetpackFuelConsumptionPerSecond = util::xml::readNumericAttribute<double>(jetpackNode, "consumption", 0.3);
        config.physics.jetpackFuelRegenerationPerSecond = util::xml::readNumericAttribute<double>(jetpackNode, "regeneration", 0.3);
        
        //
        //    Health
        //
        
        config.health = HealthComponent::loadConfig(playerNode.getChild("health"));
        
        //
        //    Construct
        //
        
        PlayerRef player = make_shared<Player>(name);
        player->build(config);
        
        return player;
    }
    
    Player::Player(string name) :
    Entity(name) {
    }
    
    Player::~Player() {
    }
    
    void Player::onHealthChanged(double oldHealth, double newHealth) {
        Entity::onHealthChanged(oldHealth, newHealth);
    }
    
    void Player::onDeath() {
        Entity::onDeath();
    }
    
    void Player::update(const time_state &time) {
        Entity::update(time);
        
        //
        //    Synchronize - dispatch player input to physics component
        //
        
        if (_health->isAlive()) {
            double direction = 0;
            
            if (_input->isGoingRight()) {
                direction += 1;
            }
            
            if (_input->isGoingLeft()) {
                direction -= 1;
            }
            
            _physics->setSpeed(direction * _config.walkSpeed * (_input->isRunning() ? _config.runMultiplier : 1.0));
            _physics->setFlying(_input->isJumping());
        }
    }
    
    dvec2 Player::getTrackingPosition() const {
        return _physics->getPosition();
    }
    
    void Player::build(config c) {
        _config = c;
        _input = make_shared<PlayerInputComponent>();
        _drawing = make_shared<PlayerDrawComponent>();
        _uiDrawing = make_shared<PlayerUIDrawComponent>();
        _physics = make_shared<PlayerPhysicsComponent>(c.physics);
        _health = make_shared<HealthComponent>(c.health);
        
        addComponent(_input);
        addComponent(_drawing);
        addComponent(_physics);
        addComponent(_health);
        addComponent(_uiDrawing);
    }
    
}
