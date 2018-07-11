//
//  Background.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/7/17.
//

#ifndef Background_hpp
#define Background_hpp

#include "core/Core.hpp"
#include "core/util/Xml.hpp"

#include "game/KesslerSyndrome/GameConstants.hpp"

namespace game {

    SMART_PTR(Background);

    class BackgroundFillDrawComponent : public core::DrawComponent {
    public:

        struct config {
            Color spaceColor;
            Color atmosphereColor;
            double innerRadius;
            double outerRadius;
            int steps;

            config() :
                    spaceColor(100, 100, 120),
                    atmosphereColor(255, 0, 255),
                    innerRadius(500),
                    outerRadius(1000),
                    steps(8) {
            }

            static config parse(const XmlTree &node);
        };

    public:

        BackgroundFillDrawComponent(config c);

        ~BackgroundFillDrawComponent();

        // DrawComponent

        void draw(const core::render_state &state) override;

    private:

        config _config;
        gl::GlslProgRef _shader;
        gl::BatchRef _batch;

    };

    class Background : public core::Object {
    public:

        struct config {
            BackgroundFillDrawComponent::config backgroundFill;

            config() {
            }

            static config parse(XmlTree node);
        };

        static BackgroundRef create(XmlTree backgroundNode);

    public:

        Background();

        ~Background();

    };

}

#endif /* Background_hpp */
