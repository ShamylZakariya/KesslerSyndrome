//
//  Xml.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/17/17.
//
//

#ifndef Xml_hpp
#define Xml_hpp

#include <cinder/Xml.h>

#include "Core.hpp"

namespace core {
    namespace util {
        namespace xml {

            /**
             @class XmlMultiTree
             XmlMultiTree is used when we want two or more mirrored XML DOMs, where "closer" ones override values in "farther" ones.
             An example of usage would be prefabs for game object configurations. A prefab element (P) would be defined somewhere, and an overriding
             per-instance element (I) would also be defined.
             The user would create a XmlMultiTree using { I,P } in that order.
             Then any requests for values in the represented DOM would first query I, and if not available there, would query P.
             The idea is that the prefab node (P) would provide a complete configuration for the game object, and the instance node (I) would
             override a few attributes to customize behavior for that instance.
             XmlMultiTree represents the minimal interface to be useful in Kessler Syndrome. No point re-implementing all the XmlTree methods which aren't used.
             */
            class XmlMultiTree {
            public:

                XmlMultiTree();

                XmlMultiTree(XmlTree tree);

                XmlMultiTree(XmlTree firstTree, XmlTree secondTree);

                XmlMultiTree(const initializer_list <XmlTree> &trees);

                XmlMultiTree(const vector <XmlTree> &trees);

                const vector <XmlTree> getTrees() const {
                    return _trees;
                }

                bool isElement() const;

                string getTag() const;

                bool hasChild(string name) const;

                XmlMultiTree getChild(string name) const;
                
                XmlMultiTree getChild(string name, size_t which) const;

                XmlMultiTree findDescentant(string relativePath) const;

                XmlMultiTree findDescendantWithId(string relativePath, string id, string pathSeparator = "/") const;

                XmlMultiTree findDescendantWithAttribute(string relativePath, string attribute, string attributeValue, string pathSeparator = "/") const;

                boost::optional<string> getAttribute(string name) const;

                string getAttribute(string name, string fallback) const;

                operator bool() const {
                    return !_trees.empty();
                }

                bool operator!() const {
                    return _trees.empty();
                }

            private:

                void _sanityCheck();

                vector <XmlTree> _trees;

            };


            // find first element with a given tag
            boost::optional<XmlTree> findElement(const XmlTree &node, string tag);

            // find first element with a given id
            boost::optional<XmlTree> findElementWithId(const XmlTree &node, string id);

            // find first node in tree of a given tag with a given attribute name/value
            boost::optional<XmlTree> findElement(const XmlTree &node, string tagName, string attributeName, string attributeValue);

            // read a double numeric attribute
            template<typename T>
            T readNumericAttribute(const XmlMultiTree &node, string attributeName, T defaultValue) {
                auto value = node.getAttribute(attributeName);
                if (value) {
                    return strtod(value->c_str(), nullptr);
                }
                return defaultValue;
            }

            // specializations for ints, etc
            template<>
            int readNumericAttribute<int>(const XmlMultiTree &node, string attributeName, int defaultValue);

            template<>
            unsigned int readNumericAttribute<unsigned int>(const XmlMultiTree &node, string attributeName, unsigned int defaultValue);

            template<>
            size_t readNumericAttribute<size_t>(const XmlMultiTree &node, string attributeName, size_t defaultValue);

            // read a sequence of numeric attributes from an xml node (where a sequence is a list of numbers separated by comma, space, or comma-space)
            vector<double> readNumericSequenceAttribute(const XmlMultiTree &node, string attributeName, vector<double> defaultValue);

            // read dvec2, dvec3 from attribute values
            dvec2 readPointAttribute(const XmlMultiTree &node, string attributeName, dvec2 defaultValue);

            dvec3 readPointAttribute(const XmlMultiTree &node, string attributeName, dvec3 defaultValue);

            Color readColorAttribute(const XmlMultiTree &node, string attributeName, Color defaultValue);

            ColorA readColorAttribute(const XmlMultiTree &node, string attributeName, ColorA defaultValue);

            bool readBoolAttribute(const XmlMultiTree &node, string attributeName, bool defaultValue);

        }
    }
}

#endif /* Xml_hpp */
