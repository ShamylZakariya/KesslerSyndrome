//
//  SvgParsing.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/11/17.
//
//

#ifndef SvgParsing_hpp
#define SvgParsing_hpp

#include <cinder/Shape2d.h>
#include <cinder/Triangulate.h>

#include "Common.hpp"
#include "MathHelpers.hpp"
#include "Exception.hpp"

namespace cinder {
    class XmlTree;
}

namespace core {
    namespace util {
        namespace svg {

            struct svg_style {
                bool hasFillColor, hasStrokeColor, hasOpacity, hasFillOpacity, hasStrokeOpacity, hasStrokeWidth, hasFillRule;
                Color fillColor, strokeColor;
                double opacity, fillOpacity, strokeOpacity, strokeWidth;
                Triangulator::Winding fillRule;

                svg_style() :
                        hasFillColor(false),
                        hasStrokeColor(false),
                        hasOpacity(false),
                        hasFillOpacity(false),
                        hasStrokeOpacity(false),
                        hasStrokeWidth(false),
                        hasFillRule(false),
                        fillColor(0, 0, 0),
                        strokeColor(0, 0, 0),
                        opacity(1),
                        fillOpacity(1),
                        strokeOpacity(1),
                        strokeWidth(0),
                        fillRule(Triangulator::WINDING_NONZERO) {
                }

                bool isFilled() const {
                    return fillOpacity >= 1.0 / 255.0;
                }

                bool isStroked() const {
                    return strokeWidth > 0 && (strokeOpacity > 1.0 / 255.0);
                }

                friend std::ostream &operator<<(std::ostream &lhs, const svg_style &rhs) {
                    return lhs << "<svg_style fill:(filled: " << (rhs.isFilled() ? "true" : "false") << " color: "
                               << rhs.fillColor << " opacity: " << rhs.fillOpacity << " winding: " << rhs.fillRule
                               << ") stroke: (stroked: " << (rhs.isStroked() ? "true" : "false") << " color: "
                               << rhs.strokeColor << " opacity: " << rhs.strokeOpacity << " width: " << rhs.strokeWidth
                               << ")>";
                }
            };

            double parseNumericAttribute(const std::string &numericAttributeValue);

            Rectd parseViewBoxAttribute(const string &viewportValue);

            /**
             Read the document size and offset of an <svg> node from the viewBox, width and height attributes.
             */
            Rectd parseDocumentFrame(const XmlTree &svgNode);

            svg_style parseStyle(const XmlTree &node);

            /**
             parse an svg transform declaration
             */
            dmat4 parseTransform(const std::string &svgTransform);

            /**
             parse a #FFFF00FF (argb) or #FF00FF (rgb) style color declaration ignoring alpha
             */
            bool parseColor(const std::string &colorValue, Color &color);

            /**
             parse a #FFFF00FF (argb) or #FF00FF (rgb) style color declaration
             */
            bool parseColor(const std::string &colorValue, ColorA &color);

            /**
             attempt to parse an svg path data declaration into a Shape2d
             If the path is lamformed, throws svg::PathParserException
             */
            void parsePath(const std::string &pathString, Shape2d &shape);

            /**
             return true if a shape named @a shapeName can be parsed and converted to shape2d
             */
            bool canParseShape(std::string shapeName);

            /**
             parse a shape described in shapeNode into a shape2d

             @a shape is NOT cleared. You can add successive shapes if desired.

             If the shape's parameters are invalid, ParserException will be thrown
             If the shape's markup is valid, but would result in nothing being drawn ( per SVG specs ) then nothing will be added to shape.
             */
            void parseShape(const XmlTree &shapeNode, Shape2d &shape);

            class ParserException : public core::Exception {
            public:

                ParserException(const std::string &w) : Exception(w) {
                }

                virtual ~ParserException() throw() {
                }

            };


        }
    }
} // end namespace core::util::svg

#endif /* SvgParsing_hpp */
