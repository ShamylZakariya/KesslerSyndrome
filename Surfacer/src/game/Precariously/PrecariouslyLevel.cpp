//
//  GameLevel.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "PrecariouslyLevel.hpp"

#include "PrecariouslyConstants.hpp"
#include "DevComponents.hpp"
#include "Xml.hpp"

using namespace core;

namespace precariously {
	
	namespace {
		
		class MouseBomberComponent : public core::InputComponent {
		public:
			
			MouseBomberComponent(terrain::TerrainObjectRef terrain, int numSpokes, int numRings, double radius, double thickness, double variance, int dispatchReceiptIndex = 0):
			InputComponent(dispatchReceiptIndex),
			_terrain(terrain),
			_numSpokes(numSpokes),
			_numRings(numRings),
			_radius(radius),
			_thickness(thickness),
			_variance(variance)
			{}
			
			bool mouseDown( const ci::app::MouseEvent &event ) override {
				_mouseScreen = event.getPos();
				_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
				
				// create a radial crack
				auto crack = make_shared<RadialCrackGeometry>(_mouseWorld,_numSpokes, _numRings, _radius, _thickness, _variance);

				// create a thing to draw the crack, and dispose of it in 2 seconds
				auto crackDrawer = Object::with("Crack", { make_shared<CrackGeometryDrawComponent>(crack) });
				crackDrawer->setFinished(true, 2);
				getLevel()->addObject(crackDrawer);

				_terrain->getWorld()->cut(crack->getPolygon(), crack->getBB());
				
				return true;
			}
			
		private:
			
			int _numSpokes, _numRings;
			double _radius, _thickness, _variance;
			vec2 _mouseScreen, _mouseWorld;
			terrain::TerrainObjectRef _terrain;
			
		};
		
	}
	

	/*
	 BackgroundRef _background;
	 PlanetRef _planet;
	 */

	PrecariouslyLevel::PrecariouslyLevel():
	Level("Unnamed") {}

	PrecariouslyLevel::~PrecariouslyLevel()
	{}

	void PrecariouslyLevel::addObject(ObjectRef obj) {
		Level::addObject(obj);
	}

	void PrecariouslyLevel::removeObject(ObjectRef obj) {
		Level::removeObject(obj);
	}

	void PrecariouslyLevel::load(ci::DataSourceRef levelXmlData) {

		auto root = XmlTree(levelXmlData);
		auto prefabsNode = root.getChild("prefabs");
		auto levelNode = root.getChild("level");

		setName(levelNode.getAttribute("name").getValue());
		
		//
		//	Load some basic level properties
		//

		auto spaceNode = util::xml::findElement(levelNode, "space");
		CI_ASSERT_MSG(spaceNode, "Expect a <space> node in <level> definition");
		applySpaceAttributes(*spaceNode);

		auto gravityNode = util::xml::findElement(levelNode, "gravity");
		CI_ASSERT_MSG(spaceNode, "Expect a <gravity> node in <level> definition");
		applyGravityAttributes(*gravityNode);
		
		//
		//	Load background
		//
		
		auto backgroundNode = util::xml::findElement(levelNode, "background");
		CI_ASSERT_MSG(backgroundNode, "Expect <background> node in level XML");
		loadBackground(backgroundNode.value());

		//
		//	Load Planet
		//
		
		auto planetNode = util::xml::findElement(levelNode, "planet");
		CI_ASSERT_MSG(planetNode, "Expect <planet> node in level XML");
		loadPlanet(planetNode.value());
		
		
		if (true) {

			addObject(Object::with("Crack", { make_shared<MouseBomberComponent>(_planet, 7, 4, 400, 20, 200) }));
			
		}
		
	}

	void PrecariouslyLevel::onReady() {
		Level::onReady();
	}

	bool PrecariouslyLevel::onCollisionBegin(cpArbiter *arb) {
		return true;
	}

	bool PrecariouslyLevel::onCollisionPreSolve(cpArbiter *arb) {
		return true;
	}

	void PrecariouslyLevel::onCollisionPostSolve(cpArbiter *arb) {
	}

	void PrecariouslyLevel::onCollisionSeparate(cpArbiter *arb) {
	}

	void PrecariouslyLevel::applySpaceAttributes(XmlTree spaceNode) {
		if (spaceNode.hasAttribute("damping")) {
			double damping = util::xml::readNumericAttribute<double>(spaceNode, "damping", 0.95);
			damping = clamp(damping, 0.0, 1.0);
			cpSpaceSetDamping(getSpace()->getSpace(), damping);
		}
	}

	void PrecariouslyLevel::applyGravityAttributes(XmlTree gravityNode) {
		string type = gravityNode.getAttribute("type").getValue();
		if (type == "radial") {
			radial_gravity_info rgi = getRadialGravity();			
			rgi.strength = util::xml::readNumericAttribute<double>(gravityNode, "strength", 10);
			rgi.falloffPower = util::xml::readNumericAttribute<double>(gravityNode, "falloff_power", 0);
			setRadialGravity(rgi);
			setGravityType(RADIAL);

			CI_LOG_D("gravity RADIAL strength: " << rgi.strength << " falloffPower: " << rgi.falloffPower);

		} else if (type == "directional") {
			dvec2 dir = util::xml::readPointAttribute(gravityNode, "dir", dvec2(0,0));
			setDirectionalGravityDirection(dir);
			setGravityType(DIRECTIONAL);

			CI_LOG_D("gravity DIRECTIONAL dir: " << dir );
		}
	}
	
	void PrecariouslyLevel::loadBackground(XmlTree backgroundNode) {
		_background = Background::create(backgroundNode);
		addObject(_background);
	}
	
	void PrecariouslyLevel::loadPlanet(XmlTree planetNode) {
		double friction = util::xml::readNumericAttribute<double>(planetNode, "friction", 1);
		double density = util::xml::readNumericAttribute<double>(planetNode, "density", 1);
		double collisionShapeRadius = 0.1;
		
		const double minDensity = 1e-3;
		density = max(density, minDensity);
		
		const terrain::material terrainMaterial(density, friction, collisionShapeRadius, ShapeFilters::TERRAIN, CollisionType::TERRAIN);
		const terrain::material anchorMaterial(1, friction, collisionShapeRadius, ShapeFilters::ANCHOR, CollisionType::ANCHOR);
		auto world = make_shared<terrain::World>(getSpace(),terrainMaterial, anchorMaterial);
		
		_planet = Planet::create("Planet", world, planetNode, DrawLayers::PLANET);
		addObject(_planet);
		
		// set the physics center of mass to the planet's center
		radial_gravity_info rgi = getRadialGravity();
		rgi.centerOfMass = _planet->getOrigin();
		setRadialGravity(rgi);
	}
	
} // namespace surfacer
