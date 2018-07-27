//
//  Terrain.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 3/30/17.
//
//

#ifndef Terrain_hpp
#define Terrain_hpp

#include "core/Core.hpp"
#include "elements/Terrain/TerrainWorld.hpp"

namespace elements {
    namespace terrain {
        
        SMART_PTR(TerrainObject);
        
        class TerrainObject : public core::Object {
        public:
            
            static TerrainObjectRef create(string name, WorldRef world, int drawLayer) {
                return make_shared<TerrainObject>(name, world, drawLayer);
            }
            
        public:
            
            TerrainObject(string name, WorldRef world, int drawLayer);
            
            virtual ~TerrainObject();
            
            void onReady(core::StageRef stage) override;
            
            void onCleanup() override;
            
            void step(const core::time_state &timeState) override;
            
            void update(const core::time_state &timeState) override;
            
            const WorldRef &getWorld() const {
                return _world;
            }
            
        private:
            WorldRef _world;
        };
        
        /**
         TerrainDrawComponent is a thin adapter to TerrainWorld's built-in draw dispatch system
         */
        class TerrainDrawComponent : public core::DrawComponent {
        public:
            
            TerrainDrawComponent(int drawLayer) :
            DrawComponent(drawLayer, core::VisibilityDetermination::ALWAYS_DRAW)
            {
            }
            
            virtual ~TerrainDrawComponent() {
            }
            
            void onReady(core::ObjectRef parent, core::StageRef stage) override;
                        
            void draw(const core::render_state &renderState) override;
            
            
            BatchDrawDelegateRef getBatchDrawDelegate() const override {
                return nullptr;
            }
            
        private:
            
            WorldRef _world;
            
        };
        
        /**
         TerrainPhysicsComponent is a thin adapter to TerrainWorld's physics system
         */
        class TerrainPhysicsComponent : public core::PhysicsComponent {
        public:
            TerrainPhysicsComponent() {
            }
            
            virtual ~TerrainPhysicsComponent() {
            }
            
            void onReady(core::ObjectRef parent, core::StageRef stage) override;
            
            cpBB getBB() const override;
            
            vector<cpBody *> getBodies() const override;
            
        private:
            
            WorldRef _world;
            
        };
        
        class MouseCutterComponent : public core::InputComponent {
        public:
            
            // emitted when a cut is performed, passes start,end,width in world units
            core::signals::signal<void(dvec2,dvec2,double)> onCut;
            
        public:
            
            MouseCutterComponent(terrain::TerrainObjectRef terrain, float radius, int dispatchReceiptIndex = 0);
            
            bool onMouseDown(const app::MouseEvent &event) override;
            
            bool onMouseUp(const app::MouseEvent &event) override;
            
            bool onMouseMove(const app::MouseEvent &event, const ivec2 &delta) override;
            
            bool onMouseDrag(const app::MouseEvent &event, const ivec2 &delta) override;
            
            bool isCutting() const {
                return _cutting;
            }
            
            float getRadius() const {
                return _radius;
            }
            
            dvec2 getCutStart() const {
                return _cutStart;
            }
            
            dvec2 getCutEnd() const {
                return _cutEnd;
            }
            
        private:
            
            bool _cutting;
            float _radius;
            vec2 _mouseScreen, _mouseWorld, _cutStart, _cutEnd;
            terrain::TerrainObjectRef _terrain;
            
        };
        
        class MouseCutterDrawComponent : public core::DrawComponent {
        public:
            
            MouseCutterDrawComponent(ColorA color = ColorA(1, 1, 1, 0.5));
            
            void onReady(core::ObjectRef parent, core::StageRef stage) override;
                        
            void draw(const core::render_state &renderState) override;
                        
        private:
            
            ColorA _color;
            weak_ptr<MouseCutterComponent> _cutterComponent;
            
        };
        
        /**
         AttachmentAdapter is an adapter for "attaching" a game object to a terrain::Attachment.
         Subclasses just need to override updatePosition() and onOrphaned(). updatePosition() implementation
         just needs to set the object's position/angle via whatever's the appropriate means, i.e., in the
         case of an Svg, you'd set the svg's position and angle.
         */
        class AttachmentAdapter : public core::Component {
        public:
            
            // create an AttachmentAdapter which monitors the provided attachment
            AttachmentAdapter(terrain::AttachmentRef attachment);
            
            void update(const core::time_state &timeState) override;
            
            // called when the terrain::Attachment moves, passing world space position/rotation and those two combined into a transform
            virtual void updatePosition(const core::time_state &timeState, dvec2 position, dvec2 rotation, dmat4 transform) {}
            
            // attachment has been orphaned in the terrain::World.
            // call to getAttachment() valid for duration of this method, subsequently, this
            // object should act as a free agent, self terminating or flying away to heaven, whatever
            virtual void onOrphaned() {}
            
            terrain::AttachmentRef getAttachment() const { return _attachment; }
            
            bool isOrphaned() const { return _orphaned; }
            
        private:
            
            void _onOrphaned(size_t id, size_t tag) {
                onOrphaned();
                _orphaned = true;
                _attachment.reset();
            }
            
        private:
            
            terrain::AttachmentRef _attachment;
            bool _orphaned, _positioned;
            dvec2 _lastPosition, _lastRotation;
            
        };
    } // namespace terrain
} // namespace elements

#endif /* Terrain_hpp */
