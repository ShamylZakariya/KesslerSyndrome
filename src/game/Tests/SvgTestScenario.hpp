//
//  SvgTestScenario.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/6/17.
//
//

#ifndef SvgTestScenario_hpp
#define SvgTestScenario_hpp

#include "core/Core.hpp"
#include "core/util/Svg.hpp"

using namespace core;

class SvgTestScenario : public Scenario {
public:

    SvgTestScenario();

    virtual ~SvgTestScenario();

    virtual void setup() override;

    virtual void cleanup() override;

    virtual void clear(const render_state &state) override;

    virtual void drawScreen(const render_state &state) override;

    void reset();

protected:

    void testSimpleSvgLoad();

    void testSimpleSvgGroupOriginTransforms();

};

#endif /* SvgTestScenario_hpp */
