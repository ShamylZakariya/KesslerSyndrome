//
//  Xml.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/17/17.
//
//

#include "Xml.hpp"

#include "Strings.hpp"

using namespace std;

namespace core {
    namespace util {
        namespace xml {

#pragma mark - XmlMultiTree

            /*
            vector<XmlTree> _trees;
            */

            XmlMultiTree::XmlMultiTree() {
            }

            XmlMultiTree::XmlMultiTree(XmlTree tree) :
                    _trees({tree}) {
                _sanityCheck();
            }

            XmlMultiTree::XmlMultiTree(XmlTree firstTree, XmlTree secondTree) :
                    _trees({firstTree, secondTree}) {
                _sanityCheck();
            }

            XmlMultiTree::XmlMultiTree(const initializer_list <XmlTree> &trees) :
                    _trees(trees) {
                _sanityCheck();
            }

            XmlMultiTree::XmlMultiTree(const vector <XmlTree> &trees) :
                    _trees(trees) {
                _sanityCheck();
            }

            bool XmlMultiTree::isElement() const {
                if (!_trees.empty()) {
                    // all our nodes must agree
                    bool isElement = _trees.front().isElement();
                    for (auto it(_trees.begin() + 1), end(_trees.end()); it != end; ++it) {
                        CI_ASSERT_MSG(it->isElement() == isElement, "All mirrored nodes in XmlMultiTree must agree on isElement");
                    }
                    return isElement;
                }
                return false;
            }

            string XmlMultiTree::getTag() const {
                if (!_trees.empty()) {
                    return _trees.front().getTag();
                }
                return "";
            }

            bool XmlMultiTree::hasChild(string name) const {
                if (!_trees.empty()) {
                    for (auto t : _trees) {
                        if (t.hasChild(name)) {
                            return true;
                        }
                    }
                }
                return false;
            }

            XmlMultiTree XmlMultiTree::getChild(string name) const {
                vector <XmlTree> children;
                for (auto t : _trees) {
                    if (t.hasChild(name)) {
                        children.push_back(t.getChild(name));
                    }
                }
                return XmlMultiTree(children);
            }
            
            XmlMultiTree XmlMultiTree::getChild(string name, size_t which) const {
                vector <XmlTree> children;
                for (auto t : _trees) {
                    size_t i = 0;
                    for (const auto &c : t.getChildren()) {
                        if (c->getTag() == name && i == which) {
                            children.push_back(*c);
                        }
                        i++;
                    }
                }

                if (!children.empty()) {
                    return XmlMultiTree(children);
                }
                
                return XmlMultiTree();
            }

            XmlMultiTree XmlMultiTree::findDescentant(string relativePath) const {
                vector <XmlTree> descendents;
                for (auto t : _trees) {
                    auto results = t.find(relativePath);
                    for (auto result : *results) {
                        descendents.push_back(result);
                    }
                }
                return XmlMultiTree(descendents);
            }

            XmlMultiTree XmlMultiTree::findDescendantWithId(string relativePath, string id, string pathSeparator) const {
                return findDescendantWithAttribute(relativePath, "id", id, pathSeparator);
            }

            XmlMultiTree XmlMultiTree::findDescendantWithAttribute(string relativePath, string attribute, string attributeValue, string pathSeparator) const {
                for (auto t : _trees) {
                    auto results = t.find(relativePath);
                    for (auto result : *results) {
                        if (result.hasAttribute(attribute) && result.getAttributeValue<string>(attribute) == attributeValue) {
                            return XmlMultiTree(result);
                        }
                    }
                }
                return XmlMultiTree();
            }

            boost::optional<string> XmlMultiTree::getAttribute(string name) const {
                for (auto t : _trees) {
                    if (t.hasAttribute(name)) {
                        return t.getAttributeValue<string>(name);
                    }
                }
                return boost::none;
            }

            string XmlMultiTree::getAttribute(string name, string fallback) const {
                if (auto v = getAttribute(name)) {
                    return *v;
                }
                return fallback;
            }

            void XmlMultiTree::_sanityCheck() {
                string tag = _trees.front().getTag();
                for (auto it(_trees.begin() + 1), end(_trees.end()); it != end; ++it) {
                    string tag2 = it->getTag();
                    if (tag2 != tag) {
                        stringstream msg;
                        msg << "All XmlTree nodes in XmlMultiTree must have same tag. tag[0]: \"" << tag << "\" tag["
                            << (it - _trees.begin()) << "]: \"" << tag2 << "\"";
                        CI_ASSERT_MSG(tag2 == tag, msg.str().c_str());
                    }
                }
            }


#pragma mark -

            namespace {
                vector<double> readNumericSequence(const string &sequence) {
                    vector<double> values;
                    string token;
                    double value;
                    for (auto c : sequence) {
                        if (c == ',' || c == ' ') {
                            if (!token.empty()) {
                                value = strtod(token.c_str(), nullptr);
                                token.clear();
                                values.push_back(value);
                            }
                            continue;
                        } else {
                            token = token + c;
                        }
                    }
                    if (!token.empty()) {
                        value = strtod(token.c_str(), nullptr);
                        values.push_back(value);
                    }
                    return values;
                }
            }

            boost::optional<XmlTree> findElement(const XmlTree &node, string tag) {
                if (node.isElement() && node.getTag() == tag) {
                    return node;
                } else {
                    for (auto childNode : node) {
                        auto result = findElement(childNode, tag);
                        if (result) {
                            return result;
                        }
                    }
                }
                return boost::none;
            }

            boost::optional<XmlTree> findElementWithId(const XmlTree &node, string id) {
                if (node.isElement() && node.hasAttribute("id") && node.getAttribute("id").getValue() == id) {
                    return node;
                } else {
                    for (auto childNode : node) {
                        auto result = findElementWithId(childNode, id);
                        if (result) {
                            return result;
                        }
                    }
                }
                return boost::none;
            }

            boost::optional<XmlTree> findElement(const XmlTree &node, string tagName, string attributeName, string attributeValue) {
                if (node.isElement() && node.getTag() == tagName && node.hasAttribute(attributeName) && node.getAttribute(attributeName).getValue() == attributeValue) {
                    return node;
                } else {
                    for (auto childNode : node) {
                        auto result = findElement(childNode, tagName, attributeName, attributeValue);
                        if (result) {
                            return result;
                        }
                    }
                }
                return boost::none;
            }

            // read an int numeric attribute
            template<>
            int readNumericAttribute<int>(const XmlMultiTree &node, string attributeName, int defaultValue) {
                auto value = node.getAttribute(attributeName);
                if (value) {
                    return static_cast<int>(strtol(value->c_str(), nullptr, 10));
                }
                return defaultValue;
            }

            // read an unsigned int numeric attribute
            template<>
            unsigned int readNumericAttribute<unsigned int>(const XmlMultiTree &node, string attributeName, unsigned int defaultValue) {
                auto value = node.getAttribute(attributeName);
                if (value) {
                    return static_cast<unsigned int>(strtoul(value->c_str(), nullptr, 10));
                }
                return defaultValue;
            }

            // read an unsigned int numeric attribute
            template<>
            size_t readNumericAttribute<size_t>(const XmlMultiTree &node, string attributeName, size_t defaultValue) {
                auto value = node.getAttribute(attributeName);
                if (value) {
                    return static_cast<size_t>(strtoul(value->c_str(), nullptr, 10));
                }
                return defaultValue;
            }

            vector<double> readNumericSequenceAttribute(const XmlMultiTree &node, string attributeName, vector<double> defaultValue) {
                auto value = node.getAttribute(attributeName);
                if (value) {
                    return readNumericSequence(*value);
                }

                return defaultValue;
            }

            dvec2 readPointAttribute(const XmlMultiTree &node, string attributeName, dvec2 defaultValue) {
                auto value = node.getAttribute(attributeName);
                if (value) {
                    auto r = readNumericSequence(*value);
                    CI_ASSERT_MSG(r.size() == 2, ("dvec2 readPointAttribute expects 2 components in seequence, got: " + str(r.size())).c_str());
                    return dvec2(r[0], r[1]);
                }

                return defaultValue;
            }

            dvec3 readPointAttribute(const XmlMultiTree &node, string attributeName, dvec3 defaultValue) {
                auto value = node.getAttribute(attributeName);
                if (value) {
                    auto r = readNumericSequence(*value);
                    CI_ASSERT_MSG(r.size() == 3, ("dvec3 readPointAttribute expects 3 components in sequence, got: " + str(r.size())).c_str());
                    return dvec3(r[0], r[1], r[2]);
                }

                return defaultValue;
            }

            Color readColorAttribute(const XmlMultiTree &node, string attributeName, Color defaultValue) {
                auto value = node.getAttribute(attributeName);
                if (value) {
                    auto r = readNumericSequence(*value);
                    CI_ASSERT_MSG(r.size() >= 3, ("Color readColorAttribute expects 3 components in sequence, got: " + str(r.size())).c_str());
                    return Color(r[0], r[1], r[2]) / 255;
                }

                return defaultValue;
            }

            ColorA readColorAttribute(const XmlMultiTree &node, string attributeName, ColorA defaultValue) {
                auto value = node.getAttribute(attributeName);
                if (value) {
                    auto r = readNumericSequence(*value);
                    CI_ASSERT_MSG(r.size() >= 3, ("Color readColorAttribute expects 3 components in sequence, got: " + str(r.size())).c_str());
                    return ColorA(r[0], r[1], r[2], r[3]) / 255;
                }

                return defaultValue;
            }

            bool readBoolAttribute(const XmlMultiTree &node, string attributeName, bool defaultValue) {
                auto value = node.getAttribute(attributeName);
                if (value) {
                    string v = *value;
                    return (v == "true") ? true : false;
                }

                return defaultValue;
            }


        }
    }
}
