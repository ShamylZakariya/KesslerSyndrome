//
//  ParticleSystem.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/23/17.
//

#ifndef ParticleSystem_hpp
#define ParticleSystem_hpp

#include <cinder/Rand.h>

#include "elements/ParticleSystem/BaseParticleSystem.hpp"

namespace elements {

    SMART_PTR(ParticleSystem);

    SMART_PTR(ParticleSimulation);

    SMART_PTR(ParticleSystemDrawComponent);

    SMART_PTR(ParticleEmitter);

    using core::seconds_t;

    /**
     Interpolator
     A keyframe-type interpolation but where the values are evenly spaced.
     If you provide the values, [0,1,0], you'd get a mapping like so:
     0.00 -> 0
     0.25 -> 0.5
     0.50 -> 1
     0.75 -> 0.5
     1.00 -> 0
     */
    template<class T>
    struct interpolator {
    public:

        interpolator() {
        }

        interpolator(const initializer_list <T> values) :
                _values(values) {
            CI_ASSERT_MSG(!_values.empty(), "Values array must not be empty");
        }

        interpolator(const interpolator<T> &copy) :
                _values(copy._values) {
            CI_ASSERT_MSG(!_values.empty(), "Values array must not be empty");
        }

        interpolator &operator=(const interpolator<T> &rhs) {
            _values = rhs._values;
            return *this;
        }

        interpolator &operator=(const initializer_list <T> values) {
            _values = values;
            CI_ASSERT_MSG(!_values.empty(), "Values array must not be empty");
            return *this;
        }

        interpolator &operator=(const T &value) {
            _values.clear();
            _values.push_back(value);
            return *this;
        }

        // get the value equivalent to *this(0)
        T getInitialValue() const {
            return _values.front();
        }

        // get the value equivalent to *this(1)
        T getFinalValue() const {
            return _values.back();
        }

        // get the interpolated value for a given time `v` from [0 to 1]
        T operator()(double v) const {
            if (_values.size() == 1) {
                return _values.front();
            }

            if (v <= 0) {
                return _values.front();
            } else if (v >= 1) {
                return _values.back();
            }

            size_t s = _values.size() - 1;
            double dist = v * s;
            double distFloor = floor(dist);
            double distCeil = distFloor + 1;
            size_t aIdx = static_cast<size_t>(distFloor);
            size_t bIdx = aIdx + 1;

            T a = _values[aIdx];
            T b = _values[bIdx];
            double factor = (dist - distFloor) / (distCeil - distFloor);

            return a + (factor * (b - a));
        }

        vector <T> _values;

    };


    struct particle_prototype {
    public:

        struct kinematics_prototype {
            bool isKinematic;
            double friction;
            double elasticity;
            double scale;
            cpShapeFilter filter;

            kinematics_prototype() :
                    isKinematic(false),
                    friction(1),
                    elasticity(0.5),
                    scale(1),
                    filter(CP_SHAPE_FILTER_ALL) {
            }

            kinematics_prototype(double friction, double elasticity, cpShapeFilter filter) :
                    isKinematic(true),
                    friction(friction),
                    elasticity(elasticity),
                    scale(1),
                    filter(filter) {
            }

            kinematics_prototype(double friction, double elasticity, double scale, cpShapeFilter filter) :
                    isKinematic(true),
                    friction(friction),
                    elasticity(elasticity),
                    scale(scale),
                    filter(filter) {
            }

            operator bool() const {
                return isKinematic == true;
            }
        };


    public:

        // index into texture atlas for this particle
        int atlasIdx;

        // lifespan in seconds of the particle
        seconds_t lifespan;

        // radius interpolator
        interpolator<double> radius;

        // damping interpolator. damping value > 0 subtracts that amount from velocity per timestamp
        interpolator<double> damping;

        // additivity interpolator
        interpolator<double> additivity;

        // mass interpolator. If non-zero particles are affected by gravity.
        // values > 0 are attracted to gravity wells and values < 0 are repelled.
        interpolator<double> mass;

        // color interpolator
        interpolator<ColorA> color;

        // initial position of particle. as particle is simulated position will change
        dvec2 position;

        // initial velocity of particle motion
        double initialVelocity;

        // if true, particle is rotated such that the X axis aligns with the direction of velocity
        bool orientToVelocity;

        // if > 0, represents the minimum velocity for scale of particle to be normal. if the particle
        // velocity falls beneath this value, the size is scaled proportionally down to zero.
        // this is a way to hide spark effects when they're immobile.
        double minVelocity;

        // mask describing which gravitation calculators to apply to ballistic (non-kinematic) particles
        // the layer mask for kinematic particles can only be set
        // via the parent ParticleSystem::config::kinematicParticleGravitationLayerMask field
        size_t gravitationLayerMask;

        kinematics_prototype kinematics;

    private:

        friend class ParticleSimulation;

        cpShape *_shape;
        cpBody *_body;
        dvec2 _velocity;
        double _completion;
        seconds_t _age;

        void destroy() {
            if (_shape) {
                cpCleanupAndFree(_shape);
                _shape = nullptr;
            }
            if (_body) {
                cpCleanupAndFree(_body);
                _body = nullptr;
            }
        }

        void prepare() {
            _age = 0;
            _completion = 0;
        }

    public:

        particle_prototype() :
                atlasIdx(0),
                lifespan(0),
                position(0, 0),
                initialVelocity(0),
                orientToVelocity(false),
                minVelocity(0),
                gravitationLayerMask(core::ALL_GRAVITATION_LAYERS),
                _shape(nullptr),
                _body(nullptr),
                _velocity(0, 0),
                _completion(0),
                _age(0) {
        }
    };

    class ParticleSimulation : public BaseParticleSimulation {
    public:

        ParticleSimulation();

        // Component
        void onReady(core::ObjectRef parent, core::StageRef stage) override;

        void onCleanup() override;

        void update(const core::time_state &time) override;

        // BaseParticleSimulation
        void setParticleCount(size_t count) override;

        size_t getFirstActive() const override;

        size_t getActiveCount() const override;

        cpBB getBB() const override;

        // ParticleSimulation

        // emit a single particle
        void emit(const particle_prototype &particle, const dvec2 &world, const dvec2 &dir);

        // if true, ParticleSimulation will when necessary sort the active particles by age
        // with oldest being drawn first. Only turn this on if you see periodic "pops" where
        // particle ordering appears to invert. This can happen when storage is compacted and
        // when particles overflow particleCount and round-robin to the beginning
        void setShouldKeepSorted(bool keepSorted) {
            _keepSorted = keepSorted;
        }

        bool shouldKeepSorted() const {
            return _keepSorted;
        }

    protected:
        
        virtual void _prepareForSimulation(const core::time_state &time);

        virtual void _simulate(const core::time_state &time);

    protected:

        size_t _count;
        cpBB _bb;
        vector <particle_prototype> _prototypes, _pending;
        core::SpaceAccessRef _spaceAccess;
        bool _keepSorted;

    };

    class ParticleEmitter : public core::Component {
    public:

        struct Source {

            // max distance from emission point; particles will be emitted from
            // a circular volume of this radius
            double radius;

            // max spread from the emission direction. A value of zero
            // results in a laser-like emission, a value of 0.25 is a cone with 90° tip.
            // A value of 1 is a full circular emission regardless of direction.
            double spread;

            // amount of per-particle randomization variance. A value of 0 means no
            // change per-particle, a value of 0.5 means each value in the particle will
            // be multiplied by a random value between [1-0.5, 1+0.5], and so on.
            double variance;

            Source(double radius, double spread, double variance) :
                    radius(max(radius, 0.0)),
                    spread(clamp(spread, 0.0, 1.0)),
                    variance(clamp(variance, 0.0, 1.0)) {
            }

            void apply(Rand &rng, const dvec2 &world, const dvec2 &normalizedDirOrZero, dvec2 &outWorld, dvec2 &outDir) const;

        };

        enum Envelope {
            // emission ramps up from 0 to full rate across lifetime
                    RampUp,

            // emission ramps from 0 to full rate at half of lifetime and back to zero at end
                    Sawtooth,

            // emission quickly ramps from 0 to full rate, continues at full rate, and then quickly back to zero at end of lifetime
                    Mesa,

            // emission starts at full rate and ramps down to zero across lifetime
                    RampDown
        };

        typedef size_t emission_id;

    public:

        ParticleEmitter(uint32_t seed = 12345);

        // Component
        void update(const core::time_state &time) override;

        void onReady(core::ObjectRef parent, core::StageRef stage) override;

        // ParticleEmitter

        void setSimulation(const ParticleSimulationRef simulation);

        ParticleSimulationRef getSimulation() const;
        
        /**
         Set the velocity of the emitter - if non-zero, the velocity will be added to emitted particles
         */
        void setVelocity(dvec2 emitterVelocity) { _velocity = emitterVelocity; }
        dvec2 getVelocity() const { return _velocity; }

        /**
         Seed the RNG that purturbs particle emission state
         */
        void seed(uint32_t seed);

        /**
         Add a template to the emission library. The higher probability is relative to other templates
         the more often this template will be emitted.
         */
        void add(const particle_prototype &prototype, Source source, int probability = 1);

        /**
         Create an emission in a diven direction from a circular volume in the world that lasts `duration seconds and has an
         emission rate following the specified envelope. Will emit at max envolope `rate particles per second.
         Returns an id usable to cancel the emission via cancel()
         */
        emission_id emit(dvec2 world, dvec2 normalizedDirOrZero, seconds_t duration, double rate, Envelope env);

        /**
         Create an emission in a given direction from a circular volume in the world that lasts `duration seconds and has an
         emission rate following the specified envelope. Will emit at max envolope `rate particles per second.
         Returns an id usable to cancel the emission via cancel()
         */
        emission_id emit(dvec2 world, dvec2 normalizedDirOrZero, seconds_t duration, double rate, const interpolator<double> &envelope);

        /**
         Create an emission in a given direction for a circular volume in the world at a given rate. It will run until cancel() is called on the returned id.
         Returns an id usable to cancel the emission via cancel()
         */
        emission_id emit(dvec2 world, dvec2 normalizedDirOrZero, double rate);

        /**
         Update the position, radius and emission direction of an active emission
         */
        void setEmissionPosition(emission_id emission, dvec2 world, dvec2 dir);

        /**
         Cancels a running emission. Pass the id returned by one of the emit() methods
        */
        void cancel(emission_id emissionId);

        /**
         Emit a single-shot burst of particles.
         THe particles are emitted at `world, with an initial velocity direction of normalizedDirOrZero.
         `count particles are emitted, with each having to pass a dice-roll for probability.
         The particle emitted will be selected from the assigned prototypes, and the Source will be applied to the
         emission volume and direction, just like any other emission.
         */
        void emitBurst(dvec2 world, dvec2 normalizedDirOrZero, int count = 1, float probability=1);

    private:

        struct emission_prototype {
            particle_prototype prototype;
            Source source;
        };

        struct emission {
            emission_id id;
            seconds_t startTime;
            seconds_t endTime;
            seconds_t secondsPerEmission;
            seconds_t secondsAccumulator;
            dvec2 world;
            dvec2 dir;
            interpolator<double> envelope;
        };

        ParticleSimulationWeakRef _simulation;
        Rand _rng;
        map <emission_id, emission> _emissions;
        vector <emission_prototype> _prototypes;
        vector <size_t> _prototypeLookup;
        dvec2 _velocity;

    };


    class ParticleSystemDrawComponent : public elements::BaseParticleSystemDrawComponent {
    public:

        struct config : public elements::BaseParticleSystemDrawComponent::config {
            gl::Texture2dRef textureAtlas;
            elements::Atlas::Type atlasType;
            gl::GlslProgRef shader;

            config() :
                    atlasType(elements::Atlas::None) {
            }

            config(const elements::BaseParticleSystemDrawComponent::config &c) :
                    elements::BaseParticleSystemDrawComponent::config(c),
                    atlasType(elements::Atlas::None) {
            }

            static config parse(const XmlTree &node);
        };

    public:

        ParticleSystemDrawComponent(config c);

        // BaseParticleSystemDrawComponent
        void setSimulation(const elements::BaseParticleSimulationRef simulation) override;

        void draw(const core::render_state &renderState) override;
        
        // ParticleSystemDrawComponent
        
        /**
         Assign a FilterStack. null by default. When non-null, all rendering in draw() will be performed to an Fbo,
         and then after the filters in the stack are executed, the result will be composited to screen.
         @param stack The filter stack to assign
         @param clearColor The color the capture Fbo will be cleared to
         */
        virtual void setFilterStack(const core::FilterStackRef &stack, ColorA clearColor) {
            _filterStack = stack;
            _filterStackClearColor = clearColor;
        }
        
        const core::FilterStackRef &getFilterStack() const { return _filterStack; }
        ColorA getFilterStackClearColor() const { return _filterStackClearColor; }

    protected:
        
        virtual void performDraw(const core::render_state &state);
        
        virtual gl::GlslProgRef createDefaultShader() const;
        
        // write stable values into _particles - these are values that can be written once and never change
        void writeStableParticleValues(const BaseParticleSimulationRef &sim);

        // update _particles store for submitting to GPU - return true iff there are particles to draw
        virtual bool updateParticles(const BaseParticleSimulationRef &sim);
        
        // called immediately before particle batch is drawn; set your shader uniforms here
        virtual void setShaderUniforms(const gl::GlslProgRef &program, const core::render_state &renderState) {}

        struct particle_vertex {
            // position of vertex
            // bound to ciPosition (float2)
            vec2 position;

            // world-space up vector of particle, scaled such that length = particle radius
            // bound to ciNormal (float2)
            vec2 up;

            // tex coord for vertex
            // bound to ciTexCoord0 (float2)
            vec2 texCoord;
            
            // top-left vertex would be (0,1), top right (1,1), bottom left (0,0), bottom right (1,0)
            // bound to ciTexCoord1 (float2)
            vec2 vertexPosition;

            // 2 random values from [-1,+1]; usable to customize a
            // particle. Each vertex of a single particle will share same random value
            // and that value will be unchanging across particle lifespan
            // bound to ciTexCoord2 (float2) in shader
            vec2 random;

            // color for the vertex
            // bound to ciColor (float4)
            ColorA color;
            
            // atlas index of this particle [0..4]
            // bound to ciBoneIndex
            int atlasIdx;
        };
        
    protected:

        config _config;
        gl::GlslProgRef _shader;
        vector <particle_vertex> _particles;
        gl::VboRef _particlesVbo;
        gl::BatchRef _particlesBatch;
        GLsizei _batchDrawStart, _batchDrawCount;
        core::FilterStackRef _filterStack;
        ColorA _filterStackClearColor;

    };


    class ParticleSystem : public elements::BaseParticleSystem {
    public:

        struct config {
            size_t maxParticleCount;
            bool keepSorted;
            size_t kinematicParticleGravitationLayerMask;
            elements::ParticleSystemDrawComponent::config drawConfig;

            config() :
                    maxParticleCount(500),
                    keepSorted(false),
                    kinematicParticleGravitationLayerMask(core::ALL_GRAVITATION_LAYERS) {
            }

            static config parse(const XmlTree &node);
        };

        static ParticleSystemRef create(std::string name, const config &c);

    public:

        // do not call this, call ::create
        ParticleSystem(std::string name, const config &c);


        // ParticleSystem
        ParticleEmitterRef createEmitter();

        size_t getGravitationLayerMask(cpBody *body) const override;

    private:

        config _config;

    };


} // end namespace elements

#endif /* ParticleSystem_hpp */
