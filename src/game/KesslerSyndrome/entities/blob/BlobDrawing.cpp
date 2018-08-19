//
//  BlobDrawing.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 8/15/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "BlobDrawing.hpp"

#include "core/MathHelpers.hpp"
#include "core/util/GlslProgLoader.hpp"
#include "core/util/Spline.hpp"

#include <cinder/Perlin.h>

using namespace core;

namespace game {
    
    namespace {
        const ColorA ParticleColor(0,0,0,1);
        
        void drawAxes(cpBody *body, double length, ColorA xAxis = ColorA(1,0,0,1), ColorA yAxis = ColorA(0,1,0,1)) {
            dvec2 origin = v2(cpBodyGetPosition(body));
            dvec2 rotation = v2(cpBodyGetRotation(body));
            
            ci::gl::color(xAxis);
            ci::gl::drawLine(origin, origin + rotation * length);
            
            ci::gl::color(yAxis);
            ci::gl::drawLine(origin, origin + rotateCCW(rotation) * length);
        }
        
        void drawCapsule(dvec2 a, dvec2 b, double radius, bool filled) {
            const dvec2 center = (a + b) * 0.5;
            const double len = distance(a, b);
            const dvec2 dir = (b - a) / len;
            const double angle = atan2(dir.y, dir.x);
            
            gl::ScopedModelMatrix smm;
            mat4 M = glm::translate(dvec3(center.x, center.y, 0)) * glm::rotate(angle, dvec3(0, 0, 1));
            gl::multModelMatrix(M);
            
            if (filled) {
                gl::drawSolidRoundedRect(Rectf(-len / 2 - radius, -radius, +len / 2 + radius, +radius), radius, 8);
            } else {
                gl::drawStrokedRoundedRect(Rectf(-len / 2 - radius, -radius, +len / 2 + radius, +radius), radius, 8);
            }
        }
        
    }

#pragma mark - BlobTentacleDrawer

    /*
     size_t _idx;
     shared_ptr<BlobPhysicsComponent::tentacle> _tentacle;
     vector<vertex> _vertices;
     spline_rep _spline;
     
     gl::Texture2dRef _texture;
     ColorA _color;
     
     gl::GlslProgRef _shader;
     gl::VboRef _vbo;
     gl::BatchRef _batch;
     */
    
    BlobTentacleDrawer::BlobTentacleDrawer(config c, const shared_ptr<BlobPhysicsComponent::tentacle> &tentacle, size_t idx):
    _idx(idx),
    _tentacle(tentacle),
    _texture(c.tex),
    _color(c.color),
    _shader(core::util::loadGlslAsset("kessler/shaders/blob_tentacle.glsl"))
    {
        _shader->uniform("Texture", 0);
        _shader->uniform("Color", _color);
    }
    
    void BlobTentacleDrawer::update(const core::time_state &time) {
        updateSplineReps();
        triangulate();
    }
    
    void BlobTentacleDrawer::draw(const core::render_state &state) {
        
        if (_batch) {
            gl::ScopedTextureBind tex(_texture, 0);
            _batch->draw();
        }
        
        if (state.testGizmoBit(Gizmos::WIREFRAME)) {
            for (size_t i = 0, N = _vertices.size(); i < N; i+= 3) {
                gl::color(ci::ColorA(CM_HSV, static_cast<float>(i) / N, 1, 1));
                gl::drawLine(_vertices[i].position, _vertices[i+1].position);
                gl::drawLine(_vertices[i+1].position, _vertices[i+2].position);
                gl::drawLine(_vertices[i+2].position, _vertices[i].position);
            }
        }
    }
    
    void BlobTentacleDrawer::updateSplineReps() {

        _spline.segmentVertices.clear();
        _spline.splineVertices.clear();

        if ( _tentacle->segments.empty() ) { return; }

        const auto &firstSegment = _tentacle->segments.front();
        const auto &lastSegment(_tentacle->segments.back());
        _spline.startWidth = firstSegment.width;
        _spline.endWidth = lastSegment.width;
        
        // use the root attachment as first spline vertex
        _spline.segmentVertices.push_back(v2(cpBodyLocalToWorld(_tentacle->rootBody, cpv(_tentacle->attachmentAnchor))));

        // now walk segments and record each body position
        for (const auto &segment : _tentacle->segments) {
            _spline.segmentVertices.push_back(v2(cpBodyGetPosition(segment.body)));
        }

        // finally perform spline subdivision
        util::spline::spline<float>( _spline.segmentVertices, 0.5f, _spline.segmentVertices.size() * 5, false, _spline.splineVertices );
    }
    
    void BlobTentacleDrawer::triangulate() {
        size_t nVertices = _vertices.size();
        _vertices.clear();

        const auto &spline = _spline.splineVertices;

        const bool odd = _idx % 2;
        float distanceAlongTentacle = 0;
        float halfWidth = _spline.startWidth / 2;
        const size_t numSegments = _tentacle->segments.size();
        
        const float
            incrementScale = static_cast<float>(1) / ( spline.size()-1),
            halfWidthIncrement = ((_spline.endWidth/2) - halfWidth) * incrementScale,
            ty = odd ? 0 : 1,
            ty2 = odd ? 1 : 0,
            dxs = numSegments * incrementScale * 0.5f;

        vec2 a,b,c,d;
        segment_extents( spline.front(), spline[1], halfWidth, a, d );
        a += spline.front();
        d += spline.front();
        
        //
        //    generate triangles from spline; our indexing looks like so:
        //    a -> b
        //    ^ /  v
        //    d <- c
        //    making triangles (a,b,d), (c,d,b) from each quad
        //
        
        for( std::vector< vec2 >::const_iterator
            s0(spline.begin()),
            s1(spline.begin()+1),
            end(spline.end());
            s1 != end;
            ++s0, ++s1 )
        {
            float length = segment_extents( *s0, *s1, halfWidth + halfWidthIncrement, b, c );
            b += *s1;
            c += *s1;
            
            float nextDistanceAlongTentacle = distanceAlongTentacle + dxs * length;
            
            const vertex va{ a, vec2( distanceAlongTentacle, ty ) };
            const vertex vb{ b, vec2( nextDistanceAlongTentacle, ty ) };
            const vertex vc{ c, vec2( nextDistanceAlongTentacle, ty2 ) };
            const vertex vd{ d, vec2( distanceAlongTentacle, ty2 ) };
            
            _vertices.push_back(va);
            _vertices.push_back(vb);
            _vertices.push_back(vd);

            _vertices.push_back(vc);
            _vertices.push_back(vd);
            _vertices.push_back(vb);
            
            distanceAlongTentacle = nextDistanceAlongTentacle;
            halfWidth += halfWidthIncrement;
            
            a = b;
            d = c;
        }
        
        //
        //    If number of quads changed, we need to (re)create the batch
        //
        
        if ( _vertices.size() != nVertices ) {
            _vbo = gl::Vbo::create(GL_ARRAY_BUFFER, _vertices, GL_STREAM_DRAW);
            
            geom::BufferLayout particleLayout;
            particleLayout.append(geom::Attrib::POSITION, 2, sizeof(vertex), offsetof(vertex, position));
            particleLayout.append(geom::Attrib::TEX_COORD_0, 2, sizeof(vertex), offsetof(vertex, texCoord));
            
            // pair our layout with vbo to create a cinder Batch
            auto mesh = gl::VboMesh::create(static_cast<uint32_t>(_vertices.size()), GL_TRIANGLES, {{particleLayout, _vbo}});
            _batch = gl::Batch::create(mesh, _shader);
        } else {

            //
            // Vbo exists, so just copy new vertices over
            //
            
            void *gpuMem = _vbo->mapReplace();
            memcpy(gpuMem, _vertices.data(), _vertices.size() * sizeof(vertex));
            _vbo->unmap();
        }
        
        
    }
    
#pragma mark - BlobDebugDrawComponent
    
    /*
     config _config;
     BlobPhysicsComponentWeakRef _physics;
     cpBB _bb;
     */
    
    BlobDebugDrawComponent::BlobDebugDrawComponent(const config &c):
            EntityDrawComponent(c.drawLayer,VisibilityDetermination::FRUSTUM_CULLING),
            _config(c)
    {}
    
    void BlobDebugDrawComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        DrawComponent::onReady(parent,stage);
        _physics = getSibling<BlobPhysicsComponent>();
    }
    
    void BlobDebugDrawComponent::update(const core::time_state &timeState) {
        const auto physics = _physics.lock();
        _bb = physics->getBB();
    }
    
    void BlobDebugDrawComponent::draw(const core::render_state &renderState) {
        const auto physics = _physics.lock();
        drawParticles(renderState, physics);
        drawTentacles(renderState, physics);
        
        const double axisLength = 16.0 * renderState.viewport->getReciprocalScale();
        drawAxes(physics->getCentralBody(), axisLength);
        
        gl::color(ColorA(1,0,1,1));
        gl::drawStrokedRect(Rectf(_bb.l, _bb.t, _bb.r, _bb.b));
    }
    
    void BlobDebugDrawComponent::drawParticles(const core::render_state &renderState, const BlobPhysicsComponentRef &physics) {
        for (const auto &particle : physics->getPhysicsParticles()) {
            dvec2 position = v2(cpBodyGetPosition(particle.body));
            double radius = particle.radius * particle.scale;
            ci::gl::color(_config.particleColor.lerp(particle.shepherdValue, ColorA(1,1,0,1)));
            ci::gl::drawSolidCircle(position, radius, 16);
        }
    }

    void BlobDebugDrawComponent::drawTentacles(const core::render_state &renderState, const BlobPhysicsComponentRef &physics) {
        ci::gl::color(_config.tentacleColor.lerp(0.7, ColorA(0,0,0,1)));
        for (const auto &tentacle : physics->getTentacles()) {
            dvec2 a = v2(cpBodyLocalToWorld(physics->getCentralBody(), cpPivotJointGetAnchorA(tentacle->segments.front().joint)));
            for (const auto &segment : tentacle->segments) {
                dvec2 b = v2(cpBodyLocalToWorld(segment.body, cpPivotJointGetAnchorB(segment.joint)));
                if (lengthSquared(a-b) > 0.01) {
                    drawCapsule(a, b, segment.width/2, false);
                }
                a = b;
            }
        }
    }
    
#pragma mark - BlobDebugDrawComponent
    
    /*
     config _config;
     BlobPhysicsComponentWeakRef _physics;
     cpBB _bb;
     vector<BlobTentacleDrawer> _tentacleDrawers;
     */
    
    BlobTentacleDrawComponent::BlobTentacleDrawComponent(const config &c):
    EntityDrawComponent(c.drawLayer,VisibilityDetermination::FRUSTUM_CULLING),
    _config(c)
    {}
    
    void BlobTentacleDrawComponent::onReady(core::ObjectRef parent, core::StageRef stage) {
        DrawComponent::onReady(parent,stage);
        _physics = getSibling<BlobPhysicsComponent>();
    }
    
    void BlobTentacleDrawComponent::update(const core::time_state &timeState) {
        const auto physics = _physics.lock();
        _bb = physics->getBB();
        
        if (_tentacleDrawers.empty()) {
            size_t idx = 0;
            BlobTentacleDrawer::config c { _config.tentacleTexture, _config.tentacleColor };
            for (const auto &tentacle : physics->getTentacles()) {
                _tentacleDrawers.emplace_back(c, tentacle, idx++);
            }
        }
        
        for (auto &tentacleDrawer : _tentacleDrawers) {
            tentacleDrawer.update(timeState);
        }
    }
    
    void BlobTentacleDrawComponent::draw(const core::render_state &renderState) {
        for (auto &tentacleDrawer : _tentacleDrawers) {
            tentacleDrawer.draw(renderState);
        }
    }
    
#pragma mark - BlobParticleSimulation
    
    
    BlobParticleSimulation::BlobParticleSimulation(BlobPhysicsComponentRef physics):
            _physics(physics),
            _perlin(4,12345)
    {}
    
    void BlobParticleSimulation::setParticleCount(size_t particleCount) {
        BaseParticleSimulation::setParticleCount(particleCount);
    }
    
    void BlobParticleSimulation::onReady(core::ObjectRef parent, core::StageRef stage) {
        for (auto state(_state.begin()), end(_state.end()); state != end; ++state) {
            state->active = true;
            state->atlasIdx = 0;
            state->color = ParticleColor;
            state->additivity = 0;
        }
        
        update(stage->getTimeState());
    }
    
    void BlobParticleSimulation::update(const core::time_state &timeState) {
        auto blobPhysics = _physics.lock();
        auto physics = blobPhysics->getPhysicsParticles().begin();
        auto state = _state.begin();
        const auto end = _state.end();
        cpBB bb = cpBBInvalid;
        double noiseOffset = timeState.time;
        double noiseStep = 0.42;
        
        for (; state != end; ++state, ++physics, noiseOffset += noiseStep) {
            
            state->position = v2(cpBodyGetPosition(physics->body));
            
            const double angle = _perlin.noise(noiseOffset);
            double cosa, sina;
            __sincos(angle, &sina, &cosa);
            const double radius = 3 * physics->radius * physics->scale * (1-physics->shepherdValue);
            state->right = radius * dvec2(cosa, sina);
            state->up = rotateCCW(state->right);
            
            bb = cpBBExpand(bb, state->position, physics->radius);
        }
        
        _bb = bb;
    }
    
    size_t BlobParticleSimulation::getFirstActive() const {
        return 0;
    };
    
    size_t BlobParticleSimulation::getActiveCount() const {
        return _state.size();
    }
    
    cpBB BlobParticleSimulation::getBB() const {
        return _bb;
    }
    
    namespace {
        
        /// custom FilterStack for running a composite filter on blob's particle system render pass
        class TonemapScreenCompositor : public FilterStack {
        public:
            TonemapScreenCompositor(BlobParticleSystemDrawComponent::config config):
                    _config(config)
            {
                auto compositor = util::loadGlslAsset("kessler/filters/tonemap_compositor.glsl");
                compositor->uniform("Alpha", static_cast<float>(1));
                setScreenCompositeShader(compositor);
            }
            
        protected:
            
            void performScreenComposite(const render_state &state, const gl::GlslProgRef &shader, const gl::FboRef &color) override {
                gl::ScopedTextureBind tonemap(_config.tonemap, 1);
                shader->uniform("Tonemap", 1);
                
                gl::ScopedTextureBind background(_config.background, 2);
                shader->uniform("BackgroundFill", 2);
                
                // use aspect ratio to build a scaling factor to guarantee background fills the viewport without tiling when BackgroundFillRepeat == 1
                const float aspectRatio = static_cast<float>(state.viewport->getAspect());
                vec2 aspect(1,1);
                if (aspectRatio > 1) {
                    aspect.y = 1 / aspectRatio;
                } else {
                    aspect.x = aspectRatio;
                }
                
                shader->uniform("Aspect", aspect);
                shader->uniform("BackgroundFillRepeat", _config.backgroundRepeat);
                shader->uniform("HighlightColor", _config.highlightColor);
                
                FilterStack::performScreenComposite(state, shader, color);
            }
            
        private:
            
            BlobParticleSystemDrawComponent::config _config;
            
        };
        
    }
    
    BlobParticleSystemDrawComponent::BlobParticleSystemDrawComponent(config c):
            elements::ParticleSystemDrawComponent(c)
    {
        auto clearColor = ColorA(ParticleColor,0);
        auto stack = make_shared<TonemapScreenCompositor>(c);
        setFilterStack(stack, clearColor);
    }
    
    gl::GlslProgRef BlobParticleSystemDrawComponent::createDefaultShader() const {
        return util::loadGlslAsset("kessler/shaders/blob_ps.glsl");
    }
    
    
}
