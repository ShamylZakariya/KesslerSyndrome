//
//  PlayerPhysicsComponents.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/8/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "PlayerPhysicsComponents.hpp"

#include "GameConstants.hpp"
#include "Player.hpp"

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
     struct config {
     cpBody *unownedParentBody;
     dvec2 localOrigin;
     dvec2 localDir;
     double rotationExtent;
     double maxLength;
     double restLength;
     double cycleScale;
     double cycleOffset;
     };
     
     cpBody *_unownedParentBody;
     cpVect _localOrigin, _localEnd, _localDir, _localRest;
     cpVect _worldOrigin, _worldEnd, _worldContact;
     double _maxLength, _restLength, _rotationExtent, _cosRotationExtent, _cycleScale, _cycleOffset;
     Phase _phase;
     */
    
    LegPhysics::LegPhysics(config c):
    _unownedParentBody(c.unownedParentBody),
    _localOrigin(cpv(c.localOrigin)),
    _localDir(cpv(c.localDir)),
    _localRest(cpv(c.localOrigin + (c.restLength * c.localDir))),
    _worldOrigin(cpBodyLocalToWorld(c.unownedParentBody, _localOrigin)),
    _maxLength(c.maxLength),
    _restLength(c.restLength),
    _rotationExtent(c.rotationExtent),
    _cosRotationExtent(cos(c.rotationExtent)),
    _cycleScale(c.cycleScale),
    _cycleOffset(c.cycleOffset),
    _phase(Phase::ProbingForContact)
    {
        _localEnd = _localRest;
        _worldEnd = cpBodyLocalToWorld(_unownedParentBody, _localEnd);
    }
    
    void LegPhysics::step(const core::time_state &time) {
        _worldOrigin = cpBodyLocalToWorld(_unownedParentBody, _localOrigin);
        
        switch(_phase) {
            case Phase::ProbingForContact:
                // while probing, our endpoint is defined in local space
                _worldEnd = cpBodyLocalToWorld(_unownedParentBody, _localRest);
                
                if (_probeForGroundContact()) {
                    _phase = Phase::Contact;
                }
                
                break;
                
            case Phase::Contact:
                // when in contact with ground, endpoint is defined in world space
                _worldEnd = cpvlerp(_worldEnd, _worldContact, 7 * time.deltaT);
                _localEnd = cpBodyWorldToLocal(_unownedParentBody, _worldEnd);
                
                if (!_canMaintainGroundContact(time)) {
                    _phase = Phase::Retracting;
                }
                
                break;
                
            case Phase::Retracting:
                
                // we perform retraction in local space since we're not attached to the ground any longer
                _localEnd = cpvlerp(_localEnd, _localRest, 7 * time.deltaT);
                _worldEnd = cpBodyLocalToWorld(_unownedParentBody, _localEnd);
                
                if (cpvdistsq(_localEnd, _localRest) < 0.01) {
                    _phase = Phase::ProbingForContact;
                }
                
                break;
        }
    }
    
    bool LegPhysics::_canMaintainGroundContact(const core::time_state &time) {
        if (cpvdistsq(_worldEnd, _worldOrigin) > (_maxLength * _maxLength)) {
            return false;
        }
        
        const auto currentLocalDir = cpvnormalize(cpvsub(_localEnd, _localOrigin));
        const auto deflection = cpvdot(currentLocalDir, _localDir);
        if (deflection < _cosRotationExtent) { // _cosRotationExtent is cos(_rotationExtent)
            return false;
        }
        
        return true;
    }
    
    bool LegPhysics::_probeForGroundContact() {
        const auto rotation = cpBodyGetRotation(_unownedParentBody);
        const auto probeDir = cpvrotate(_getLocalProbeDir(), rotation);
        const auto probeOrigin = _worldOrigin;
        const auto probeEnd = cpvadd(probeOrigin, cpvmult(probeDir, _maxLength));
        
        leg_contact_segment_query_data data(_worldOrigin);
        cpSpaceSegmentQuery(cpBodyGetSpace(_unownedParentBody), probeOrigin, probeEnd, 1, CP_SHAPE_FILTER_ALL, legContactSegmentQueryHandler, &data);
        if (data.dist < _maxLength * 1.0) {
            _worldContact = data.contact;
            return true;
        }
        return false;
    }
    
    cpVect LegPhysics::_getLocalProbeDir() const {
        return _localDir;
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
            
    }
    
    /**
     config _config;
     bool _flying;
     double _speed;
     
     ci::Perlin _perlin;
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
            WheelRadius = Width,
            WheelSensorRadius = WheelRadius * 1.1,
            WheelMass = Density * M_PI * WheelRadius * WheelRadius;
        
        const cpVect WheelPositionOffset = cpv(0, -HalfHeight);
        
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
            const double cycleStep = 1 / static_cast<double>(count);
            ci::Rand rng;
            
            for (size_t i = 0; i < count; i++) {
                const double cycleOffset = i * cycleStep;
                const double angle = startAngle + (legAngleRange * cycleOffset) + rng.nextFloat(-0.05, 0.05);
                const dvec2 dir(cos(angle), sin(angle));
                const double legDeflectionScale = (i / static_cast<double>(count-1)) * 2.0 - 1.0;
                const auto rngContrib = 1.0;
                
                LegPhysics::config c;
                c.unownedParentBody = _body;
                c.localOrigin = dvec2(0, - 0.75 * HalfHeight) + (dvec2(dir.x,-dir.y) * WheelRadius * 0.75 + (rngContrib * dvec2(rng.nextVec2()) * WheelRadius * 0.1));
                c.localDir = normalize(dir + dvec2(0.1 * rngContrib * dvec2(rng.nextVec2())));
                c.rotationExtent = radians(25 + abs(legDeflectionScale) * 65 + rngContrib * rng.nextFloat(-10, 10));
                c.maxLength = Height * (1.5 +  rngContrib * rng.nextFloat(-0.3, 0.3));
                c.restLength = Height * (0.3 + rngContrib * rng.nextFloat(-0.1, 0.1));
                c.cycleOffset = cycleOffset;
                c.cycleScale = 0.5;
                
                _legSimulations.push_back(make_shared<LegPhysics>(c));
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
    
    dvec2 PlayerPhysicsComponent::getLinearVelocity() const {
        return v2(cpBodyGetVelocity(_body));
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
    
}
