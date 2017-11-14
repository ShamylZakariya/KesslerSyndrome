//
//  ParticleSystem.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/23/17.
//

#ifndef ParticleSystem_hpp
#define ParticleSystem_hpp

#include <cinder/Rand.h>

#include "BaseParticleSystem.hpp"

namespace particles {
	
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
		
		interpolator()
		{}
		
		interpolator(const initializer_list<T> values):
		_values(values)
		{
			CI_ASSERT_MSG(!_values.empty(), "Values array must not be empty");
		}
		
		interpolator(const interpolator<T> &copy):
		_values(copy._values)
		{
			CI_ASSERT_MSG(!_values.empty(), "Values array must not be empty");
		}
		
		interpolator &operator = (const interpolator<T> &rhs) {
			_values = rhs._values;
			return *this;
		}
		
		interpolator &operator = (const initializer_list<T> values) {
			_values = values;
			CI_ASSERT_MSG(!_values.empty(), "Values array must not be empty");
			return *this;
		}
		
		interpolator &operator = (const T &value) {
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
		
	private:
		
		vector<T> _values;
		
	};
	
	
	struct particle_prototype {
	public:
		
		struct kinematics_prototype {
			bool isKinematic;
			double friction;
			cpShapeFilter filter;
			
			kinematics_prototype():
			isKinematic(false),
			friction(1),
			filter(CP_SHAPE_FILTER_ALL)
			{}
			
			kinematics_prototype(double friction, cpShapeFilter filter):
			isKinematic(true),
			friction(friction),
			filter(filter)
			{}
			
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
		interpolator<ci::ColorA> color;
		
		// initial position of particle. as particle is simulated position will change
		dvec2 position;
		
		// initial velocity to apply to particle. particle motion will be ballistic after this
		dvec2 velocity;
		
		// if true, particle is rotated such that the X axis aligns with the direction of velocity
		bool orientToVelocity;
		
		kinematics_prototype kinematics;
		
	private:
		
		friend class ParticleSimulation;
		
		cpShape* _shape;
		cpBody* _body;
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
		
		particle_prototype():
		atlasIdx(0),
		lifespan(0),
		position(0,0),
		velocity(0,0),
		orientToVelocity(false),
		_shape(nullptr),
		_body(nullptr),
		_completion(0),
		_age(0)
		{}
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
		void emit(const particle_prototype &particle);
		void emit(const particle_prototype &particle, const dvec2 &world, const dvec2 &vel);

	protected:
		
		virtual void _prepareForSimulation(const core::time_state &time);
		virtual void _simulate(const core::time_state &time);
		
	protected:
		
		size_t _count;
		cpBB _bb;
		vector<particle_prototype> _prototypes, _pending;
		core::SpaceAccessRef _spaceAccess;
		
	};
	
	class ParticleEmitter : public core::Component {
	public:
		
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
		 Seed the RNG that purturbs particle emission state
		 */
		void seed(uint32_t seed);
		
		/**
		 Add a template to the emission library. The higher probability is relative to other templates
		 the more often this template will be emitted.
		 */
		void add(const particle_prototype &prototype, float variance, int probability = 1);
		
		/**
		 Create an emission for a circular volume in the world that lasts `duration seconds and has an
		 emission rate following the specified envelope. Will emit at max envolope `rate particles per second.
		 Returns an id usable to cancel the emission via cancel()
		 */
		emission_id emit(dvec2 world, double radius, dvec2 vel, seconds_t duration, double rate, Envelope env);
		
		/**
		 Create an emission for a circular volume in the world that lasts `duration seconds and has an
		 emission rate following the specified envelope. Will emit at max envolope `rate particles per second.
		 Returns an id usable to cancel the emission via cancel()
		 */
		emission_id emit(dvec2 world, double radius, dvec2 vel, seconds_t duration, double rate, const interpolator<double> &envelope);
		
		/**
		 Create an emission for a circular volume in the world at a given rate. It will run until cancel() is called on the returned id.
		 Returns an id usable to cancel the emission via cancel()
		 */
		emission_id emit(dvec2 world, double radius, dvec2 vel, double rate);
		
		/**
		 Update the position, radius and velocity of an active emission
		 */
		void setEmissionPosition(emission_id emission, dvec2 world, double radius, dvec2 vel);
		
		/**
		 Cancels a running emission. Pass the id returned by one of the emit() methods
		*/
		void cancel(emission_id emissionId);
		
		/**
		 Emit `count particles in the circular volume of `radius about `world
		 */
		void emitBurst(dvec2 world, double radius, dvec2 vel, int count = 1);


	private:
		
		double nextDouble(double variance);
		dvec2 nextDVec2(double variance);		
		dvec2 perturb(const dvec2 dir, double variance);
		
		struct emission_prototype {
			particle_prototype prototype;
			float variance;
		};
		
		struct emission {
			emission_id id;
			seconds_t startTime;
			seconds_t endTime;
			seconds_t secondsPerEmission;
			seconds_t secondsAccumulator;
			dvec2 world;
			dvec2 velocity;
			double radius;			
			interpolator<double> envelope;
		};
		
		ParticleSimulationWeakRef _simulation;
		ci::Rand _rng;
		map<emission_id, emission> _emissions;
		vector<emission_prototype> _prototypes;
		vector<size_t> _prototypeLookup;
		
	};

	
	class ParticleSystemDrawComponent : public particles::BaseParticleSystemDrawComponent {
	public:
		
		struct config : public particles::BaseParticleSystemDrawComponent::config {
			ci::gl::Texture2dRef textureAtlas;
			particles::Atlas::Type atlasType;

			config():
			atlasType(particles::Atlas::None)
			{}
			
			config(const particles::BaseParticleSystemDrawComponent::config &c):
			particles::BaseParticleSystemDrawComponent::config(c),
			atlasType(particles::Atlas::None)
			{}
			
			static config parse(const core::util::xml::XmlMultiTree &node);
		};
		
	public:
		
		ParticleSystemDrawComponent(config c);
		
		// BaseParticleSystemDrawComponent
		void setSimulation(const particles::BaseParticleSimulationRef simulation) override;
		void draw(const core::render_state &renderState) override;
		
	protected:
		
		// update _particles store for submitting to GPU - return true iff there are particles to draw
		virtual bool updateParticles(const BaseParticleSimulationRef &sim);
		
		struct particle_vertex {
			vec2 position;
			vec2 texCoord;
			ci::ColorA color;
		};
		
		config _config;
		gl::GlslProgRef _shader;
		vector<particle_vertex> _particles;
		gl::VboRef _particlesVbo;
		gl::BatchRef _particlesBatch;
		GLsizei _batchDrawStart, _batchDrawCount;
		
	};
	
	
	class ParticleSystem : public particles::BaseParticleSystem {
	public:
		
		struct config {
			size_t maxParticleCount;
			particles::ParticleSystemDrawComponent::config drawConfig;
			
			config():
			maxParticleCount(500)
			{}
			
			static config parse(const core::util::xml::XmlMultiTree &node);
		};
		
		static ParticleSystemRef create(std::string name, const config &c);
		
	public:
		
		// do not call this, call ::create
		ParticleSystem(std::string name);
		
		ParticleEmitterRef createEmitter();
		
	};
	
	
}

#endif /* ParticleSystem_hpp */
