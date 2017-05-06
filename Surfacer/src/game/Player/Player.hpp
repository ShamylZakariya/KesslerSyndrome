//
//  Player.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/23/17.
//
//

#ifndef Player_hpp
#define Player_hpp

#include <cinder/Xml.h>

#include "Core.hpp"
#include "DevComponents.hpp"

namespace player {

	SMART_PTR(Player);
	SMART_PTR(PlayerGunComponent);
	SMART_PTR(PlayerPhysicsComponent);
	SMART_PTR(JetpackUnicyclePlayerPhysicsComponent);
	SMART_PTR(PlayerDrawComponent);
	SMART_PTR(PlayerInputComponent);
	SMART_PTR(BeamProjectileComponent);

#pragma mark - BeamProjectileComponent

	class BeamProjectileComponent : public core::Component {
	public:

		struct contact {
			dvec2 position;
			dvec2 normal;
			core::GameObjectRef target;

			contact(const dvec2 &p, const dvec2 &n, const core::GameObjectRef &t):
			position(p),
			normal(n),
			target(t)
			{}
		};

		struct config {
			double range;
			double width;
			double length;
			double velocity;
			double cutDepth;

			config():
			range(0),
			width(0),
			length(0),
			velocity(0),
			cutDepth(0)
			{}
		};

		struct segment {
			dvec2 head, tail;
			double len;

			segment(){}

			segment(dvec2 h, dvec2 t, double l):
			head(h),
			tail(t),
			len(l)
			{}
		};

	public:

		BeamProjectileComponent(config c);
		BeamProjectileComponent(config c, dvec2 origin, dvec2 dir);
		virtual ~BeamProjectileComponent();

		void fire(dvec2 origin, dvec2 dir);
		dvec2 getOrigin() const { return _origin; }
		dvec2 getDirection() const { return _dir; }
		dvec2 getPosition() const { return _segment.head; }
		double getWidth() const;
		segment getSegment() const { return _segment; }

		// returns a vector of coordinates in world space representing the intersection with world geometry of the gun beam
		const vector<contact> &getContacts() const { return _contacts; }

		// Component
		void onReady(core::GameObjectRef parent, core::LevelRef level) override;
		void onCleanup() override;
		void update(const core::time_state &time) override;

	private:

		void updateContacts();

	private:

		config _config;
		dvec2 _origin, _dir;
		segment _segment, _cutToPerform;
		double _penetrationRemaining;
		bool _hasHit;
		core::seconds_t _lastDeltaT;
		vector<contact> _contacts;
		
	};

#pragma mark - BeamProjectileDrawComponent

	class BeamProjectileDrawComponent : public core::DrawComponent {
	public:

		BeamProjectileDrawComponent();
		virtual ~BeamProjectileDrawComponent();

		// Component
		void onReady(core::GameObjectRef parent, core::LevelRef level) override;

		// DrawComponent
		cpBB getBB() const override;
		void draw(const core::render_state &renderState) override;
		core::VisibilityDetermination::style getVisibilityDetermination() const override;
		int getLayer() const override;
		int getDrawPasses() const override;
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	private:

		BeamProjectileComponentWeakRef _projectile;

	};


#pragma mark - PlayerGunComponent

	class PlayerGunComponent : public core::Component {
	public:

		struct config {
			BeamProjectileComponent::config pulse;
			BeamProjectileComponent::config blast;
			double blastChargePerSecond;
		};

	public:

		PlayerGunComponent(config c);
		virtual ~PlayerGunComponent();

		void setShooting(bool shooting);
		bool isShooting() const { return _isShooting; }

		// get the current charge level [0,1]
		double getBlastChargeLevel() const { return _blastCharge; }

		// origin of gun beam in world space
		void setBeamOrigin(dvec2 origin) { _beamOrigin = origin; }
		dvec2 getBeamOrigin() const { return _beamOrigin; }

		// normalized direction of gun beam in world space
		void setBeamDirection(dvec2 dir) { _beamDir = dir; }
		dvec2 getBeamDirection() const { return _beamDir; }

		// Component
		void update(const core::time_state &time) override;

	private:

		void firePulse();
		void fireBlast();

	private:

		config _config;
		bool _isShooting;
		dvec2 _beamOrigin, _beamDir;
		double _blastCharge;
		core::seconds_t _pulseStartTime, _blastStartTime;

	};

#pragma mark - PlayerPhysicsComponent

	class PlayerPhysicsComponent : public core::PhysicsComponent {
	public:

		struct config {
			// initial position of player in world units
			dvec2 position;

			double width;
			double height;
			double density;
			double footFriction;
			double bodyFriction;

			double jetpackAntigravity;
			double jetpackFuelMax;
			double jetpackFuelConsumptionPerSecond;
			double jetpackFuelRegenerationPerSecond;
		};

	public:

		PlayerPhysicsComponent(config c);
		virtual ~PlayerPhysicsComponent();

		// PhysicsComponent
		void onReady(core::GameObjectRef parent, core::LevelRef level) override;
		vector<cpBody*> getBodies() const override { return _bodies; }


		// PlayerPhysicsComponent
		const config& getConfig()const { return _config; }

		virtual cpBB getBB() const = 0;
		virtual dvec2 getPosition() const = 0;
		virtual dvec2 getUp() const = 0;
		virtual dvec2 getGroundNormal() const = 0;
		virtual bool isTouchingGround() const = 0;
		virtual cpBody *getBody() const = 0;
		virtual cpBody *getFootBody() const = 0;
		virtual double getJetpackFuelLevel() const = 0;
		virtual double getJetpackFuelMax() const = 0;

		vector<cpShape*> getShapes() const { return _shapes; }
		vector<cpConstraint*> getConstraints() const { return _constraints; }

		// Control inputs, called by Player in Player::update
		virtual void setSpeed( double vel ) { _speed = vel; }
		double getSpeed() const { return _speed; }

		virtual void setFlying( bool j ) { _flying = j; }
		bool isFlying() const { return _flying; }

	protected:

		cpShape *_add( cpShape *shape ) { _shapes.push_back(shape); return shape; }
		cpConstraint *_add( cpConstraint *constraint ) { _constraints.push_back(constraint); return constraint; }
		cpBody *_add( cpBody *body ) { _bodies.push_back(body); return body; }

		dvec2 _getGroundNormal() const;
		bool _isTouchingGround( cpShape *shape ) const;


	protected:

		config _config;
		vector<cpBody*> _bodies;
		vector<cpShape*> _shapes;
		vector<cpConstraint*> _constraints;
		bool _flying;
		double _speed;

	};

#pragma mark - JetpackUnicyclePlayerPhysicsComponent

	class JetpackUnicyclePlayerPhysicsComponent : public PlayerPhysicsComponent {
	public:

		JetpackUnicyclePlayerPhysicsComponent(config c);
		virtual ~JetpackUnicyclePlayerPhysicsComponent();

		// PhysicsComponent
		void onReady(core::GameObjectRef parent, core::LevelRef level) override;
		void step(const core::time_state &timeState) override;

		// PlayerPhysicsComponent
		cpBB getBB() const override;
		dvec2 getPosition() const override;
		dvec2 getUp() const override;
		dvec2 getGroundNormal() const override;
		bool isTouchingGround() const override;
		cpBody *getBody() const override;
		cpBody *getFootBody() const override;
		double getJetpackFuelLevel() const override;
		double getJetpackFuelMax() const override;

		struct capsule {
			dvec2 a,b;
			double radius;
		};

		capsule getBodyCapsule() const;

		struct wheel {
			dvec2 position;
			double radius;
			double radians;
		};

		wheel getFootWheel() const;


	private:

		cpBody *_body, *_wheelBody;
		cpShape *_bodyShape, *_wheelShape, *_groundContactSensorShape;
		cpConstraint *_wheelMotor, *_orientationConstraint;
		double _wheelRadius, _wheelFriction, _touchingGroundAcc, _totalMass;
		double _jetpackFuelLevel, _jetpackFuelMax, _lean;
		dvec2 _up, _groundNormal;
		PlayerInputComponentWeakRef _input;
	};

#pragma mark - PlayerInputComponent

	class PlayerInputComponent : public core::InputComponent {
	public:

		PlayerInputComponent();
		virtual ~PlayerInputComponent();

		// actions
		bool isRunning() const;
		bool isGoingRight() const;
		bool isGoingLeft() const;
		bool isJumping() const;
		bool isCrouching() const;
		bool isShooting() const;
		dvec2 getShootingTargetWorld() const;

	private:

		int _keyRun, _keyLeft, _keyRight, _keyJump, _keyCrouch;
		bool _shooting;

	};

#pragma mark - PlayerDrawComponent

	class PlayerDrawComponent : public core::DrawComponent {
	public:

		PlayerDrawComponent();
		virtual ~PlayerDrawComponent();

		// DrawComponent
		void onReady(core::GameObjectRef parent, core::LevelRef level) override;

		cpBB getBB() const override;
		void draw(const core::render_state &renderState) override;
		void drawScreen(const core::render_state &renderState) override;
		core::VisibilityDetermination::style getVisibilityDetermination() const override;
		int getLayer() const override;
		int getDrawPasses() const override;
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	protected:

		void drawPlayer(const core::render_state &renderState);
		void drawGunCharge(PlayerGunComponentRef gun, const core::render_state &renderState);

	private:
		
		JetpackUnicyclePlayerPhysicsComponentWeakRef _physics;
		PlayerGunComponentWeakRef _gun;
		
	};

#pragma mark - Player

	class Player : public core::GameObject, public TargetTrackingViewportControlComponent::TrackingTarget {
	public:

		struct config {
			PlayerPhysicsComponent::config physics;
			PlayerGunComponent::config gun;
			// TODO: Add a PlayerDrawComponent::config for appearance control

			double walkSpeed;
			double runMultiplier;

			config():
			walkSpeed(1), // 1mps
			runMultiplier(3)
			{}
		};

		/**
		 Create a player configured via the XML in playerXmlFile, at a given position in world units
		 */
		static PlayerRef create(string name, ci::DataSourceRef playerXmlFile, dvec2 position);

	public:
		Player(string name);
		virtual ~Player();

		const config &getConfig() const { return _config; }

		// GameObject
		void update(const core::time_state &time) override;

		// TrackingTarget
		TargetTrackingViewportControlComponent::tracking getViewportTracking() const override;

		const PlayerPhysicsComponentRef &getPhysics() const { return _physics; }
		const PlayerInputComponentRef &getInput() const { return _input; }
		const PlayerGunComponentRef &getGun() const { return _gun; }

	protected:

		virtual void build(config c);

	private:

		config _config;
		PlayerPhysicsComponentRef _physics;
		PlayerDrawComponentRef _drawing;
		PlayerInputComponentRef _input;
		PlayerGunComponentRef _gun;

	};

}

#endif /* Player_hpp */
