//
//  TerrainDetail_Svg.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 1/30/18.
//

#ifndef TerrainDetail_Svg_hpp
#define TerrainDetail_Svg_hpp

#include "TerrainDetail.hpp"

namespace elements {
    namespace terrain {
        namespace detail {
            
#pragma mark - Svg
            
            void svg_traverse_elements(XmlTree node, dmat4 worldTransform, vector<pair<string, PolyLine2d>> &idsAndContours);
            
            void svg_traverse_anchors(XmlTree node, dmat4 worldTransform, vector<PolyLine2d> &contours);
            
            void svg_traverse_world(XmlTree node, dmat4 worldTransform, vector<PolyLine2d> &contours);
            
            void svg_descend(XmlTree node, dmat4 worldTransform,
                             vector<PolyLine2d> &shapeContours,
                             vector<PolyLine2d> &anchorContours,
                             vector<pair<string, PolyLine2d>> &elementIdsAndContours);
            
            void svg_load(XmlTree node, dmat4 worldTransform, vector<ShapeRef> &outShapes, vector<AnchorRef> &outAnchors, vector<ElementRef> &outElements);
            
        }
    }
} // end namespace elements::terrain::detail

#endif /* TerrainDetail_Svg_hpp */
