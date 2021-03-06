//
//  BaseParticleSystem.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/18/17.
//

#include "elements/ParticleSystem/BaseParticleSystem.hpp"

using namespace core;
namespace elements {

#pragma mark - Data


    const vec2 TexCoords[4] =
            {
                    vec2(0, 0),
                    vec2(1, 0),
                    vec2(1, 1),
                    vec2(0, 1)
            };

    namespace {

        const float r2 = 0.5f;
        const float r4 = 0.25f;

        const vec2 AtlasOffset_None[1] = {vec2(0, 0)};

        const vec2 AtlasOffset_TwoByTwo[4] =
                {
                        vec2(0 * r2, 0 * r2), vec2(1 * r2, 0 * r2),
                        vec2(0 * r2, 1 * r2), vec2(1 * r2, 1 * r2)
                };

        const vec2 AtlasOffset_FourByFour[16] =
                {
                        vec2(0 * r4, 0 * r4), vec2(1 * r4, 0 * r4), vec2(2 * r4, 0 * r4), vec2(3 * r4, 0 * r4),
                        vec2(0 * r4, 1 * r4), vec2(1 * r4, 1 * r4), vec2(2 * r4, 1 * r4), vec2(3 * r4, 1 * r4),
                        vec2(0 * r4, 2 * r4), vec2(1 * r4, 2 * r4), vec2(2 * r4, 2 * r4), vec2(3 * r4, 2 * r4),
                        vec2(0 * r4, 3 * r4), vec2(1 * r4, 3 * r4), vec2(2 * r4, 3 * r4), vec2(3 * r4, 3 * r4)
                };

        const vec2 *AtlasOffsetsByAtlasType[3] = {
                AtlasOffset_None,
                AtlasOffset_TwoByTwo,
                AtlasOffset_FourByFour
        };

        const float AtlasScalingByAtlasType[3] = {
                1, r2, r4
        };
    }


#pragma mark - Atlas

    Atlas::Type Atlas::fromString(std::string typeStr) {
        typeStr = core::strings::lowercase(typeStr);
        if (typeStr == "twobytwo" || typeStr == "2x2") {
            return TwoByTwo;
        } else if (typeStr == "fourbyfour" || typeStr == "4x4") {
            return FourByFour;
        }
        return None;
    }

    std::string Atlas::toString(elements::Atlas::Type t) {
        switch (t) {
            case None:
                return "None";
            case TwoByTwo:
                return "TwoByTwo";
            case FourByFour:
                return "FourByFour";
        }
        return "None";
    }

    const vec2 *Atlas::AtlasOffsets(Type atlasType) {
        return AtlasOffsetsByAtlasType[atlasType];
    }

    float Atlas::AtlasScaling(Type atlasType) {
        return AtlasScalingByAtlasType[atlasType];
    }
    
    size_t Atlas::ElementCount(Type atlasType) {
        switch (atlasType) {
            case None:
                return 1;
            case TwoByTwo:
                return 4;
            case FourByFour:
                return 16;
        }
        return 1;
    }


#pragma mark - BaseParticleSimulation

    BaseParticleSimulation::BaseParticleSimulation() {
    }

#pragma mark - BaseParticleSystemDrawComponent

    BaseParticleSystemDrawComponent::config BaseParticleSystemDrawComponent::config::parse(const XmlTree &node) {
        config c;
        c.drawLayer = util::xml::readNumericAttribute<int>(node, "drawLayer", c.drawLayer);
        return c;
    }

    BaseParticleSystemDrawComponent::BaseParticleSystemDrawComponent(config c) :
            DrawComponent(c.drawLayer, VisibilityDetermination::FRUSTUM_CULLING),
            _config(c)
    {
    }

    cpBB BaseParticleSystemDrawComponent::getBB() const {
        if (BaseParticleSimulationRef s = _simulation.lock()) {
            return s->getBB();
        }
        return cpBBInvalid;
    }

#pragma mark - BaseParticleSystem

    BaseParticleSystem::BaseParticleSystem(string name) :
            Object(name) {
    }

    void BaseParticleSystem::onReady(StageRef stage) {
        Object::onReady(stage);
        CI_ASSERT_MSG(_simulation, "Expect a BaseParticleSimulation to be among installed components at time of onReady()");
        for (auto dc : getDrawComponents()) {
            if (BaseParticleSystemDrawComponentRef psdc = dynamic_pointer_cast<BaseParticleSystemDrawComponent>(dc)) {
                psdc->setSimulation(_simulation);
            }
        }
    }

    void BaseParticleSystem::addComponent(ComponentRef component) {
        Object::addComponent(component);
        if (BaseParticleSimulationRef sim = dynamic_pointer_cast<BaseParticleSimulation>(component)) {
            _simulation = sim;
        }
    }

} // end namespace elements
