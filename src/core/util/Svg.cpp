//
//  Svg.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/4/17.
//
//

#include "core/util/Svg.hpp"

#include "core/Strings.hpp"
#include "core/ChipmunkHelpers.hpp"

namespace core {
    namespace util {
        
        void BlendMode::bind() const {
            glBlendFuncSeparate(_srcColor, _dstColor, _srcAlpha, _dstAlpha);
            glBlendEquationSeparate(_blendEqColor, _blendEqAlpha);
        }

        
        namespace svg {

#pragma mark - Parsing

            namespace {

                string indent(int l) {
                    return string(l, '\t');
                }

                template<typename TRANSFORMER>
                cpBB transform(const TriMeshRef &sourceMesh, TriMeshRef &destMesh,
                        const vector <Shape::stroke> &sourceStrokes, vector <Shape::stroke> &destStrokes,
                        const TRANSFORMER &transformer) {
                    //
                    //	transform vertices from sourceMesh into destMesh, copying over same indices
                    //

                    const vec2 *vertices = sourceMesh->getPositions<2>();
                    const size_t numVertices = sourceMesh->getNumVertices();
                    const vector <uint32_t> &indices = sourceMesh->getIndices();

                    cpBB bounds = cpBBInvalid;
                    vector <vec2> destVertices;
                    destVertices.reserve(numVertices);
                    for (int i = 0; i < numVertices; i++) {
                        const vec2 v = transformer(vertices[i]);
                        bounds = cpBBExpand(bounds, v);
                        destVertices.push_back(v);
                    }

                    vector <uint32_t> destIndices;
                    destIndices.reserve(indices.size());
                    for (auto index : indices) {
                        destIndices.push_back(index);
                    }

                    //
                    //	transfer new vertices and same indices over to destMesh
                    //

                    if (destMesh) {
                        destMesh->clear();
                    } else {
                        destMesh = make_shared<TriMesh>(TriMesh::Format().positions(2));
                    }

                    destMesh->appendPositions(&(destVertices.front()), destVertices.size());
                    destMesh->appendIndices(&(destIndices.front()), destIndices.size());

                    //
                    //	transform vertices in sourceStrokes into destStrokes
                    //

                    destStrokes.clear();
                    for (auto stroke : sourceStrokes) {
                        destStrokes.emplace_back();
                        Shape::stroke &back(destStrokes.back());

                        back.closed = stroke.closed;
                        for (auto v : stroke.vertices) {
                            back.vertices.push_back(transformer(v));
                        }
                    }

                    return bounds;
                }

                struct to_world_transformer {
                    double documentScale;
                    dvec2 documentSize;
                    dmat4 toWorld;

                    to_world_transformer(const dvec2 &dSize, double dScale, const dmat4 tw) :
                            documentScale(dScale),
                            documentSize(dSize),
                            toWorld(tw) {
                    }

                    inline dvec2 operator()(const dvec2 &v) const {
                        dvec2 world = (toWorld * v) * documentScale;
                        world.y = documentSize.y - world.y;

                        return world;
                    }
                };

                struct to_local_transformer {
                    dvec2 localOrigin;

                    to_local_transformer(const dvec2 &lo) :
                            localOrigin(lo) {
                    }

                    inline dvec2 operator()(const dvec2 &v) const {
                        return v - localOrigin;
                    }
                };

            } // end anonymous namespace

#pragma mark - Appearance

            /*
                AppearanceWeakRef _parentAppearance;
                svg_style _style;
                BlendMode _blendMode;
                map< string, string > _attributes;
             */

            Appearance::Appearance() {
            }

            Appearance::~Appearance() {
            }

            void Appearance::parse(const XmlTree &node) {

                //
                //	Cache all attributes, in case we want to query them later
                //

                for (const XmlTree::Attr &attr : node.getAttributes()) {
                    _attributes[attr.getName()] = attr.getValue();
                }

                _style = util::svg::parseStyle(node);
            }

            void Appearance::setFillColor(const ColorA &color) {
                _style.fillColor = color;
                _style.fillOpacity = color.a;
                _style.hasFillColor = true;
                _style.hasFillOpacity = true;
            }

            ColorA Appearance::getFillColor() const {
                return ColorA(_getFillColor(), _getFillOpacity());
            }

            void Appearance::setFillAlpha(double a) {
                _style.fillOpacity = a;
                _style.hasFillColor = true;
            }

            bool Appearance::getFillAlpha() const {
                return _getFillOpacity();
            }

            void Appearance::setStrokeColor(const ColorA &color) {
                _style.strokeColor = color;
                _style.strokeOpacity = color.a;
                _style.hasStrokeColor = true;
                _style.hasStrokeOpacity = true;
            }

            ColorA Appearance::getStrokeColor() const {
                return ColorA(_getStrokeColor(), _getStrokeOpacity());
            }

            void Appearance::setStrokeWidth(double w) {
                _style.strokeWidth = max(w, double(0));
                _style.hasStrokeWidth = true;;
            }

            double Appearance::getStrokeWidth() const {
                return _getStrokeWidth();
            }

            bool Appearance::isStroked() const {
                return _getStrokeWidth() > 0 && _getStrokeOpacity() > ALPHA_EPSILON;
            }

            bool Appearance::isFilled() const {
                return _getFillOpacity() > ALPHA_EPSILON;
            }

            void Appearance::setBlendMode(const BlendMode &bm) {
                _blendMode = bm;
            }

            const BlendMode &Appearance::getBlendMode() const {
                return _blendMode;
            }

            void Appearance::setFillRule(Triangulator::Winding fr) {
                _style.fillRule = fr;
                _style.hasFillRule = true;
            }

            Triangulator::Winding Appearance::getFillRule() const {
                return _getFillRule();
            }

            string Appearance::getAttribute(string name) const {
                const auto pos = _attributes.find(name);
                if (pos != _attributes.end()) {
                    return pos->second;
                }

                if (AppearanceRef parent = _parentAppearance.lock()) {
                    return parent->getAttribute(name);
                }

                return "";
            }

            Color Appearance::_getFillColor() const {
                if (_style.hasFillColor) {
                    return _style.fillColor;
                } else if (AppearanceRef p = _parentAppearance.lock()) {
                    return p->_getFillColor();
                }
                return _style.fillColor; // default
            }

            double Appearance::_getFillOpacity() const {
                if (_style.hasFillOpacity) {
                    return _style.fillOpacity;
                } else if (AppearanceRef p = _parentAppearance.lock()) {
                    return p->_getFillOpacity();
                }
                return _style.fillOpacity; // default
            }

            Color Appearance::_getStrokeColor() const {
                if (_style.hasStrokeColor) {
                    return _style.strokeColor;
                } else if (AppearanceRef p = _parentAppearance.lock()) {
                    return p->_getStrokeColor();
                }
                return _style.strokeColor; // default
            }

            double Appearance::_getStrokeOpacity() const {
                if (_style.hasStrokeOpacity) {
                    return _style.strokeOpacity;
                } else if (AppearanceRef p = _parentAppearance.lock()) {
                    return p->_getStrokeOpacity();
                }
                return _style.strokeOpacity; // default
            }

            double Appearance::_getStrokeWidth() const {
                if (_style.hasStrokeWidth) {
                    return _style.strokeWidth;
                } else if (AppearanceRef p = _parentAppearance.lock()) {
                    return p->_getStrokeWidth();
                }
                return _style.strokeWidth; // default
            }

            Triangulator::Winding Appearance::_getFillRule() const {
                if (_style.hasFillRule) {
                    return _style.fillRule;
                } else if (AppearanceRef p = _parentAppearance.lock()) {
                    return p->_getFillRule();
                }
                return _style.fillRule; // default
            }


#pragma mark - Shape

            /*
                AppearanceRef _appearance;
                bool _origin;
                dmat4 _svgTransform;
                map< string, string > _attributes;
                string _type, _id;
                cpBB _localBB;

                TriMeshRef _svgMesh, _worldMesh, _localMesh;
                gl::VboMeshRef _localVboMesh;

                vector<stroke> _svgStrokes, _worldStrokes, _localStrokes;
                Rectd _worldBounds, _localBounds;	 */

            Shape::Shape() :
                    _appearance(make_shared<Appearance>()),
                    _origin(false),
                    _localBB(cpBBInvalid) {
            }

            Shape::~Shape() {
            }

            void Shape::parse(const XmlTree &shapeNode) {
                _type = shapeNode.getTag();

                if (shapeNode.hasAttribute("id")) {
                    _id = shapeNode.getAttribute("id").getValue();
                }

                //
                // any node with id containing string __origin__ is an origin node for group
                //
                
                if (_id.find("__origin__") == 0) {
                    _origin = true;
                }

                //
                //	Cache all attributes, in case we want to query them later
                //

                for (const XmlTree::Attr &attr : shapeNode.getAttributes()) {
                    _attributes[attr.getName()] = attr.getValue();
                }

                //
                //	Parse style attributes from this node
                //

                _appearance->parse(shapeNode);

                //
                //	Get the transform
                //

                if (shapeNode.hasAttribute("transform")) {
                    _svgTransform = util::svg::parseTransform(shapeNode.getAttribute("transform").getValue());
                }


                //
                //	parse svg shape into a Shape2d, and then convert that to _svgMesh in document space.
                //	later, in Group::_normalize we'll project to world space, and then to local space.
                //
                Shape2d svgShape;
                util::svg::parseShape(shapeNode, svgShape);
                _build(svgShape);
            }

            void Shape::draw(const render_state &state, const GroupRef &owner, double opacity, const gl::GlslProgRef &shader) {
                CI_ASSERT_MSG(doesDraw(), "Should never call draw() on a shape which doesn't draw");
                
                if (!isOrigin()) {
                    _drawGame(state, owner, opacity, _localMesh, _localStrokes, shader);
                }
                
                if (state.gizmoMask) {
                    _drawGizmos(state, owner, opacity, _localMesh, _localStrokes, shader);
                }
            }
            
            bool Shape::doesDraw() const {
                return _appearance->isFilled() || _appearance->isStroked();
            }

            void Shape::_build(const Shape2d &shape) {
                _svgMesh.reset();
                _worldMesh.reset();
                _localMesh.reset();
                _svgStrokes.clear();
                _worldStrokes.clear();
                _localStrokes.clear();

                const double ApproximationScale = 0.5;
                const AppearanceRef appearance = getAppearance();
                
                //
                //  Triangulate and subdivide perimeter.
                //

                _svgMesh = Triangulator(shape, ApproximationScale).createMesh(appearance->getFillRule());

                for (const Path2d &path : shape.getContours()) {
                    _svgStrokes.push_back(stroke());
                    _svgStrokes.back().closed = path.isClosed();
                    _svgStrokes.back().vertices = path.subdivide(ApproximationScale);
                }
            }

            Rectd Shape::_projectToWorld(const dvec2 &documentSize, double documentScale, const dmat4 &worldTransform) {
                dmat4 toWorld = worldTransform * _svgTransform;
                cpBB bounds = transform(_svgMesh, _worldMesh, _svgStrokes, _worldStrokes, to_world_transformer(documentSize, documentScale, toWorld));

                _worldBounds.set(bounds.l, bounds.b, bounds.r, bounds.t);
                return _worldBounds;
            }

            void Shape::_makeLocal(const dvec2 &originWorld) {

                //
                //	Move from world to local coordinate system
                //

                _localBB = transform(_worldMesh, _localMesh, _worldStrokes, _localStrokes, to_local_transformer(originWorld));

                //
                // free up memory we're not using anymore
                //

                _worldMesh.reset();
                _svgMesh.reset();
                _worldStrokes.clear();
                _svgStrokes.clear();
            }

            void Shape::_drawGizmos(const render_state &state, const GroupRef &owner, double opacity, const TriMeshRef &mesh, vector <stroke> &strokes, const gl::GlslProgRef &shader) {
                if (isOrigin()) {
                    
                    if (state.testGizmoBit(Gizmos::ANCHORS)) {
                        dmat4 worldTransform = owner->_worldTransform(dmat4(1));

                        double screenSize = 20;
                        dvec2 xAxis(worldTransform[0]);
                        dvec2 yAxis(worldTransform[1]);

                        dvec2 scale(screenSize / length(xAxis), screenSize / length(yAxis));
                        scale *= state.viewport->getReciprocalScale();

                        shader->uniform("Color", ColorA(1, 0, 0, 1));
                        gl::drawLine(vec2(0, 0), vec2(scale.x, 0));
                        shader->uniform("Color", ColorA(1, 0, 0, 0.5));
                        gl::drawLine(vec2(0, 0), vec2(-scale.x, 0));

                        shader->uniform("Color", ColorA(0, 1, 0, 1));
                        gl::drawLine(vec2(0, 0), vec2(0, scale.y));
                        shader->uniform("Color", ColorA(0, 1, 0, 0.5));
                        gl::drawLine(vec2(0, 0), vec2(0, -scale.y));
                    }
                    
                } else {

                    if (state.testGizmoBit(Gizmos::WIREFRAME)) {
                        //
                        //    Crude immediate-mode style drawing of meshes and strokes
                        //
                        
                        const AppearanceRef appearance = getAppearance();
                        
                        if (appearance->isFilled()) {
                            ColorA fc = appearance->getFillColor();
                            shader->uniform("Color", ColorA(fc.r, fc.g, fc.b, fc.a * opacity));
                            gl::draw(*mesh);
                        }
                        
                        if (appearance->isStroked()) {
                            ColorA sc = appearance->getStrokeColor();
                            shader->uniform("Color", ColorA(sc.r, sc.g, sc.b, sc.a * opacity));
                            gl::lineWidth(1);
                            
                            for (auto &stroke : strokes) {
                                vec2 a = stroke.vertices.front();
                                for (auto it = stroke.vertices.begin() + 1, end = stroke.vertices.end(); it != end; ++it) {
                                    vec2 b = *it;
                                    gl::drawLine(a, b);
                                    a = b;
                                }
                                
                                if (stroke.closed) {
                                    gl::drawLine(a, stroke.vertices.front());
                                }
                            }
                        }
                    }
                }
            }

            void Shape::_drawGame(const render_state &state, const GroupRef &owner, double opacity, const TriMeshRef &mesh, vector <stroke> &strokes, const gl::GlslProgRef &shader) {
                gl::Context::getCurrent()->setDefaultShaderVars();
                _drawFills(state, owner, opacity, mesh, shader);
                _drawStrokes(state, owner, opacity, strokes, shader);
            }

            void Shape::_drawStrokes(const render_state &state, const GroupRef &owner, double opacity, vector <stroke> &strokes, const gl::GlslProgRef &shader) {
                const AppearanceRef appearance = getAppearance();
                if (appearance->isStroked()) {

                    ColorA sc = appearance->getStrokeColor();
                    if (sc.a * opacity > ALPHA_EPSILON) {
                        shader->uniform("Color", ColorA(sc.r, sc.g, sc.b, sc.a * opacity));
                        gl::lineWidth(1);

                        for (auto &stroke : strokes) {
                            if (!stroke.vboMesh) {
                                gl::VboMesh::Layout layout;
                                layout.usage(GL_STATIC_DRAW).attrib(geom::POSITION, 2);

                                const GLsizei count = static_cast<GLsizei>(stroke.vertices.size());
                                const GLenum primitive = stroke.closed ? GL_LINE_LOOP : GL_LINE_STRIP;
                                stroke.vboMesh = gl::VboMesh::create(count, primitive, {layout});
                                stroke.vboMesh->bufferAttrib(geom::POSITION, count * sizeof(vec2), stroke.vertices.data());
                            }

                            gl::draw(stroke.vboMesh);
                        }
                    }
                }
            }

            void Shape::_drawFills(const render_state &state, const GroupRef &owner, double opacity, const TriMeshRef &mesh, const gl::GlslProgRef &shader) {
                const AppearanceRef appearance = getAppearance();
                if (appearance->isFilled()) {
                    ColorA fc = appearance->getFillColor();
                    if (fc.a * opacity > ALPHA_EPSILON) {
                        shader->uniform("Color", ColorA(fc.r, fc.g, fc.b, fc.a * opacity));

                        if (!_localVboMesh) {
                            _localVboMesh = gl::VboMesh::create(*mesh);
                        }

                        gl::draw(_localVboMesh);
                    }
                }
            }


#pragma mark - Group

            /*
                typedef pair< GroupRef,ShapeRef> drawable;

                GroupWeakRef _parent;
                ShapeRef _originShape;
                AppearanceRef _appearance;

                string _id;
                double _opacity;
                bool _transformDirty;
                double _localTransformAngle;
                dvec2 _localTransformPosition, _localTransformScale, _documentSize;
                dmat4 _groupTransform, _transform;
                BlendMode _blendMode;

                vector< ShapeRef > _shapes;
                map< string, ShapeRef > _shapesById;
                vector< GroupRef > _groups;
                map< string, GroupRef > _groupsById;
                map< string, string > _attributes;
                vector< drawable > _drawables;

                gl::GlslProgRef _shader;
             */

            Group::Group() :
                    _appearance(make_shared<Appearance>()),
                    _opacity(1),
                    _transformDirty(true),
                    _localTransformAngle(0),
                    _localTransformPosition(0, 0),
                    _localTransformScale(1, 1),
                    _documentSize(0, 0) {
            }

            Group::~Group() {
                clear();
            }

            void Group::load(DataSourceRef svgData, double documentScale) {
                clear();

                //
                //	We're reading a document, not a <g> group; but we want to load attributes and children and (if any,) shapes
                //

                XmlTree svgNode = XmlTree(svgData).getChild("svg");
                _parse(svgNode);

                //
                //	Figure out the document size. This is important because we use a bottom-left
                //	coordinate system, and SVG uses a top-left. We need to flip.
                //

                const Rectd documentFrame = util::svg::parseDocumentFrame(svgNode);
                const dvec2 parentWorldOrigin(documentFrame.getX1(), documentFrame.getY1());
                _documentSize.x = documentFrame.getWidth();
                _documentSize.y = documentFrame.getHeight();

                //
                //	Now normalize our generated geometry
                //

                _normalize(_documentSize, documentScale, dmat4(1), parentWorldOrigin);
            }

            void Group::clear() {
                _originShape.reset();
                _shapes.clear();
                _shapesById.clear();
                _groups.clear();
                _groupsById.clear();
                _attributes.clear();
                _drawables.clear();
            }

            void Group::draw(const render_state &state) {

                //
                //	Build the shader (lazily)
                //

                if (isRootGroup() && !_shader) {
                    auto vsh = CI_GLSL(150,
                            uniform
                            mat4 ciModelViewProjection;
                            in
                            vec4 ciPosition;

                            void main(void) {
                                gl_Position = ciModelViewProjection * ciPosition;
                            }
                    );

                    auto fsh = CI_GLSL(150,
                            uniform
                            vec4 Color;
                            out
                            vec4 oColor;

                            void main(void) {
                                oColor = Color;
                            }
                    );


                    _shader = gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));
                }

                {
                    gl::ScopedGlslProg sp(_shader);
                    _draw(state, 1, _shader);
                }

                if (state.testGizmoBit(Gizmos::AABBS) && isRootGroup()) {
                    gl::color(1, 0, 1);
                    cpBB bounds = getBB();
                    gl::drawStrokedRect(Rectf(bounds.l, bounds.b, bounds.r, bounds.t), state.viewport->getReciprocalScale());
                }
            }

            cpBB Group::getBB() {
                return _getBB(dmat4());
            }

            void Group::trace(int depth) const {
                std::string
                        ind = indent(depth),
                        ind2 = indent(depth + 1);

                app::console() << ind << "[Group name: " << getId() << std::endl << ind << " +shapes:" << std::endl;

                for (ShapeRef shape : getChildShapes()) {
                    app::console() << ind2 << "[SvgShape name: " << shape->getId()
                                   << " type: " << shape->getType()
                                   << " origin: " << str(shape->isOrigin())
                                   << " filled: " << str(shape->getAppearance()->isFilled())
                                   << " stroked: " << str(shape->getAppearance()->isStroked())
                                   << " fillColor: " << shape->getAppearance()->getFillColor()
                                   << " strokeColor: " << shape->getAppearance()->getStrokeColor()
                                   << "]" << std::endl;
                }

                app::console() << ind << " +groups:" << std::endl;
                for (const GroupRef &obj : getChildGroups()) {
                    obj->trace(depth + 1);
                }

                app::console() << ind << "]" << std::endl;
            }

            GroupRef Group::getRootGroup() const {
                auto r = const_cast<Group *>(this)->shared_from_this();

                while (GroupRef p = r->getParentGroup()) {
                    r = p;
                }

                return r;
            }

            bool Group::isRootGroup() const {
                return !getParentGroup();
            }

            GroupRef Group::getChildGroupById(const std::string &name) const {
                auto pos(_groupsById.find(name));
                if (pos != _groupsById.end()) return pos->second;

                return nullptr;
            }

            ShapeRef Group::getChildShapeById(const std::string &name) const {
                auto pos(_shapesById.find(name));
                if (pos != _shapesById.end()) return pos->second;

                return nullptr;
            }

            GroupRef Group::findGroup(const std::string &pathToChild, char separator) const {
                if (pathToChild.at(0) == separator) {
                    return getRootGroup()->findGroup(pathToChild.substr(1), separator);
                }

                GroupRef node = const_cast<Group *>(this)->shared_from_this();
                strings::stringvec path = strings::split(pathToChild, separator);

                for (auto childName(path.begin()), end(path.end()); childName != end; ++childName) {
                    node = node->getChildGroupById(*childName);

                    //
                    //	if the node is empty, return it as a failure. if we're on the last element, return the node
                    //

                    if (!node || ((childName + 1) == end)) return node;
                }

                return nullptr;
            }
            
            namespace {
                GroupRef _findGroupById(const GroupRef &group, const string &groupId) {
                    if (group->getId() == groupId) {
                        return group;
                    }
                    for (const auto &childGroup : group->getChildGroups()) {
                        auto result = _findGroupById(childGroup, groupId);
                        if (result != nullptr) {
                            return result;
                        }
                    }
                    return nullptr;
                }
                
                ShapeRef _findShapeById(const GroupRef &group, const string &shapeId) {
                    for (const auto &childShape : group->getChildShapes()) {
                        if (childShape->getId() == shapeId) {
                            return childShape;
                        }
                    }
                    for (const auto &childGroup : group->getChildGroups()) {
                        auto result = _findShapeById(childGroup, shapeId);
                        if (result != nullptr) {
                            return result;
                        }
                    }
                    return nullptr;                }
            }
            
            GroupRef Group::findGroupById(const string &groupId, bool searchFromRoot) {
                auto node = searchFromRoot ? getRootGroup() : shared_from_this();
                return _findGroupById(node, groupId);
            }
            
            ShapeRef Group::findShapeById(const string &shapeId, bool searchFromRoot) {
                auto node = searchFromRoot ? getRootGroup() : shared_from_this();
                return _findShapeById(node, shapeId);
            }

            dvec2 Group::getDocumentSize() const {
                return getRootGroup()->_documentSize;
            }

            void Group::setPosition(const dvec2 &position) {
                _localTransformPosition = position;
                _transformDirty = true;
            }
            
            void Group::setRotation(const dvec2 &rotation) {
                _localTransformAngle = atan2(rotation.y, rotation.x);
                _transformDirty = true;
            }
            
            dvec2 Group::getRotation() const {
                return dvec2(cos(_localTransformAngle), sin(_localTransformAngle));
            }

            void Group::setAngle(double a) {
                _localTransformAngle = a;
                _transformDirty = true;
            }
            
            void Group::setScale(const dvec2 &scale) {
                _localTransformScale = scale;
                _transformDirty = true;
            }

            void Group::setBlendMode(const BlendMode &bm) {
                _blendMode = bm;
            }

            dvec2 Group::localToGlobal(const dvec2 &p) {
                const dmat4 worldTransform = _worldTransform(dmat4(1));
                return worldTransform * p;
            }

            dvec2 Group::globalToLocal(const dvec2 &p) {
                const dmat4 worldTransform = _worldTransform(dmat4(1));
                const dmat4 inverseWorldTransform = glm::inverse(worldTransform);
                return inverseWorldTransform * p;
            }
            
            void Group::_parse(const XmlTree &svgGroupNode) {
                clear();
                _parseGroupAttributes(svgGroupNode);
                _loadAppearances(svgGroupNode);
                _loadGroupsAndShapes(svgGroupNode);
            }

            void Group::_draw(const render_state &state, double parentOpacity, const gl::GlslProgRef &shader) {
                auto self = shared_from_this();
                double opacity = _opacity * parentOpacity;
                BlendMode lastBlendMode = getBlendMode();

                gl::enable(GL_BLEND);
                lastBlendMode.bind();

                _updateTransform();

                gl::ScopedModelMatrix smm;
                gl::multModelMatrix(_transform);

                for (auto c(_drawables.begin()), end(_drawables.end()); c != end; ++c) {
                    if (c->first) {

                        //
                        //	Draw child group
                        //

                        c->first->_draw(state, opacity, shader);
                    } else if (c->second) {
                       
                        //
                        //	Draw child shape with its blend mode if one was set, or ours otherwise
                        //

                        if (c->second->doesDraw()) {
                            if (c->second->getAppearance()->getBlendMode() != lastBlendMode) {
                                lastBlendMode = c->second->getAppearance()->getBlendMode();
                                lastBlendMode.bind();
                            }
                            c->second->draw(state, self, opacity, shader);
                        }
                    }
                }
            }

            void Group::_parseGroupAttributes(const XmlTree &groupNode) {
                if (groupNode.hasAttribute("id")) {
                    _id = groupNode.getAttribute("id").getValue();
                }

                if (groupNode.hasAttribute("transform")) {
                    _groupTransform = util::svg::parseTransform(groupNode.getAttribute("transform").getValue());
                }

                {
                    util::svg::svg_style style = util::svg::parseStyle(groupNode);
                    _opacity = style.opacity;
                }

                //
                //	Now cache all attributes, in case we want to query them later
                //

                for (const auto &attr : groupNode.getAttributes()) {
                    _attributes[attr.getName()] = attr.getValue();
                }
            }

            void Group::_loadGroupsAndShapes(const XmlTree &fromNode) {
                auto self = shared_from_this();
                for (auto childNode = fromNode.begin(); childNode != fromNode.end(); ++childNode) {
                    const std::string tag = childNode->getTag();

                    if (tag == "g") {
                        GroupRef child = make_shared<Group>();

                        child->_parse(*childNode);
                        child->_parent = self;

                        _groups.push_back(child);
                        _groupsById[child->getId()] = child;

                        _drawables.push_back(drawable(child, nullptr));
                    } else if (util::svg::canParseShape(tag)) {
                        ShapeRef shape = make_shared<Shape>();

                        shape->getAppearance()->setParentAppearance(_appearance);
                        shape->parse(*childNode);

                        _shapes.push_back(shape);
                        _shapesById[shape->getId()] = shape;

                        if (shape->isOrigin()) {
                            _originShape = shape;
                        }

                        _drawables.push_back(drawable(nullptr, shape));
                    }
                }
            }

            void Group::_loadAppearances(const XmlTree &fromNode) {
                _appearance->parse(fromNode); // load style attrs from this node

                // if we have a <use /> child node, load attrs from it
                if (fromNode.hasChild("use")) {
                    _appearance->parse(fromNode.getChild("use"));
                }
            }

            void Group::_updateTransform() {
                if (_transformDirty) {
                    _transformDirty = false;
                    
                    const dmat4 S = glm::scale(dvec3(_localTransformScale, 1));
                    const dmat4 T = glm::translate(dvec3(_localTransformPosition, 0));
                    const dmat4 R = glm::rotate(_localTransformAngle, dvec3(0,0,1));
                    _transform = _groupTransform * (T * S * R);
                }
            }

            void Group::_normalize(const dvec2 &documentSize, double documentScale, const dmat4 &worldTransform, const dvec2 &parentWorldOrigin) {
                //
                //	Project shapes to world, and gather their collective bounds in world space
                //

                dmat4 toWorld = worldTransform * _groupTransform;

                Rectd worldBounds(
                        +std::numeric_limits<double>::max(),
                        +std::numeric_limits<double>::max(),
                        -std::numeric_limits<double>::max(),
                        -std::numeric_limits<double>::max()
                );

                if (!_shapes.empty()) {
                    for (const auto &shape : _shapes) {
                        worldBounds.include(shape->_projectToWorld(documentSize, documentScale, toWorld));
                    }
                } else {
                    worldBounds.set(0, 0, documentSize.x, documentSize.y);
                }

                //
                //	determine the origin of this object in world coordinates.
                //	if an 'origin' object was specified, use the center of its world bounds.
                //	otherwise use the center of the bounds of all our shapes.
                //

                dvec2 worldOrigin = worldBounds.getCenter();

                if (_originShape) {
                    worldOrigin = _originShape->_worldBounds.getCenter();
                }

                //
                //	Now make the shapes geometry local to our worldOrigin
                //

                for (auto shape : _shapes) {
                    shape->_makeLocal(worldOrigin);
                }

                //
                //	now set our transform to make our world origin local to our parent's world origin
                //

                _groupTransform = glm::translate(dvec3(worldOrigin - parentWorldOrigin, 0));

                //
                //	finally pass on to children
                //

                for (auto child : _groups) {
                    child->_normalize(documentSize, documentScale, toWorld, worldOrigin);
                }

                //
                //	Now, finally, the root object should move its group transform to local, and make
                //	the group transform identity. This is because nobody 'owns' the root object, so
                //	its transform is inherently local.
                //

                if (isRootGroup()) {
                    _localTransformPosition = worldOrigin - parentWorldOrigin;
                    _groupTransform = dmat4(1);
                }
            }

            dmat4 Group::_worldTransform(dmat4 m) {
                if (GroupRef p = getParentGroup()) {
                    return p->_worldTransform(m);
                }

                _updateTransform();
                return m * _transform;
            }

            cpBB Group::_getBB(dmat4 toWorld) {
                cpBB bb = cpBBInvalid;
                _updateTransform();

                toWorld = toWorld * _transform;

                for (auto shape : getChildShapes()) {
                    cpBB shapeLocalBB = shape->getLocalBB();
                    bb = cpBBExpand(bb, cpBBTransform(shapeLocalBB, toWorld));
                }

                for (auto childGroup : getChildGroups()) {
                    bb = cpBBExpand(bb, childGroup->_getBB(toWorld));
                }

                return bb;
            }

        }
    }
} // core::util::svg
