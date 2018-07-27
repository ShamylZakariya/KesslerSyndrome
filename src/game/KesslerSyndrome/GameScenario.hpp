//
//  SurfacerScenario.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/5/17.
//

#ifndef GameStageScenario_hpp
#define GameStageScenario_hpp

#include "core/Core.hpp"
#include "elements/Terrain/Terrain.hpp"

namespace game {


    class GameScenario : public core::Scenario {
    public:

        GameScenario(string stageXmlFile);

        virtual ~GameScenario();

        virtual void setup() override;

        virtual void cleanup() override;

        void reset();

    private:

        string _stageXmlFile;

    };

}

#endif /* GameStageScenario_hpp */
