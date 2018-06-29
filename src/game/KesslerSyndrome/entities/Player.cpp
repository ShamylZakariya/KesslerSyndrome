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
#include "GlslProgLoader.hpp"

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

    
#pragma mark - LegTessellator
    
    /*
     LegPhysicsWeakRef _leg;
     vector<dvec2> _bezierControlPoints;
     vector<vec2> _vertices;
     */

    void LegTessellator::computeBezier(const core::render_state &state, Perlin &perlin) {
        const LegPhysicsRef leg = _leg.lock();
        const seconds_t cycle = (state.time * leg->_cycleScale) + leg->_cycleOffset;
        dvec2 cp0Offset(perlin.fBm(cycle + 1), perlin.fBm(cycle + 2));
        dvec2 cp1Offset(perlin.fBm(cycle + 10), perlin.fBm(cycle + 20));
        dvec2 endOffset(perlin.fBm(cycle + 30), perlin.fBm(cycle + 40));
        
        
        // we're doing a simple bezier curve where the control points are equal. we're using
        // the a point along the leg span, offset by its perpendicular
        const auto basePoint = lrp(0.85, leg->getWorldOrigin(), leg->getWorldEnd());
        const auto len = length(leg->getWorldEnd() - leg->getWorldOrigin()) + 1e-4;
        const auto dir = (leg->getWorldEnd() - leg->getWorldOrigin()) / len;
        const auto maxControlPointDeflection = leg->_maxLength * 0.4;
        const auto minControlPointDeflection = leg->_maxLength * 0.05;
        const auto controlPointWiggleRange = leg->_maxLength * 0.5;
        const auto endWiggleRange = leg->_restLength * 0.25;
        const auto controlPointDeflection = lrp((len / leg->_maxLength), minControlPointDeflection, maxControlPointDeflection);
        
        // assign start and end points
        _bezierControlPoints[0] = leg->getWorldOrigin();
        _bezierControlPoints[3] = leg->getWorldEnd() + endWiggleRange * endOffset;
        
        if (dot(rotateCCW(dir), leg->getWorldUp()) > 0) {
            const auto cp = basePoint + (controlPointDeflection * rotateCCW(dir));
            _bezierControlPoints[1] = cp + controlPointWiggleRange * cp0Offset;
            _bezierControlPoints[2] = cp + controlPointWiggleRange * cp1Offset;
        } else {
            const auto cp = basePoint + (controlPointDeflection * rotateCW(dir));
            _bezierControlPoints[1] = cp + controlPointWiggleRange * cp0Offset;
            _bezierControlPoints[2] = cp + controlPointWiggleRange * cp1Offset;
        }
    }

    void LegTessellator::tessellate(const core::render_state &state, float width, size_t subdivisions, ColorA color, vector<vertex> &vertices) {
        const auto step = 1 / static_cast<float>(subdivisions);
        
        // bzA bezier segment start
        // bzB bezier segment end
        // bzDir normalized segment dir
        // bzDirPerp CW-perpendicular to dir
        // tAl tesselated segment start left
        // tAr tesselated segment start right
        // tBl tesselated segment end left
        // tBr tesselated segment end right

        vec2 bzA = _bezierControlPoints[0];
        vec2 bzB = util::bezier(step, _bezierControlPoints[0], _bezierControlPoints[1], _bezierControlPoints[2], _bezierControlPoints[3]);
        vec2 bzDir = normalize(bzB - bzA);
        vec2 bzDirPerp = rotateCW(bzDir);
        vec2 tBl = bzB - width * bzDirPerp;
        vec2 tBr = bzB + width * bzDirPerp;

        // create first triangle
        vertices.push_back({_bezierControlPoints[0], color});
        vertices.push_back({tBl, color});
        vertices.push_back({tBr, color});
        
        bzA = bzB;
        vec2 tAl = tBl;
        vec2 tAr = tBr;

        for (size_t seg = 1; seg < subdivisions - 1; seg++) {
            const auto t = (seg+1) * step;
            bzB = util::bezier(t, _bezierControlPoints[0], _bezierControlPoints[1], _bezierControlPoints[2], _bezierControlPoints[3]);
            bzDir = normalize(bzB - bzA);
            bzDirPerp = rotateCW(bzDir);

            const auto scaledWidth = lrp<float>(t, width, width * 0.5f);
            tBl = bzB - scaledWidth * bzDirPerp;
            tBr = bzB + scaledWidth * bzDirPerp;
            
            vertices.push_back({tAl, color});
            vertices.push_back({tBl, color});
            vertices.push_back({tAr, color});

            vertices.push_back({tAr, color});
            vertices.push_back({tBl, color});
            vertices.push_back({tBr, color});
            
            tAl = tBl;
            tAr = tBr;
        }
        
        // create last triangle
        vertices.push_back({_bezierControlPoints[3], color});
        vertices.push_back({tBr, color});
        vertices.push_back({tBl, color});
    }

#pragma mark - LegBatchDrawer

    /*
     vector<LegTessellatorRef> _legTessellators;
     vector<LegTessellator::vertex> _vertices;
     gl::GlslProgRef _shader;
     gl::VboRef _vbo;
     gl::BatchRef _batch;
     ColorA _legColor;
     */
    LegBatchDrawer::LegBatchDrawer(vector<LegTessellatorRef> legTessellators, ColorA legColor):
        _legTessellators(legTessellators),
        _shader(core::util::loadGlslAsset("kessler/shaders/player_leg.glsl")),
        _legColor(legColor)
    {
    }
    
    void LegBatchDrawer::draw(const core::render_state &state, Perlin &perlin) {
        //
        // update and tesselate our leg geometry
        //

        _vertices.clear();
        for (const auto &lt : _legTessellators) {
            lt->computeBezier(state, perlin);
            lt->tessellate(state, 1.5, 10, _legColor, _vertices);
        }
        
        if (!_vbo) {

            //
            //  Lazily create VBO - note we have a constant number of vertices
            //
    
            _vbo = gl::Vbo::create(GL_ARRAY_BUFFER, _vertices, GL_STREAM_DRAW);

            geom::BufferLayout particleLayout;
            particleLayout.append(geom::Attrib::POSITION, 2, sizeof(LegTessellator::vertex), offsetof(LegTessellator::vertex, position));
            particleLayout.append(geom::Attrib::COLOR, 4, sizeof(LegTessellator::vertex), offsetof(LegTessellator::vertex, color));

            // pair our layout with vbo to create a cinder Batch
            auto mesh = gl::VboMesh::create(static_cast<uint32_t>(_vertices.size()), GL_TRIANGLES, {{particleLayout, _vbo}});
            _batch = gl::Batch::create(mesh, _shader);
        } else {

            //
            // Vbo exists, so just copy new positions over
            //

            void *gpuMem = _vbo->mapReplace();
            memcpy(gpuMem, _vertices.data(), _vertices.size() * sizeof(LegTessellator::vertex));
            _vbo->unmap();
        }
        
        //
        //  Now draw
        //
        
        _batch->draw();
        
        if (state.testGizmoBit(Gizmos::WIREFRAME)) {
            gl::ScopedColor(ColorA(1,0,1,1));
            
            for (int i = 0; i < _vertices.size(); i += 3) {
                vec2 a = _vertices[i + 0].position;
                vec2 b = _vertices[i + 1].position;
                vec2 c = _vertices[i + 2].position;
                gl::drawLine(a, b);
                gl::drawLine(b, c);
                gl::drawLine(c, a);
            }
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

        void drawStrokedCapsule(dvec2 a, dvec2 b, double radius) {
            const dvec2 center = (a + b) * 0.5;
            const double len = distance(a, b);
            const dvec2 dir = (b - a) / len;
            const double angle = atan2(dir.y, dir.x);
            
            gl::ScopedModelMatrix smm;
            mat4 M = glm::translate(dvec3(center.x, center.y, 0)) * glm::rotate(angle, dvec3(0, 0, 1));
            gl::multModelMatrix(M);
            
            gl::drawStrokedRoundedRect(Rectf(-len / 2 - radius, -radius, +len / 2 + radius, +radius), radius, 8);
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
                c.localOrigin = dvec2(0, -HalfHeight) + (dvec2(dir.x,-dir.y) * WheelRadius * 0.75 + (rngContrib * dvec2(rng.nextVec2()) * WheelRadius * 0.1));
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
     LegBatchDrawerRef _legBatchDrawer;
     core::util::svg::GroupRef _svgDoc, _root, _bulb;
     vector<core::util::svg::GroupRef> _eyes;
     ci::Perlin _perlin;
     elements::ParticleSystemRef _thrustParticleSystem;
     elements::ParticleEmitterRef _thrustParticleEmitter;
     elements::ParticleEmitter::emission_id _thrustEmissionId;
     */
    
    PlayerDrawComponent::PlayerDrawComponent():
            EntityDrawComponent(DrawLayers::PLAYER, VisibilityDetermination::FRUSTUM_CULLING),
            _svgDoc(util::svg::Group::loadSvgDocument(app::loadAsset("kessler/players/player.svg"), 1)),
            _thrustEmissionId(0)
    {
    }
    
    PlayerDrawComponent::~PlayerDrawComponent() {
    }
    
    void PlayerDrawComponent::onReady(ObjectRef parent, StageRef stage) {
        DrawComponent::onReady(parent, stage);
        auto physics = getSibling<PlayerPhysicsComponent>();
        _physics = physics;
        
        //
        // determine the approximate world size of the physics component, and scale our SVG accordingly
        //
        
        const auto physicsHeight = physics->getConfig().height;
        const auto svgHeight = _svgDoc->getBB().t - _svgDoc->getBB().b;
        const auto scale = physicsHeight / svgHeight;
        _svgDoc->setScale(scale);
        
        //
        //  Find various components that we animate
        //

        _root = _svgDoc->find("doc/player/Group/root");
        _eyes = _svgDoc->find("doc/player/Group/bulb/eyes")->getGroups();
        _bulb = _svgDoc->find("doc/player/Group/bulb");

        //
        //  Now determine leg color by examining the SVG
        //

        const auto shape = _root->getShapeNamed("leg_root");
        const auto appearance = shape->getAppearance();
        const auto legColor = ColorA(appearance->getFillColor(), appearance->getFillAlpha());
        
        //
        // create the leg tessellators, and a batch drawer to render them
        //
        
        vector<LegTessellatorRef> tesselators;
        for (const auto &leg : physics->getLegs()) {
            tesselators.push_back(make_shared<LegTessellator>(leg));
        }
        _legBatchDrawer = make_shared<LegBatchDrawer>(tesselators, legColor);
        
        //
        //  Build particle system which draws thrust
        //

        buildThrustParticleSystem(stage);
    }
    
    cpBB PlayerDrawComponent::getBB() const {
        if (PlayerPhysicsComponentRef ppc = _physics.lock()) {
            return ppc->getBB();
        }
        
        return cpBBInvalid;
    }
    
    void PlayerDrawComponent::update(const core::time_state &timeState) {
        PlayerPhysicsComponentRef physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");
        
        {
            auto cycle = timeState.time * 0.5 * M_PI;
            auto step = 1 / static_cast<double>(_eyes.size());
            for (auto &eye : _eyes) {
                auto phase = lrp<double>((cos(cycle) + 1) * 0.5, 0.25, 1.0);
                cycle += M_PI * step;
                eye->setOpacity(phase);
            }
        }
        
        {
            auto phase = timeState.time * M_PI + _perlin.fBm(timeState.time * 0.1);
            auto v = (cos(phase) + 1) * 0.5;
            _bulb->setScale(lrp(v, 0.9, 1.1), lrp(1-v, 0.9, 1.1));
        }
        
        if (physics->isFlying()) {
            const auto thrustDir = physics->getJetpackThrustDirection();
            const PlayerPhysicsComponent::wheel footWheel = physics->getFootWheel();
            const auto pos = footWheel.position;
            if (_thrustEmissionId == 0) {
                _thrustEmissionId = _thrustParticleEmitter->emit(pos, thrustDir, 120);
            } else {
                _thrustParticleEmitter->setEmissionPosition(_thrustEmissionId, pos, thrustDir);
            }
        } else {
            _thrustParticleEmitter->cancel(_thrustEmissionId);
            _thrustEmissionId = 0;
        }
    }
    
    void PlayerDrawComponent::draw(const render_state &renderState) {
        drawPlayer(renderState);
        
        if (renderState.testGizmoBit(Gizmos::PHYSICS)) {
            drawPlayerPhysics(renderState);
        }
    }
    
    void PlayerDrawComponent::drawPlayer(const render_state &renderState) {
        PlayerPhysicsComponentRef physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");

        _legBatchDrawer->draw(renderState, _perlin);

        _svgDoc->setPosition(physics->getPosition());
        _svgDoc->setRotation(v2(cpBodyGetRotation(physics->getBody())));
        _svgDoc->draw(renderState);
    }
    
    void PlayerDrawComponent::drawPlayerPhysics(const core::render_state &renderState) {
        PlayerPhysicsComponentRef physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");
        
        gl::ScopedBlendAlpha sba;
        
        const PlayerPhysicsComponent::wheel FootWheel = physics->getFootWheel();
        const PlayerPhysicsComponent::capsule BodyCapsule = physics->getBodyCapsule();
        
        // draw the wheel
        gl::color(1, 1, 1, 0.5);
        gl::drawStrokedCircle(FootWheel.position, FootWheel.radius, 32);
        gl::color(0, 0, 0, 0.5);
        gl::drawLine(FootWheel.position, FootWheel.position + FootWheel.radius * dvec2(cos(FootWheel.radians), sin(FootWheel.radians)));
        
        // draw the capsule
        gl::color(1, 1, 1, 0.5);
        drawStrokedCapsule(BodyCapsule.a, BodyCapsule.b, BodyCapsule.radius);
        
        // draw the ground normal indicator
        gl::color(1, 0, 0, 0.5);
        gl::drawLine(physics->getPosition(), physics->getPosition() + physics->getGroundNormal() * 10.0);
        gl::drawSolidCircle(physics->getPosition(), 1, 12);
        
        // draw the jetpack thrust
        if (physics->isFlying()) {
            const auto thrustDir = physics->getJetpackThrustDirection();
            const auto angle = atan2(thrustDir.y, thrustDir.x) + M_PI_2;
            const auto pos = FootWheel.position;
            
            gl::ScopedModelMatrix smm;
            gl::multModelMatrix(glm::translate(dvec3(pos, 0)) * glm::rotate(angle, dvec3(0, 0, 1)));
            
            gl::color(1, 0, 0, 0.5);
            dvec2 a = dvec2(-FootWheel.radius, 0);
            dvec2 b = dvec2(FootWheel.radius, 0);
            dvec2 c = dvec2(0, -4 * FootWheel.radius);
            gl::drawLine(a,b);
            gl::drawLine(b,c);
            gl::drawLine(c,a);
        }
    }

    void PlayerDrawComponent::buildThrustParticleSystem(StageRef stage) {
        PlayerPhysicsComponentRef physics = _physics.lock();
        CI_ASSERT_MSG(physics, "PlayerPhysicsComponentRef should be accessbile");
        
        const auto footWheel = physics->getFootWheel();
        const auto maxRadius = footWheel.radius * 1;

        auto image = loadImage(app::loadAsset("kessler/textures/Explosion.png"));
        gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
        
        using namespace elements;
        
        ParticleSystem::config config;
        config.maxParticleCount = 500;
        config.keepSorted = true;
        config.drawConfig.drawLayer = getLayer() + 1;
        config.drawConfig.textureAtlas = gl::Texture2d::create(image, fmt);
        config.drawConfig.atlasType = Atlas::TwoByTwo;
        
        _thrustParticleSystem = ParticleSystem::create("PlayerDrawComponent::_thrustParticleSystem", config);
        stage->addObject(_thrustParticleSystem);
        
        //
        // build fire, smoke and spark templates
        //
        
        particle_prototype fire;
        fire.atlasIdx = 0;
        fire.lifespan = 0.75;
        fire.radius = {0, maxRadius, maxRadius * 0.5, 0};
        fire.damping = { 0 };
        fire.additivity = { 1, 0.7 };
        fire.mass = {0};
        fire.initialVelocity = 50;
        fire.gravitationLayerMask = GravitationLayers::GLOBAL;
        fire.color = {ColorA(0,0.5,1,0),ColorA(0,0.5,1,1),ColorA(1,0.5,0,1),ColorA(1,0,0,0)};
        
        particle_prototype smoke;
        smoke.atlasIdx = 0;
        smoke.lifespan = 0.75;
        smoke.radius = {0, 0, maxRadius * 2, 0};
        smoke.damping = {0,0,0,0.02};
        smoke.additivity = 0;
        smoke.mass = {0};
        smoke.initialVelocity = 20;
        smoke.gravitationLayerMask = GravitationLayers::GLOBAL;
        smoke.color = ColorA(0.9, 0.9, 0.9, 0.2);
        
        _thrustParticleEmitter = _thrustParticleSystem->createEmitter();
        _thrustParticleEmitter->add(fire, ParticleEmitter::Source(2, 0.2, 0.3), 2);
        _thrustParticleEmitter->add(smoke, ParticleEmitter::Source(2, 0.3, 0.3), 1);
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
        
        addComponent(_physics);
        addComponent(_input);
        addComponent(_health);
        addComponent(_drawing);
        addComponent(_uiDrawing);
    }
    
}
