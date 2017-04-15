//
//  GameObject.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#ifndef GameObject_hpp
#define GameObject_hpp

#include "Common.hpp"
#include "InputDispatcher.hpp"
#include "RenderState.hpp"
#include "TimeState.hpp"

namespace core {

	SMART_PTR(GameObject);
	SMART_PTR(Level);

	SMART_PTR(Component);
	class Component : public enable_shared_from_this<Component>{
	public:

		Component(){}
		virtual ~Component(){}

		// get typed shared_from_this, e.g., shared_ptr<FooComponent> = shared_from_this<FooComponent>();
		template<typename T>
		shared_ptr<T> shared_from_this() const {
			return dynamic_pointer_cast<T>(enable_shared_from_this<Component>::shared_from_this());
		}

		// get typed shared_from_this, e.g., shared_ptr<FooComponent> = shared_from_this<FooComponent>();
		template<typename T>
		shared_ptr<T> shared_from_this() {
			return dynamic_pointer_cast<T>(enable_shared_from_this<Component>::shared_from_this());
		}

		GameObjectRef getGameObject() const { return _gameObject.lock(); }
		LevelRef getLevel() const;

		template<typename T>
		shared_ptr<T> getSibling() const;

		virtual void onReady(GameObjectRef parent, LevelRef level){}
		virtual void step(const time_state &timeState){}
		virtual void update(const time_state &timeState){}

	protected:
		friend class GameObject;

		virtual void attachedToGameObject(GameObjectRef gameObject) {
			_gameObject = gameObject;
		}

		virtual void detachedFromGameObject() {
			_gameObject.reset();
		}

	private:

		GameObjectWeakRef _gameObject;

	};

	SMART_PTR(PhysicsComponent);
	class PhysicsComponent : public Component {
	public:
		PhysicsComponent():_space(nullptr){}
		virtual ~PhysicsComponent(){}

		void onReady(GameObjectRef parent, LevelRef level) override;

		cpSpace* getSpace() const { return _space; }

		// return a vector of all bodies in use
		virtual vector<cpBody*> getBodies() const = 0;

	private:
		cpSpace *_space;
	};


	namespace VisibilityDetermination
	{
		enum style {
			ALWAYS_DRAW,
			FRUSTUM_CULLING,
			NEVER_DRAW
		};

		inline string toString(style s) {
			switch(s) {
				case ALWAYS_DRAW: return "ALWAYS_DRAW";
				case FRUSTUM_CULLING: return "FRUSTUM_CULLING";
				case NEVER_DRAW: return "NEVER_DRAW";
			}
		}
	}


	#pragma mark - BatchDrawDelegate

	SMART_PTR(DrawComponent);
	class DrawComponent : public Component {
	public:

		/**
		 @class BatchDrawDelegate

		 Some DrawComponents are part of a larger whole ( terrain::Shape, or a swarm of identical enemies, 
		 for example ) and it is a great help to efficiency to mark them as being part of a single batch 
		 to be rendered contiguously. In this case, the render system will group the batch together and 
		 then when drawing them, will for the first call prepareForBatchDraw on the first's batchDrawDelegate, 
		 draw each, and then call cleanupAfterBatchDraw on the last of the contiguous set.
		 Note: if these items are on different layers, the draw system will prioritize layer ordering over batching.
		 */
		class BatchDrawDelegate
		{
		public:

			BatchDrawDelegate(){}
			virtual ~BatchDrawDelegate(){}

			/**
			 Called before a batch of like objects are rendered.
			 Passes the render_state, and the first object in the series. This is to simplify
			 situations where a single object is BatchDrawDelegate to multiple target batches.
			 */
			virtual void prepareForBatchDraw( const render_state &, const DrawComponentRef &firstInBatch ){}
			virtual void cleanupAfterBatchDraw( const render_state &, const DrawComponentRef &firstInBatch, const DrawComponentRef &lastInBatch ){}
			
		};

		typedef shared_ptr<BatchDrawDelegate> BatchDrawDelegateRef;

	public:
		DrawComponent(){}
		virtual ~DrawComponent(){}

		virtual cpBB getBB() const = 0;
		virtual void draw(const core::render_state &renderState) = 0;
		virtual VisibilityDetermination::style getVisibilityDetermination() const = 0;
		virtual int getLayer() const = 0;
		virtual int getDrawPasses() const { return 1; }
		virtual BatchDrawDelegateRef getBatchDrawDelegate() const { return nullptr; }

		// call this to notify the draw dispatch system that this DrawComponent has an updated BB
		virtual void notifyMoved();

	};


	SMART_PTR(InputComponent);
	class InputComponent : public Component, public InputListener{
	public:
		InputComponent(){}
		InputComponent(int dispatchReceiptIndex):InputListener(dispatchReceiptIndex){}
		virtual ~InputComponent(){}

		virtual void onReady(GameObjectRef parent, LevelRef level) override;

		void monitorKey( int keyCode );
		void ignoreKey( int keyCode );

		/**
			Called when a monitored key is pressed
		 */
		virtual void monitoredKeyDown( int keyCode ){}

		/**
			Called when a monitored key is released
		 */
		virtual void monitoredKeyUp( int keyCode ){}

		/**
			return true if any monitored keys are pressed.
			this ignores any keys that haven't been registered for monitoring by monitorKey()
		 */
		bool isKeyDown( int keyCode ) const override;

		virtual bool keyDown( const ci::app::KeyEvent &event ) override;
		virtual bool keyUp( const ci::app::KeyEvent &event ) override;

	private:

		map< int, bool > _monitoredKeyStates;
	};


	class GameObject : public enable_shared_from_this<GameObject> {
	public:

		static GameObjectRef with(string name, const initializer_list<ComponentRef> &components) {
			auto obj = make_shared<GameObject>(name);
			for (auto &component : components) {
				obj->addComponent(component);
			}

			return obj;
		}

	public:

		GameObject(string name);
		virtual ~GameObject();

		size_t getId() const { return _id; }
		string getName() const { return _name; }

		virtual void addComponent(ComponentRef component);
		virtual void removeComponent(ComponentRef component);

		virtual void setFinished(bool finished=true) {
			_finished = finished;
		}

		virtual bool isFinished() const { return _finished; }

		template<typename T>
		shared_ptr<T> getComponent() const {
			for (const auto &c : _components) {
				shared_ptr<T> typedC = dynamic_pointer_cast<T>(c);
				if (typedC) {
					return typedC;
				}
			}
			return nullptr;
		}

		DrawComponentRef getDrawComponent() const { return _drawComponent; }

		LevelRef getLevel() const { return _level.lock(); }

		virtual void onReady(LevelRef level);

		virtual void step(const time_state &timeState);
		virtual void update(const time_state &timeState);

	protected:

		friend class Level;
		virtual void onAddedToLevel(LevelRef level) { _level = level; }
		virtual void onRemovedFromLevel() { _level.reset(); }

	private:

		static size_t _idCounter;
		size_t _id;
		string _name;
		bool _finished;
		bool _ready;
		set<ComponentRef> _components;
		DrawComponentRef _drawComponent;
		LevelWeakRef _level;


	};

#pragma mark - Impls

	template<typename T>
	shared_ptr<T> Component::getSibling() const {
		return getGameObject()->getComponent<T>();
	}


}

inline ostream &operator << ( ostream &os, core::VisibilityDetermination::style style )
{
	return os << core::VisibilityDetermination::toString(style);
}


#endif /* GameObject_hpp */
