//
//  SurfacerScenario.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/5/17.
//

#ifndef GameStageScenario_hpp
#define GameStageScenario_hpp

#include "Core.hpp"
#include "Terrain.hpp"

namespace game {


    class GameScenario : public core::Scenario {
    public:

        GameScenario(string stageXmlFile);

        virtual ~GameScenario();

        virtual void setup() override;

        virtual void cleanup() override;

        virtual void drawScreen(const core::render_state &state) override;

        virtual bool keyDown(const app::KeyEvent &event) override;

        void reset();

    private:

        string _stageXmlFile;

    };

}

#endif /* GameStageScenario_hpp */
