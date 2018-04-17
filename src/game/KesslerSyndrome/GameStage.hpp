//
//  GameStage.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#ifndef GameStage_hpp
#define GameStage_hpp

#include <cinder/Xml.h>

#include "Core.hpp"

#include "Background.hpp"
#include "Planet.hpp"
#include "CloudLayerParticleSystem.hpp"
#include "ParticleSystem.hpp"

namespace game {

    SMART_PTR(GameStage);

#pragma mark - GameStage

    class GameStage : public core::Stage {
    public:
        GameStage();

        virtual ~GameStage();

        //
        //	GameStage
        //

        void load(ci::DataSourceRef stageXmlData);

        BackgroundRef getBackground() const {
            return _background;
        }

        PlanetRef getPlanet() const {
            return _planet;
        }


    protected:

        // Stage
        void onReady() override;

        void update(const core::time_state &time) override;

        bool onCollisionBegin(cpArbiter *arb) override;

        bool onCollisionPreSolve(cpArbiter *arb) override;

        void onCollisionPostSolve(cpArbiter *arb) override;

        void onCollisionSeparate(cpArbiter *arb) override;

        // GameStage
        void applySpaceAttributes(XmlTree spaceNode);

        void buildGravity(XmlTree gravityNode);

        void loadBackground(XmlTree planetNode);

        void loadPlanet(XmlTree planetNode);

        CloudLayerParticleSystemRef loadCloudLayer(XmlTree cloudLayer, int drawLayer);
        
        void loadGreebleSystem(const core::util::xml::XmlMultiTree &greebleNode);

        void buildExplosionParticleSystem();

        void buildDustParticleSystem();

        void cullRubble();

        void makeSleepersStatic();

        void performExplosion(dvec2 world);
        
        void handleTerrainTerrainContact(cpArbiter *arbiter);

    private:

        BackgroundRef _background;
        PlanetRef _planet;
        vector <CloudLayerParticleSystemRef> _cloudLayers;
        core::RadialGravitationCalculatorRef _gravity;
        particles::ParticleEmitterRef _explosionEmitter;
        particles::ParticleEmitterRef _dustEmitter;
        core::ViewportControllerRef _viewportController;

    };

} // namespace game

#endif /* GameStage_hpp */
