//
//  System.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/23/17.
//

#include "ParticleSystem.hpp"

#include <chipmunk/chipmunk_unsafe.h>
#include "GlslProgLoader.hpp"

using namespace core;
namespace elements {

    namespace {

        double MinKinematicParticleRadius = 0.05;

        dvec2 safe_normalize(const dvec2 dir) {
            double l2 = lengthSquared(dir);
            if (l2 > 1e-3) {
                double l = sqrt(l2);
                return dir / l;
            }
            return dir;
        }

        // scales value by [1-variance to 1+variance]
        double perturb(Rand &rng, double value, double variance) {
            return value * (1.0 + static_cast<double>(rng.nextFloat(-variance, +variance)));
        }

        interpolator<double> perturb(Rand &rng, const interpolator<double> &value, double variance) {
            interpolator<double> c = value;
            for (auto it(c._values.begin()), end(c._values.end()); it != end; ++it) {
                *it = *it * (1.0 + static_cast<double>(rng.nextFloat(-variance, +variance)));
            }
            return c;
        }

        particle_prototype perturb(Rand &rng, const particle_prototype &value, double variance) {
            particle_prototype c = value;

            c.lifespan = perturb(rng, c.lifespan, variance);
            c.radius = perturb(rng, c.radius, variance);
            c.damping = perturb(rng, c.damping, variance);
            c.additivity = perturb(rng, c.additivity, variance);
            c.mass = perturb(rng, c.mass, variance);
            c.initialVelocity = perturb(rng, c.initialVelocity, variance);

            return c;
        }


    }

#pragma mark - ParticleSimulation

    /*
     size_t _count;
     cpBB _bb;
     vector<particle_prototype> _prototypes;
     core::SpaceAccessRef _spaceAccess;
     */

    ParticleSimulation::ParticleSimulation() :
            BaseParticleSimulation(),
            _count(0),
            _bb(cpBBInvalid) {
    }

    // Component
    void ParticleSimulation::onReady(core::ObjectRef parent, core::StageRef stage) {
        BaseParticleSimulation::onReady(parent, stage);
        _spaceAccess = stage->getSpace();
    }

    void ParticleSimulation::onCleanup() {
        BaseParticleSimulation::onCleanup();
    }

    void ParticleSimulation::update(const core::time_state &time) {
        BaseParticleSimulation::update(time);
        _prepareForSimulation(time);
        _simulate(time);
    }

    // BaseParticleSimulation

    void ParticleSimulation::setParticleCount(size_t count) {
        BaseParticleSimulation::setParticleCount(count);
        _prototypes.resize(count);
    }

    size_t ParticleSimulation::getFirstActive() const {
        // ALWAYS zero
        return 0;
    }

    size_t ParticleSimulation::getActiveCount() const {
        return min(_count, _state.size());
    }

    cpBB ParticleSimulation::getBB() const {
        return _bb;
    }

    // ParticleSimulation

    void ParticleSimulation::emit(const particle_prototype &particle, const dvec2 &world, const dvec2 &dir) {
        _pending.push_back(particle);
        _pending.back().position = world;
        _pending.back()._velocity = dir * particle.initialVelocity;
    }

    void ParticleSimulation::_prepareForSimulation(const core::time_state &time) {
        // run a first pass where we update age and completion, then if necessary perform a compaction pass
        size_t expiredCount = 0;
        const size_t activeCount = getActiveCount();
        const size_t storageSize = _prototypes.size();
        bool sortSuggested = false;

        auto state = _state.begin();
        auto end = state + activeCount;
        auto prototype = _prototypes.begin();
        for (; state != end; ++state, ++prototype) {

            //
            // update age and completion
            //

            prototype->_age += time.deltaT;
            prototype->_completion = prototype->_age / prototype->lifespan;

            if (prototype->_completion > 1) {

                //
                // This particle is expired. Clean it up, and note how many expired we have
                //

                prototype->destroy();
                expiredCount++;
            }
        }

        if (expiredCount > activeCount / 2) {

            //
            // parition templates such that expired ones are at the end; then reset _count accordingly
            // note: We don't need to sort _particleState because it's ephemeral; we'll overwrite what's
            // needed next pass to update()
            //

            auto end = partition(_prototypes.begin(), _prototypes.begin() + activeCount, [](const particle_prototype &prototype) {
                return prototype._completion <= 1;
            });

            _count = end - _prototypes.begin();
            sortSuggested = true;
        }

        //
        //	Process any particles that were emitted
        //

        if (!_pending.empty()) {

            for (auto &particle : _pending) {

                //
                // if a particle already lives at this point, perform any cleanup needed
                //

                const size_t idx = _count % storageSize;
                _prototypes[idx].destroy();

                //
                //	Assign template, and if it's kinematic, create chipmunk physics backing
                //

                _prototypes[idx] = particle;

                if (particle.kinematics) {
                    double mass = particle.mass.getInitialValue();
                    double radius = particle.kinematics.scale * max(particle.radius.getInitialValue(), MinKinematicParticleRadius);
                    double moment = cpMomentForCircle(mass, 0, radius, cpvzero);
                    cpBody *body = cpBodyNew(mass, moment);
                    cpShape *shape = cpCircleShapeNew(body, radius, cpvzero);

                    // set up user data, etc to play well with our "engine"
                    cpBodySetUserData(body, this);
                    cpShapeSetUserData(shape, this);
                    _spaceAccess->addBody(body);
                    _spaceAccess->addShape(shape);

                    // set initial state
                    cpBodySetPosition(body, cpv(particle.position));
                    cpBodySetVelocity(body, cpv(particle._velocity));
                    cpShapeSetFilter(shape, particle.kinematics.filter);
                    cpShapeSetFriction(shape, particle.kinematics.friction);
                    cpShapeSetElasticity(shape, saturate(particle.kinematics.elasticity));

                    _prototypes[idx]._body = body;
                    _prototypes[idx]._shape = shape;
                }

                _prototypes[idx].prepare();

                _count++;

                // we've round-robined, which means a sort might be needed
                if (_count >= storageSize) {
                    sortSuggested = true;
                }
            }

            _pending.clear();
        }

        if (sortSuggested && _keepSorted) {
            // sort so oldest particles are at front of _prototypes
            sort(_prototypes.begin(), _prototypes.begin() + getActiveCount(), [](const particle_prototype &a, const particle_prototype &b) {
                return a._age > b._age;
            });
        }

        // we'll re-enable in _simulate
        // TODO: Consider a more graceful handling of this, since re-enablement below is awkward
        for (auto &state : _state) {
            state.active = false;
        }
    }

    void ParticleSimulation::_simulate(const core::time_state &time) {

        const auto &gravities = getStage()->getGravities();
        cpBB bb = cpBBInvalid;

        auto state = _state.begin();
        auto end = state + getActiveCount();
        auto prototype = _prototypes.begin();
        size_t idx = 0;
        for (; state != end; ++state, ++prototype, ++idx) {

            // _prepareForSimulation deactivates all particles
            // and requires _simulate to re-activate any which should be active

            state->active = prototype->_completion <= 1;
            if (!state->active) {
                continue;
            }

            const auto radius = prototype->radius(prototype->_completion);
            const auto size = radius * M_SQRT2;
            const auto damping = 1 - saturate(prototype->damping(prototype->_completion));
            const auto additivity = prototype->additivity(prototype->_completion);
            const auto mass = prototype->mass(prototype->_completion);
            const auto color = prototype->color(prototype->_completion);

            bool didRotate = false;

            if (!prototype->kinematics) {

                //
                //	Non-kinematic particles get standard velocity + damping + gravity applied
                //

                prototype->position = prototype->position + prototype->_velocity * time.deltaT;

                for (const auto &gravity : gravities) {
                    if (gravity->getGravitationLayer() & prototype->gravitationLayerMask) {
                        auto force = gravity->calculate(prototype->position);
                        prototype->_velocity += mass * force.magnitude * force.dir * time.deltaT;
                    }
                }

                if (damping < 1) {
                    prototype->_velocity *= damping;
                }

            } else {

                //
                // Kinematic bodies are simulated by chipmunk; extract position, rotation etc
                //

                cpShape *shape = prototype->_shape;
                cpBody *body = prototype->_body;

                prototype->position = v2(cpBodyGetPosition(body));

                if (damping < 1) {
                    cpBodySetVelocity(body, cpvmult(cpBodyGetVelocity(body), damping));
                    cpBodySetAngularVelocity(body, damping * cpBodyGetAngularVelocity(body));
                }

                prototype->_velocity = v2(cpBodyGetVelocity(body));

                if (!prototype->orientToVelocity) {
                    dvec2 rotation = v2(cpBodyGetRotation(body));
                    state->right = rotation * size;
                    state->up = rotateCCW(state->right);
                    didRotate = true;
                }

                // now update chipmunk's representation
                cpBodySetMass(body, max(mass, 0.0));
                cpCircleShapeSetRadius(shape, max(radius * prototype->kinematics.scale, MinKinematicParticleRadius));
            }

            if (prototype->orientToVelocity) {
                dvec2 dir = prototype->_velocity;
                double len2 = lengthSquared(dir);
                if (prototype->orientToVelocity && len2 > 1e-2) {
                    state->right = (dir / sqrt(len2)) * size;
                    state->up = rotateCCW(state->right);
                    didRotate = true;
                }
            }

            if (!didRotate) {
                // default rotation
                state->right.x = size;
                state->right.y = 0;
                state->up.x = 0;
                state->up.y = size;
            }

            if (prototype->minVelocity > 0) {
                double vel = length(prototype->_velocity);
                double scale = vel / prototype->minVelocity;
                if (scale < 1) {
                    state->right *= scale;
                    state->up *= scale;
                }
            }

            state->atlasIdx = prototype->atlasIdx;
            state->position = prototype->position;
            state->color = color;
            state->additivity = additivity;

            bb = cpBBExpand(bb, prototype->position, size);
        }

        //
        // update BB and notify
        //

        _bb = bb;
        notifyMoved();
    }

#pragma mark - ParticleEmitter

    namespace {
        ParticleEmitter::emission_id _emission_id_tracker = 0;

        ParticleEmitter::emission_id next_emission_id() {
            return ++_emission_id_tracker;
        }
    }

    void ParticleEmitter::Source::apply(Rand &rng, const dvec2 &world, const dvec2 &normalizedDirOrZero, dvec2 &outWorld, dvec2 &outDir) const {

        // plot position
        if (radius > 0) {
            outWorld = world + (dvec2(rng.nextVec2()) * radius * static_cast<double>(rng.nextFloat()));
        } else {
            outWorld = world;
        }

        // plot spread
        double l2 = lengthSquared(normalizedDirOrZero);
        if (spread > 0 && l2 > 1e-5) {
            dvec2 side = rotateCCW(normalizedDirOrZero);
            dvec2 back = -normalizedDirOrZero;
            if (rng.nextBool()) {
                side = -side;
            }
            double f = static_cast<double>(rng.nextFloat()) * spread;
            if (f < 0.5) {
                outDir = lrp(f / 0.5, normalizedDirOrZero, side);
            } else {
                outDir = lrp((f - 0.5) / 0.5, side, back);
            }
            outDir = normalize(outDir);
        } else {
            outDir = normalizedDirOrZero;
        }
    }

    /*
     ParticleSimulationWeakRef _simulation;
     vector<emission> _emissions;
     vector<emission_prototype> _prototypes;
     vector<int> _prototypeLookup;
    */

    ParticleEmitter::ParticleEmitter(uint32_t seed) :
            _rng(seed) {
    }

    void ParticleEmitter::update(const core::time_state &time) {
        if (ParticleSimulationRef sim = _simulation.lock()) {

            //
            //	Process all active emissions, collecting those which have expired
            //

            vector <emission_id> expired;
            for (auto &it : _emissions) {
                emission &e = it.second;
                e.secondsAccumulator += time.deltaT;

                // this one's done, collect and move on
                if (e.endTime > 0 && e.endTime < time.time) {
                    expired.push_back(e.id);
                    continue;
                }

                double ramp = 1;
                if (e.startTime > 0 && e.endTime > 0) {
                    double life = (time.time - e.startTime) / (e.endTime - e.startTime);
                    ramp = e.envelope(life);
                }

                if (e.secondsAccumulator > e.secondsPerEmission) {
                    // get number of particles to emit
                    double particlesToEmit = ramp * e.secondsAccumulator / e.secondsPerEmission;
                    //CI_LOG_D("ramp: " << ramp << " particlesToEmit: " << particlesToEmit);

                    while (particlesToEmit >= 1) {

                        // emit one particle
                        size_t idx = static_cast<size_t>(_rng.nextUint()) % _prototypeLookup.size();
                        const emission_prototype &proto = _prototypes[_prototypeLookup[idx]];

                        dvec2 world, dir;
                        proto.source.apply(_rng, e.world, e.dir, world, dir);

                        sim->emit(perturb(_rng, proto.prototype, proto.source.variance), world, dir);

                        // remove one particle's worth of seconds from accumulator
                        // accumulator will retain leftovers for next step
                        particlesToEmit -= 1;
                        e.secondsAccumulator -= e.secondsPerEmission;
                    }
                }
            }

            // dispose of expired emissions
            if (!expired.empty()) {
                for (auto id : expired) {
                    _emissions.erase(id);
                }
            }
        }
    }

    void ParticleEmitter::onReady(ObjectRef parent, StageRef stage) {
        if (!getSimulation()) {
            ParticleSimulationRef sim = getSibling<ParticleSimulation>();
            if (sim) {
                setSimulation(sim);
            }
        }
    }

    // ParticleEmitter

    void ParticleEmitter::setSimulation(const ParticleSimulationRef simulation) {
        _simulation = simulation;
    }

    ParticleSimulationRef ParticleEmitter::getSimulation() const {
        return _simulation.lock();
    }

    void ParticleEmitter::seed(uint32_t seed) {
        _rng.seed(seed);
    }

    void ParticleEmitter::add(const particle_prototype &prototype, Source source, int probability) {
        size_t idx = _prototypes.size();
        _prototypes.push_back({prototype, source});
        for (int i = 0; i < probability; i++) {
            _prototypeLookup.push_back(idx);
        }
    }

    size_t ParticleEmitter::emit(dvec2 world, dvec2 normalizedDirOrZero, seconds_t duration, double rate, Envelope env) {
        switch (env) {
            case RampUp:
                return emit(world, normalizedDirOrZero, duration, rate, interpolator<double>({0.0, 1.0}));
                break;
            case Sawtooth:
                return emit(world, normalizedDirOrZero, duration, rate, interpolator<double>({0.0, 1.0, 0.0}));
                break;
            case Mesa:
                return emit(world, normalizedDirOrZero, duration, rate, interpolator<double>({0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0}));
                break;
            case RampDown:
                return emit(world, normalizedDirOrZero, duration, rate, interpolator<double>({1.0, 0.0}));
                break;
        }

        // this shouldn't happen
        return 0;
    }

    size_t ParticleEmitter::emit(dvec2 world, dvec2 normalizedDirOrZero, seconds_t duration, double rate, const interpolator<double> &envelope) {
        emission e;
        e.id = next_emission_id();
        e.startTime = getStage()->getTimeState().time;
        e.endTime = e.startTime + duration;
        e.secondsPerEmission = 1.0 / rate;
        e.secondsAccumulator = 0;
        e.world = world;
        e.dir = safe_normalize(normalizedDirOrZero);
        e.envelope = envelope;
        _emissions[e.id] = e;

        return e.id;
    }

    size_t ParticleEmitter::emit(dvec2 world, dvec2 normalizedDirOrZero, double rate) {
        emission e;
        e.id = next_emission_id();
        e.startTime = e.endTime = -1;
        e.secondsPerEmission = 1.0 / rate;
        e.secondsAccumulator = 0;
        e.world = world;
        e.dir = safe_normalize(normalizedDirOrZero);
        e.envelope = 1;

        _emissions[e.id] = e;

        return e.id;
    }

    void ParticleEmitter::setEmissionPosition(emission_id emissionId, dvec2 world, dvec2 normalizedDirOrZero) {
        auto pos = _emissions.find(emissionId);
        if (pos != _emissions.end()) {
            pos->second.world = world;
            pos->second.dir = safe_normalize(normalizedDirOrZero);
        }
    }

    void ParticleEmitter::cancel(emission_id id) {
        auto pos = _emissions.find(id);
        if (pos != _emissions.end()) {
            _emissions.erase(pos);
        }
    }

    void ParticleEmitter::emitBurst(dvec2 world, dvec2 normalizedDirOrZero, int count, float probability) {
        if (ParticleSimulationRef sim = _simulation.lock()) {
            normalizedDirOrZero = safe_normalize(normalizedDirOrZero);

            for (int i = 0; i < count; i++) {
                if (probability < 1 && _rng.nextFloat() > probability) {
                    continue;
                }
                
                size_t idx = static_cast<size_t>(_rng.nextUint()) % _prototypeLookup.size();
                const emission_prototype &proto = _prototypes[_prototypeLookup[idx]];

                dvec2 modulatedWorld, modulatedDir;
                proto.source.apply(_rng, world, normalizedDirOrZero, modulatedWorld, modulatedDir);

                sim->emit(perturb(_rng, proto.prototype, proto.source.variance), modulatedWorld, modulatedDir);
            }
        }
    }

#pragma mark - ParticleSystemDrawComponent

    /*
     config _config;
     gl::GlslProgRef _shader;
     vector<particle_vertex> _particles;
     gl::VboRef _particlesVbo;
     gl::BatchRef _particlesBatch;
     GLsizei _batchDrawStart, _batchDrawCount;
     */

    ParticleSystemDrawComponent::config ParticleSystemDrawComponent::config::parse(const util::xml::XmlMultiTree &node) {
        config c = BaseParticleSystemDrawComponent::config::parse(node);

        auto textureName = node.getAttribute("textureAtlas");
        if (textureName) {

            auto image = loadImage(app::loadAsset(*textureName));
            gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);

            c.textureAtlas = gl::Texture2d::create(image, fmt);
            c.atlasType = Atlas::fromString(node.getAttribute("atlasType", "None"));
        }

        return c;
    }

    ParticleSystemDrawComponent::ParticleSystemDrawComponent(config c) :
            BaseParticleSystemDrawComponent(c),
            _config(c),
            _batchDrawStart(0),
            _batchDrawCount(0),
            _shader(c.shader) {
    }

    void ParticleSystemDrawComponent::setSimulation(const BaseParticleSimulationRef simulation) {
        BaseParticleSystemDrawComponent::setSimulation(simulation);
        
        // if a shader wasn't provided with the config, use the default one
        if (!_shader) {
            _shader = createDefaultShader();
        }

        // now build our GPU backing. Note, GL_QUADS is deprecated so we need GL_TRIANGLES, which requires 6 vertices to make a quad
        size_t count = simulation->getParticleCount();
        _particles.resize(count * 6, particle_vertex());

        writeStableParticleValues(simulation);
        updateParticles(simulation);

        // create VBO GPU-side which we can stream to
        _particlesVbo = gl::Vbo::create(GL_ARRAY_BUFFER, _particles, GL_STREAM_DRAW);

        geom::BufferLayout particleLayout;
        particleLayout.append(geom::Attrib::POSITION, 2, sizeof(particle_vertex), offsetof(particle_vertex, position));
        particleLayout.append(geom::Attrib::NORMAL, 2, sizeof(particle_vertex), offsetof(particle_vertex, up));
        particleLayout.append(geom::Attrib::TEX_COORD_0, 2, sizeof(particle_vertex), offsetof(particle_vertex, texCoord));
        particleLayout.append(geom::Attrib::TEX_COORD_1, 2, sizeof(particle_vertex), offsetof(particle_vertex, vertexPosition));
        particleLayout.append(geom::Attrib::TEX_COORD_2, 2, sizeof(particle_vertex), offsetof(particle_vertex, random));
        particleLayout.append(geom::Attrib::COLOR, 4, sizeof(particle_vertex), offsetof(particle_vertex, color));
        particleLayout.append(geom::Attrib::BONE_INDEX, geom::DataType::FLOAT, 1, sizeof(particle_vertex), offsetof(particle_vertex, atlasIdx));

        // pair our layout with vbo.
        auto mesh = gl::VboMesh::create(static_cast<uint32_t>(_particles.size()), GL_TRIANGLES, {{particleLayout, _particlesVbo}});
        _particlesBatch = gl::Batch::create(mesh, _shader);
    }

    void ParticleSystemDrawComponent::draw(const render_state &renderState) {
        
        switch(renderState.mode) {
            case RenderMode::GAME: {
                auto sim = getSimulation();
                if (updateParticles(sim)) {
                    gl::ScopedTextureBind tex(_config.textureAtlas, 0);
                    gl::ScopedBlendPremult blender;
                    
                    setShaderUniforms(_shader, renderState);
                    _particlesBatch->draw(_batchDrawStart, _batchDrawCount);
                }
                break;
            }

            case RenderMode::DEVELOPMENT:{
                // draw BB
                cpBB bb = getBB();
                const ColorA bbColor(1, 0.2, 1, 0.5);
                gl::color(bbColor);
                gl::drawStrokedRect(Rectf(bb.l, bb.b, bb.r, bb.t), 1);
                break;
            }

            case RenderMode::COUNT:
                break;
        }
    }
    
    gl::GlslProgRef ParticleSystemDrawComponent::createDefaultShader() const {
        return core::util::loadGlslAsset("core/elements/shaders/particle_system.glsl");
    }

    void ParticleSystemDrawComponent::writeStableParticleValues(const BaseParticleSimulationRef &sim) {
        auto vertex = _particles.begin();
        const auto activeCount = sim->getActiveCount();
        auto stateBegin = sim->getParticleState().begin();
        auto stateEnd = stateBegin + activeCount;
        auto rng = Rand();

        for (auto state = stateBegin; state != stateEnd; ++state) {
            vec2 rand(rng.nextFloat() * 2.0f - 1.0f, rng.nextFloat() * 2.0f - 1.0f);
            vertex->random = rand;
            vertex->vertexPosition = TexCoords[0];
            ++vertex;
            
            vertex->random = rand;
            vertex->vertexPosition = TexCoords[1];
            ++vertex;
            
            vertex->random = rand;
            vertex->vertexPosition = TexCoords[2];
            ++vertex;
            
            vertex->random = rand;
            vertex->vertexPosition = TexCoords[0];
            ++vertex;
            
            vertex->random = rand;
            vertex->vertexPosition = TexCoords[2];
            ++vertex;
            
            vertex->random = rand;
            vertex->vertexPosition = TexCoords[3];
            ++vertex;
        }
    }

    bool ParticleSystemDrawComponent::updateParticles(const BaseParticleSimulationRef &sim) {

        // walk the simulation particle state and write to our vertices
        const Atlas::Type AtlasType = _config.atlasType;
        const vec2 *AtlasOffsets = Atlas::AtlasOffsets(AtlasType);
        const float AtlasScaling = Atlas::AtlasScaling(AtlasType);
        const ColorA transparent(0, 0, 0, 0);
        const vec2 origin(0, 0);

        vec2 shape[4];
        mat2 rotator;
        const auto activeCount = sim->getActiveCount();
        auto vertex = _particles.begin();
        auto stateBegin = sim->getParticleState().begin();
        auto stateEnd = stateBegin + activeCount;
        int verticesWritten = 0;

        for (auto state = stateBegin; state != stateEnd; ++state) {

            // Check if particle is active && visible before writing geometry
            if (state->active && state->color.a >= ALPHA_EPSILON) {


                shape[0] = state->position - state->right - state->up;
                shape[1] = state->position + state->right - state->up;
                shape[2] = state->position + state->right + state->up;
                shape[3] = state->position - state->right + state->up;

                //
                //	For each vertex, assign position, color and texture coordinate
                //	Note, GL_QUADS is deprecated so we have to draw two TRIANGLES
                //

                const vec2 up = state->up;
                const ColorA pc = state->color;
                const ColorA additiveColor(pc.r * pc.a, pc.g * pc.a, pc.b * pc.a, pc.a * (1 - static_cast<float>(state->additivity)));
                const int atlasIdx = static_cast<int>(state->atlasIdx);
                const vec2 atlasOffset = AtlasOffsets[atlasIdx];

                // GL_TRIANGLE
                vertex->position = shape[0];
                vertex->up = up;
                vertex->texCoord = (TexCoords[0] * AtlasScaling) + atlasOffset;
                vertex->color = additiveColor;
                vertex->atlasIdx = atlasIdx;
                ++vertex;

                vertex->position = shape[1];
                vertex->up = up;
                vertex->texCoord = (TexCoords[1] * AtlasScaling) + atlasOffset;
                vertex->color = additiveColor;
                vertex->atlasIdx = atlasIdx;
                ++vertex;

                vertex->position = shape[2];
                vertex->up = up;
                vertex->texCoord = (TexCoords[2] * AtlasScaling) + atlasOffset;
                vertex->color = additiveColor;
                vertex->atlasIdx = atlasIdx;
                ++vertex;

                // GL_TRIANGLE
                vertex->position = shape[0];
                vertex->up = up;
                vertex->texCoord = (TexCoords[0] * AtlasScaling) + atlasOffset;
                vertex->color = additiveColor;
                vertex->atlasIdx = atlasIdx;
                ++vertex;

                vertex->position = shape[2];
                vertex->up = up;
                vertex->texCoord = (TexCoords[2] * AtlasScaling) + atlasOffset;
                vertex->color = additiveColor;
                vertex->atlasIdx = atlasIdx;
                ++vertex;

                vertex->position = shape[3];
                vertex->up = up;
                vertex->texCoord = (TexCoords[3] * AtlasScaling) + atlasOffset;
                vertex->color = additiveColor;
                vertex->atlasIdx = atlasIdx;
                ++vertex;

                verticesWritten += 6;

            } else {
                //
                // active == false or alpha == 0, so this particle isn't visible:
                // write 2 triangles which will not be rendered
                //
                for (int i = 0; i < 6; i++) {
                    vertex->position = origin;
                    vertex->up.x = vertex->up.y = 0;
                    vertex->color = transparent;
                    ++vertex;
                }
            }
        }

        if (_particlesVbo && verticesWritten > 0) {

            // transfer to GPU
            _batchDrawStart = 0;
            _batchDrawCount = static_cast<GLsizei>(activeCount) * 6;
            void *gpuMem = _particlesVbo->mapReplace();
            memcpy(gpuMem, _particles.data(), _batchDrawCount * sizeof(particle_vertex));
            _particlesVbo->unmap();

            return true;
        }

        return false;
    }


#pragma mark - System

    ParticleSystem::config ParticleSystem::config::parse(const util::xml::XmlMultiTree &node) {
        config c;
        c.maxParticleCount = util::xml::readNumericAttribute<size_t>(node, "count", c.maxParticleCount);
        c.keepSorted = util::xml::readBoolAttribute(node, "sorted", c.keepSorted);
        c.drawConfig = ParticleSystemDrawComponent::config::parse(node.getChild("draw"));
        return c;
    }

    ParticleSystemRef ParticleSystem::create(string name, const config &c) {
        auto simulation = make_shared<ParticleSimulation>();
        simulation->setParticleCount(c.maxParticleCount);
        simulation->setShouldKeepSorted(c.keepSorted);
        auto draw = make_shared<ParticleSystemDrawComponent>(c.drawConfig);

        ParticleSystemRef ps = make_shared<ParticleSystem>(name, c);
        ps->addComponent(draw);
        ps->addComponent(simulation);

        return ps;
    }

    ParticleSystem::ParticleSystem(string name, const config &c) :
            BaseParticleSystem(name),
            _config(c) {
    }

    ParticleEmitterRef ParticleSystem::createEmitter() {
        ParticleEmitterRef emitter = make_shared<ParticleEmitter>();
        emitter->setSimulation(getComponent<ParticleSimulation>());
        addComponent(emitter);
        return emitter;
    }

    size_t ParticleSystem::getGravitationLayerMask(cpBody *body) const {
        return _config.kinematicParticleGravitationLayerMask;
    }

} // end namespace elements
