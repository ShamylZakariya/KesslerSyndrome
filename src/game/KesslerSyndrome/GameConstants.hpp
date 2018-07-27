//
//  GameConstants.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 10/7/17.
//

#ifndef GameConstants_hpp
#define GameConstants_hpp

#include "core/Core.hpp"

namespace game {

    namespace CollisionType {

        /*
         The high 16 bits are a mask, the low are a type_id, the actual type is the logical OR of the two.
         */

        namespace is {
            enum bits {
                SHOOTABLE = 1 << 16,
                TOWABLE = 1 << 17
            };
        }

        enum type_id {

            TERRAIN = 1 | is::SHOOTABLE | is::TOWABLE,
            ANCHOR = 2 | is::SHOOTABLE,
            ENEMY = 3 | is::SHOOTABLE,
            PLAYER = 4 | is::SHOOTABLE,
            WEAPON = 5,
            FLUID = 6,
            DECORATION = 7 | is::TOWABLE | is::SHOOTABLE,
            STATIC_DECORATION = 8 | is::SHOOTABLE,
            SENSOR = 9

        };
    }

    namespace ShapeFilters {

        enum Categories {
            _TERRAIN = 1 << 0,            // 1
            _TERRAIN_PROBE = 1 << 1,    // 2
            _GRABBABLE = 1 << 2,        // 4
            _ANCHOR = 1 << 3,            // 8
            _PLAYER = 1 << 4,            // 16
            _ENEMY = 1 << 5                // 32
        };

        extern cpShapeFilter TERRAIN;
        extern cpShapeFilter TERRAIN_PROBE; // special filter for making queries against terrain
        extern cpShapeFilter ANCHOR;
        extern cpShapeFilter GRABBABLE;
        extern cpShapeFilter PLAYER;
        extern cpShapeFilter ENEMY;
    }

    namespace DrawLayers {
        enum Layer {
            BACKGROUND = 0,
            PLANET = 1000,
            ENEMY = 2000,
            PLAYER = 3000,
            EFFECTS = 4000
        };
    }
    
    namespace ScreenDrawLayers {
        enum Layer {
            BACKGROUND = 0,
            PLAYER = 1000,
            MENU = 2000
        };
    }

    namespace GravitationLayers {
        enum Layer {
            GLOBAL = 1 << 0,
            EXPLOSION = 1 << 1
        };
    }
    
    namespace InputDispatchReceiptOrder {
        enum Order {
            PLAYER = 0
        };
    }


}

#endif /* GameConstants_hpp */
