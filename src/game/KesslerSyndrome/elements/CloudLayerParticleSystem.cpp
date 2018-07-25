//
//  CloudLayerParticleSystem.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/18/17.
//

#include "game/KesslerSyndrome/elements/CloudLayerParticleSystem.hpp"

#include "core/util/GlslProgLoader.hpp"
#include "core/filters/Filters.hpp"

using namespace core;
using namespace elements;


namespace game {
    
    CloudLayerParticleSimulation::noise_config CloudLayerParticleSimulation::noise_config::parse(const XmlTree &node) {
        noise_config n;
        n.octaves = util::xml::readNumericAttribute<int>(node, "noise", n.octaves);
        n.seed = util::xml::readNumericAttribute<int>(node, "seed", n.seed);
        return n;
    }


    CloudLayerParticleSimulation::particle_config CloudLayerParticleSimulation::particle_config::parse(const XmlTree &node) {
        particle_config pt;
        pt.minRadius = util::xml::readNumericAttribute<double>(node, "minRadius", pt.minRadius);
        pt.maxRadius = util::xml::readNumericAttribute<double>(node, "maxRadius", pt.maxRadius);
        pt.minRadiusNoiseValue = util::xml::readNumericAttribute<double>(node, "minRadiusNoiseValue", pt.minRadiusNoiseValue);
        pt.color = util::xml::readColorAttribute(node, "color", pt.color);

        return pt;
    }


    CloudLayerParticleSimulation::config CloudLayerParticleSimulation::config::parse(const XmlTree &node) {
        config c;

        c.noise = noise_config::parse(node.getChild("noise"));
        c.particle = particle_config::parse(node.getChild("particle"));
        c.origin = util::xml::readPointAttribute(node, "origin", c.origin);
        c.radius = util::xml::readNumericAttribute<double>(node, "radius", c.radius);
        c.count = util::xml::readNumericAttribute<size_t>(node, "count", c.count);
        c.period = util::xml::readNumericAttribute<seconds_t>(node, "period", c.period);
        c.turbulence = util::xml::readNumericAttribute<double>(node, "turbulence", c.turbulence);
        c.displacementForce = util::xml::readNumericAttribute<double>(node, "displacementForce", c.displacementForce);
        c.returnForce = util::xml::readNumericAttribute<double>(node, "returnForce", c.returnForce);

        return c;
    }

    /*
     config _config;
     ci::Perlin _noise;
     core::seconds_t _time;
     cpBB _bb;
     vector<core::RadialGravitationCalculatorRef> _displacements;
     vector<particle_physics> _physics;
     */

    CloudLayerParticleSimulation::CloudLayerParticleSimulation(const config &c) :
            _config(c),
            _noise(c.noise.octaves, c.noise.seed),
            _time(0) {
    }

    void CloudLayerParticleSimulation::onReady(ObjectRef parent, StageRef stage) {

        //
        // create store, set defult state, and run simulate() once to populate
        //
        setParticleCount(_config.count);

        double a = 0;
        double da = 2 * M_PI / getActiveCount();
        auto state = _state.begin();
        auto physics = _physics.begin();
        for (size_t i = 0, N = getParticleCount(); i < N; i++, ++state, ++physics, a += da) {

            // set up initial physics state
            physics->position = physics->previous_position = physics->home = _config.origin + _config.radius * dvec2(cos(a), sin(a));
            physics->damping = Rand::randFloat(0.4, 0.7);
            physics->velocity = dvec2(0, 0);
            physics->radius = 0;

            state->atlasIdx = 0;
            state->color = _config.particle.color;
            state->additivity = 0;
            state->position = physics->home;
            state->active = true; // always active
        }

        simulate(stage->getTimeState());
    }

    void CloudLayerParticleSimulation::update(const time_state &timeState) {
        _time += timeState.deltaT;
        simulate(timeState);
    }

    void CloudLayerParticleSimulation::setParticleCount(size_t count) {
        BaseParticleSimulation::setParticleCount(count);
        _physics.resize(count);
    }

    void CloudLayerParticleSimulation::addGravityDisplacement(const RadialGravitationCalculatorRef &gravity) {
        _displacements.push_back(gravity);
    }

    void CloudLayerParticleSimulation::simulate(const time_state &timeState) {

        if (!_displacements.empty()) {
            pruneDisplacements();
            applyGravityDisplacements(timeState);
        }

        double a = 0;
        const double TwoPi = 2 * M_PI;
        const double da = TwoPi / getActiveCount();
        const double cloudLayerRadius = _config.radius;
        const double cloudLayerRadius2 = cloudLayerRadius * cloudLayerRadius;
        const double particleRadiusDelta = _config.particle.maxRadius - _config.particle.minRadius;
        const double particleMinRadius = _config.particle.minRadius;
        const double noiseYAxis = _time / _config.period;
        const double noiseMin = _config.particle.minRadiusNoiseValue;
        const double noiseTurbulence = _config.turbulence;
        const double rNoiseRange = 1.0 / (1.0 - noiseMin);
        const double returnForce = _config.returnForce;
        const double deltaT2 = timeState.deltaT * timeState.deltaT;
        const dvec2 origin = _config.origin;
        cpBB bounds = cpBBInvalid;
        auto physics = _physics.begin();
        auto state = _state.begin();
        const auto end = _state.end();

        for (; state != end; ++state, ++physics, a += da) {

            //
            // simplistic verlet integration
            //

            physics->velocity = (physics->position - physics->previous_position) * physics->damping;
            physics->previous_position = physics->position;

            dvec2 acceleration = (physics->home - physics->position) * returnForce;
            physics->position += physics->velocity + (acceleration * deltaT2);

            //
            // move position back to circle
            //

            dvec2 dirToPosition = physics->position - origin;
            double d2 = lengthSquared(dirToPosition);
            if (d2 < cloudLayerRadius2 - 1e-1 || d2 > cloudLayerRadius2 + 1e-1) {
                dirToPosition /= sqrt(d2);
                physics->position = origin + cloudLayerRadius * dirToPosition;
            }

            //
            //	now update position scale and rotation
            //

            state->position = physics->position;

            double angle;
            if (lengthSquared(physics->position - physics->home) > 1e-2) {
                angle = atan2(physics->position.y, physics->position.x) - M_PI_2;
            } else {
                angle = a - M_PI_2;
            }

            double fbm = _noise.fBm(a * noiseTurbulence, noiseYAxis);
            double noise = (fbm + 1.0) * 0.5;
            double radius = 0;
            if (noise > noiseMin) {
                double remappedNoiseVale = (noise - noiseMin) * rNoiseRange;
                radius = particleMinRadius + remappedNoiseVale * particleRadiusDelta;
            }
            physics->radius = lrp(timeState.deltaT, physics->radius, radius);

            //
            //	Compute the rotation axes
            //

            double cosa, sina;
            __sincos(angle, &sina, &cosa);
            state->right = physics->radius * dvec2(cosa, sina);
            state->up = rotateCCW(state->right);

            //
            // we're using alpha as a flag to say this particle should or should not be drawn
            //

            state->color.a = physics->radius > 1e-2 ? 1 : 0;

            //
            //	Update bounds
            //

            bounds = cpBBExpand(bounds, state->position, physics->radius);
        }

        _bb = bounds;
    }

    void CloudLayerParticleSimulation::pruneDisplacements() {
        _displacements.erase(std::remove_if(_displacements.begin(), _displacements.end(), [](const GravitationCalculatorRef &g) {
            return g->isFinished();
        }), _displacements.end());
    }

    void CloudLayerParticleSimulation::applyGravityDisplacements(const time_state &timeState) {
        for (const auto &g : _displacements) {
            dvec2 centerOfMass = g->getCenterOfMass();
            double magnitude = -1 * g->getMagnitude() * timeState.deltaT * _config.displacementForce;
            for (auto physics = _physics.begin(), end = _physics.end(); physics != end; ++physics) {
                dvec2 dir = physics->position - centerOfMass;
                double d2 = length2(dir);
                physics->position += magnitude * dir / d2;
            }
        }
    }
    
#pragma mark - CloudLayerParticleSystemDrawComponent
    
    CloudLayerParticleSystemDrawComponent::CloudLayerParticleSystemDrawComponent(config c, ColorA particleColor):
            ParticleSystemDrawComponent(c)
    {
        auto clearColor = ColorA(particleColor, 0);
        auto stack = make_shared<FilterStack>();
        setFilterStack(stack, clearColor);
        
        auto compositor = util::loadGlslAsset("kessler/filters/cloudlayer_compositor.glsl");
        compositor->uniform("Alpha", particleColor.a);
        stack->setScreenCompositeShader(compositor);
    }
    
    gl::GlslProgRef CloudLayerParticleSystemDrawComponent::createDefaultShader() const {
        return util::loadGlslAsset("kessler/shaders/cloudlayer.glsl");
    }
    
    void CloudLayerParticleSystemDrawComponent::setShaderUniforms(const gl::GlslProgRef &program, const core::render_state &renderState) {
        ParticleSystemDrawComponent::setShaderUniforms(program, renderState);
    }


#pragma mark - CloudLayerParticleSystem
    

    CloudLayerParticleSystem::config CloudLayerParticleSystem::config::parse(const XmlTree &node) {
        config c;
        c.drawConfig.drawLayer = DrawLayers::EFFECTS;
        c.drawConfig = ParticleSystemDrawComponent::config::parse(node.getChild("draw"));
        c.simulationConfig = CloudLayerParticleSimulation::config::parse(node.getChild("simulation"));
        return c;
    }

    CloudLayerParticleSystemRef CloudLayerParticleSystem::create(config c) {

        // alpha for cloud layer is applied in the compositor pass,
        // so set alpha to 1 during initial rendering; and pass actual
        // color to the draw component so it knows how to composite correctly

        const auto particleColor = c.simulationConfig.particle.color;
        c.simulationConfig.particle.color.a = 1;
        
        auto simulation = make_shared<CloudLayerParticleSimulation>(c.simulationConfig);
        auto draw = make_shared<CloudLayerParticleSystemDrawComponent>(c.drawConfig, particleColor);

        return Object::create<CloudLayerParticleSystem>("CloudLayer", {draw, simulation});
    }

    CloudLayerParticleSystem::CloudLayerParticleSystem(string name) :
            BaseParticleSystem(name) {
    }


}
