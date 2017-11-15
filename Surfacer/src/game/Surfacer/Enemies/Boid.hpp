
//
//  Boid.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/17.
//
//

#ifndef Boid_hpp
#define Boid_hpp

#include <cinder/Rand.h>
#include <cinder/Xml.h>

#include "Core.hpp"
#include "Entity.hpp"
#include "SurfacerStage.hpp"
#include "Xml.hpp"

namespace surfacer { namespace enemy {

	SMART_PTR(BoidPhysicsComponent);
	SMART_PTR(BoidDrawComponent);
	SMART_PTR(Boid);
	SMART_PTR(BoidFlockDrawComponent);
	SMART_PTR(BoidFlockController);
	SMART_PTR(BoidFlock);

#pragma mark - BoidPhysicsComponent

	class BoidPhysicsComponent : public core::PhysicsComponent {
	public:

		struct config {
			dvec2 position;
			double radius;
			double sensorRadius;
			double speed;
			double density;
		};

	public:

		BoidPhysicsComponent(config c, double ruleVariance, cpGroup group);
		virtual ~BoidPhysicsComponent();

		const config &getConfig() const { return _config; }

		dvec2 getPosition() const { return _position; }
		dvec2 getVelocity() const { return _velocity; }
		dvec2 getRotation() const { return _rotation; }
		double getRadius() const { return _config.radius; }
		double getSensorRadius() const { return _config.sensorRadius; }
		double getRuleVariance() const { return _ruleVariance; }

		cpShape *getShape() const { return _shape; }

		void setTargetVelocity(dvec2 vel);
		void addToTargetVelocity(dvec2 vel);
		dvec2 getTargetVelocity() const { return _targetVelocity; }

		void setFacingDirection(dvec2 dir);
		dvec2 getFacingDirection() const { return _facingDirection; }

		void onReady(core::ObjectRef parent, core::StageRef stage) override;
		void onCleanup() override;
		void step(const core::time_state &time) override;
		cpBB getBB() const override;
		size_t getGravitationLayerMask(cpBody*) const override { return 0; } // not affected by ANY gravities

	protected:

		config _config;
		cpGroup _group;
		cpBody *_body;
		cpConstraint *_gear;
		cpShape *_shape;
		dvec2 _targetVelocity, _position, _velocity, _rotation, _facingDirection;
		double _mass, _ruleVariance;

	};

#pragma mark - Boid

	class Boid : public core::Entity {
	public:

		struct config {
			BoidPhysicsComponent::config physics;
			core::HealthComponent::config health;
		};

		static BoidRef create(string name, BoidFlockControllerRef flockController, config c, dvec2 initialPosition, dvec2 initialVelocity, double ruleVariance, cpGroup group);

	public:

		Boid(string name, BoidFlockControllerRef flockController, config c);
		virtual ~Boid();

		const config &getConfig() const { return _config; }
		double getRuleVariance() const { return _boidPhysics->getRuleVariance(); }

		BoidPhysicsComponentRef getBoidPhysicsComponent() const { return _boidPhysics; }
		BoidFlockControllerRef getFlockController() const { return _flockController.lock(); }

		// Entity
		void onHealthChanged(double oldHealth, double newHealth) override;
		void onDeath() override;

		// Object
		void onReady(core::StageRef stage) override;
		void onCleanup() override;

	private:

		config _config;
		BoidFlockControllerWeakRef _flockController;
		BoidPhysicsComponentRef _boidPhysics;

	};

#pragma mark - BoidDrawComponent

	class BoidFlockDrawComponent : public core::DrawComponent {
	public:
		struct config {
		};

	public:

		BoidFlockDrawComponent(config c);
		virtual ~BoidFlockDrawComponent();

		void onReady(core::ObjectRef parent, core::StageRef stage) override;

		void update(const core::time_state &time) override;
		void draw(const core::render_state &renderState) override;
		core::VisibilityDetermination::style getVisibilityDetermination() const override { return core::VisibilityDetermination::FRUSTUM_CULLING; }
		int getLayer() const override { return DrawLayers::ENEMY; }
		int getDrawPasses() const override { return 1; }
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }
		cpBB getBB() const override;

	protected:

		config _config;
		BoidFlockControllerWeakRef _flockController;
		gl::VboMeshRef _unitCircleMesh;
		
	};


#pragma mark - BoidFlockController

	class BoidFlockController : public core::Component {
	public:

		struct rule_contributions {
			// scale of rule causing flock to converge about centroid
			double flockCentroid;

			// scale of rule causing flock to attempt to maintain a shared velocity
			double flockVelocity;

			// scale of rule causing flock to avoid collisions with eachother, environment
			double collisionAvoidance;

			// scale of rule causing flock to seek target
			double targetSeeking;

			// each boid gets a random variance of its rule scaling. ruleVariance of zero means each boid
			// follows the rules precisely the same. a variance of 0.5 means each boid gets a scaling of (1 + rand(-0.5,0.5))
			// meaning some boids will follow the rules 50% less, and some by 50% more.
			double ruleVariance;

			rule_contributions():
			flockCentroid(0.1),
			flockVelocity(0.1),
			collisionAvoidance(0.1),
			targetSeeking(0.1),
			ruleVariance(0.25)
			{}
		};

		struct config {
			Boid::config boid;
			rule_contributions ruleContributions;
			core::seconds_t trackingMemorySeconds;
			vector<string> target_ids;
		};

		struct tracking_state {
			// the Boid who saw the target
			BoidRef eyeBoid;

			// the current location of the boid who saw the target
			dvec2 eyeBoidPosition;

			// the target
			core::ObjectRef target;

			// the current location of the target
			dvec2 targetPosition;

			// true if the target was visible
			bool targetVisible;
			core::seconds_t lastTargetVisibleTime;

			tracking_state():
			eyeBoidPosition(0),
			targetPosition(0),
			targetVisible(false),
			lastTargetVisibleTime(0)
			{}
		};

	public:

		// signal fired when all boids in the flock are gone
		core::signals::signal< void(BoidFlockControllerRef) > onFlockDidFinish;

		// signal fired when a boid in the flock is killed right before it's removed from the stage
		core::signals::signal< void(BoidFlockControllerRef, BoidRef) > onBoidFinished;

	public:

		/**
		 Create a BoidFlockController which will prefix Boids with a given name, and which will use a given configuration
		 to apply rule_contributions while computing flock updates, and template params for each boid.
		 */
		BoidFlockController(string name, config c);
		virtual ~BoidFlockController();

		/**
		Spawn `count boids from `origin in `initialDirection with a given `config
		*/
		void spawn(size_t count, dvec2 origin, dvec2 initialDirection);

		const vector<BoidRef> &getFlock() const { return _flock; }

		/**
		Get the number of living boids in the flock
		*/
		size_t getFlockSize() const;

		/**
		 Get the name that will be prefixed to Boids
		 */
		string getName() const { return _name; }

		/**
		 Assign a new set of rule contributions overriding those assigned in the constructor
		 */
		void setRuleContributions(rule_contributions rc) { _config.ruleContributions = rc; }

		/**
		 Get the rule contributions which will be used to drive the flock
		 */
		rule_contributions getRuleContributions() const { return _config.ruleContributions; }

		/**
		 Set the targets this Boid flock will pursue.
		 Internally, the targets are held as weak_ptr<> and the flock will pursue the first which is
		 live, is in the stage, and has a PhysicsRepresentation to query for position.
		 */
		void setTargets(vector<core::ObjectRef> targets);

		/**
		 Add a target for the flock to pursue
		 */
		void addTarget(core::ObjectRef target);

		/**
		 Clear the targets this flock will pursue. Flock will continue to follow flcoking rules, but without target seeking
		 */
		void clearTargets();

		const vector<core::ObjectWeakRef> getTargets() const { return _targets; }

		/**
		 Get the current tracking state used to direct the flock
		 */
		const tracking_state &getTrackingState() const { return _trackingState; }

		/**
		 Get the bounds of the flock
		 */
		cpBB getFlockBB() const { return _flockBB; }

		// Component
		void onReady(core::ObjectRef parent, core::StageRef stage) override;
		void update(const core::time_state &time) override;

	protected:

		friend class Boid;
		friend class BoidFlockDrawComponent;

		void _updateTrackingState(const core::time_state &time);
		boost::optional<pair<cpBB,dvec2>> _getObjectPosition(const core::ObjectRef &obj) const;
		bool _checkLineOfSight(dvec2 start, dvec2 end, core::ObjectRef target) const;

		void _updateFlock_canonical(const core::time_state &time);

		void _onBoidFinished(const BoidRef &boid);

	protected:

		cpSpace *_space;
		cpGroup _group;
		string _name;
		vector<BoidRef> _flock;
		vector<core::ObjectWeakRef> _targets;
		dvec2 _lastSpawnOrigin;
		config _config;
		ci::Rand _rng;
		cpBB _flockBB;
		tracking_state _trackingState;
		dvec2 _swarmTargetOffset;
		core::seconds_t _nextSwarmTargetOffsetUpdate;

		// raw ptr for performance - profiling shows 75% of update() loops are wasted on shared_ptr<> refcounting
		vector<BoidPhysicsComponent*> _flockPhysicsComponents;

	};

#pragma mark - BoidFlock

	/**
	 BoidFlock is a Object which acts as a controller for a flock of boids.
	 Generally speaking, since BoidFlock is just a Object and not an Entity, it's not
	 a thing which can be drawn or have health or be shot. Rather, an "owner" Entity, say
	 the Eggsac, creates a BoidFlock and adds it to the Stage.
	 */
	class BoidFlock : public core::Object {
	public:

		struct config {
			BoidFlockController::config controller;
			BoidFlockDrawComponent::config draw;
		};

		static config loadConfig(core::util::xml::XmlMultiTree flockNode);

		static BoidFlockRef create(string name, config c);

	public:

		BoidFlock(string name, BoidFlockControllerRef controller, BoidFlockDrawComponentRef drawer);
		virtual ~BoidFlock();

		BoidFlockControllerRef getBoidFlockController() const { return _flockController; }
		BoidFlockDrawComponentRef getBoidDrawComponent() const { return _drawer; }

	private:

		BoidFlockControllerRef _flockController;
		BoidFlockDrawComponentRef _drawer;

	};


}} // end namespace surfacer::enemy

#endif /* Boid_hpp */
