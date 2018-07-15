//
//  Xml.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/17/17.
//
//

#ifndef Xml_hpp
#define Xml_hpp

#include <cinder/Color.h>
#include <cinder/Xml.h>

#include "core/Common.hpp"
#include "core/MathHelpers.hpp"

namespace core {
    namespace util {
        namespace xml {
            

            // find first element with a given tag
            boost::optional<XmlTree> findElement(const XmlTree &node, const string &tag);

            // find first element with a given id
            boost::optional<XmlTree> findElementWithId(const XmlTree &node, const string &id);

            // find first node in tree of a given tag with a given attribute name/value
            boost::optional<XmlTree> findElement(const XmlTree &node, const string &tagName, const string &attributeName, const string &attributeValue);

            // read a double numeric attribute
            template<typename T>
            T readNumericAttribute(const XmlTree &node, const string &attributeName, T defaultValue) {
                return node.getAttributeValue<T>(attributeName, defaultValue);
            }

            // read a sequence of numeric attributes from an xml node (where a sequence is a list of numbers separated by comma, space, or comma-space)
            vector<double> readNumericSequenceAttribute(const XmlTree &node, const string &attributeName, const vector<double> &defaultValue);

            // read dvec2, dvec3 from attribute values
            dvec2 readPointAttribute(const XmlTree &node, const string &attributeName, dvec2 defaultValue);

            dvec3 readPointAttribute(const XmlTree &node, const string &attributeName, dvec3 defaultValue);

            Color readColorAttribute(const XmlTree &node, const string &attributeName, Color defaultValue);

            ColorA readColorAttribute(const XmlTree &node, const string &attributeName, ColorA defaultValue);

            bool readBoolAttribute(const XmlTree &node, const string &attributeName, bool defaultValue);

        }
    }
}

#endif /* Xml_hpp */
