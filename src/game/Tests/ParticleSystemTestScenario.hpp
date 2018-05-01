//
//  ParticleSystemTestScenario.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/2/17.
//

#ifndef ParticleSystemTestScenario_hpp
#define ParticleSystemTestScenario_hpp

#include <cinder/Rand.h>

#include "Core.hpp"
#include "Svg.hpp"

#include "Terrain.hpp"
#include "ParticleSystem.hpp"
#include "ViewportController.hpp"


class ParticleSystemTestScenario : public core::Scenario {
public:

    ParticleSystemTestScenario();

    virtual ~ParticleSystemTestScenario();

    virtual void setup() override;

    virtual void cleanup() override;

    virtual void update(const core::time_state &time) override;

    virtual void clear(const core::render_state &state) override;

    virtual void drawScreen(const core::render_state &state) override;

    virtual bool keyDown(const app::KeyEvent &event) override;

    void reset();

protected:

    void buildExplosionPs();

private:

    elements::ParticleSystemRef _explosionPs;
    elements::ParticleEmitterRef _explosionEmitter;
    elements::ParticleEmitter::emission_id _explosionEmissionId;
    elements::ViewportControllerRef _viewportController;
};

#endif /* ParticleSystemTestScenario_hpp */
