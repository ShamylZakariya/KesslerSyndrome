//
//  GameLevel.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#ifndef PrecariouslyLevel_hpp
#define PrecariouslyLevel_hpp

#include <cinder/Xml.h>

#include "Core.hpp"

#include "Background.hpp"
#include "Planet.hpp"

namespace precariously {

	SMART_PTR(PrecariouslyLevel);

#pragma mark - PrecariouslyLevel

	class PrecariouslyLevel : public core::Level {
	public:
		PrecariouslyLevel();
		virtual ~PrecariouslyLevel();

		//
		//	Level
		//

		void addObject(core::ObjectRef obj) override;
		void removeObject(core::ObjectRef obj) override;

		//
		//	PrecariouslyLevel
		//

		void load(ci::DataSourceRef levelXmlData);
		BackgroundRef getBackground() const { return _background; }
		PlanetRef getPlanet() const { return _planet; }


	protected:

		// Level
		void onReady() override;
		bool onCollisionBegin(cpArbiter *arb) override;
		bool onCollisionPreSolve(cpArbiter *arb) override;
		void onCollisionPostSolve(cpArbiter *arb) override;
		void onCollisionSeparate(cpArbiter *arb) override;

		// PrecariouslyLevel
		void applySpaceAttributes(XmlTree spaceNode);
		void applyGravityAttributes(XmlTree gravityNode);
		void loadBackground(XmlTree planetNode);
		void loadPlanet(XmlTree planetNode);

	private:

		BackgroundRef _background;
		PlanetRef _planet;
		
	};
	
} // namespace surfacer

#endif /* PrecariouslyLevel_hpp */
