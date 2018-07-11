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

#include "core/Core.hpp"

#include "elements/ParticleSystem/ParticleSystem.hpp"
#include "elements/Components/ViewportController.hpp"
#include "game/KesslerSyndrome/elements/Background.hpp"
#include "game/KesslerSyndrome/elements/Planet.hpp"
#include "game/KesslerSyndrome/elements/CloudLayerParticleSystem.hpp"
#include "game/KesslerSyndrome/entities/player/Player.hpp"

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

        void load(DataSourceRef stageXmlData);

        BackgroundRef getBackground() const {
            return _background;
        }

        PlanetRef getPlanet() const {
            return _planet;
        }
        
        PlayerRef getPlayer() const {
            return _player;
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
        void applySpaceAttributes(const XmlTree &spaceNode);
        
        void buildGravity(const XmlTree &gravityNode);

        void loadBackground(const XmlTree &planetNode);

        void loadPlanet(const XmlTree &planetNode);
        
        void loadPlayer(const XmlTree &playerNode);

        CloudLayerParticleSystemRef loadCloudLayer(const XmlTree &cloudLayer, int drawLayer);
        
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
        PlayerRef _player;
        vector <CloudLayerParticleSystemRef> _cloudLayers;
        core::RadialGravitationCalculatorRef _gravity;
        elements::ParticleEmitterRef _explosionEmitter;
        elements::ParticleEmitterRef _dustEmitter;
        vector<elements::ViewportControllerRef> _viewportControllers;
        
        double _planetRadius;

    };

} // namespace game

#endif /* GameStage_hpp */
