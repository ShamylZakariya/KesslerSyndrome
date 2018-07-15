//
//  Xml.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/17/17.
//
//

#include "core/util/Xml.hpp"
#include "core/Strings.hpp"

using namespace std;

namespace core {
    namespace util {
        namespace xml {

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

            boost::optional<XmlTree> findElement(const XmlTree &node, const string &tag) {
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

            boost::optional<XmlTree> findElementWithId(const XmlTree &node, const string &id) {
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

            boost::optional<XmlTree> findElement(const XmlTree &node, const string &tagName, const string &attributeName, const string &attributeValue) {
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

            vector<double> readNumericSequenceAttribute(const XmlTree &node, const string &attributeName, const vector<double> &defaultValue) {
                if (node.hasAttribute(attributeName)) {
                    return readNumericSequence(node.getAttribute(attributeName));
                }

                return defaultValue;
            }

            dvec2 readPointAttribute(const XmlTree &node, const string &attributeName, dvec2 defaultValue) {
                if (node.hasAttribute(attributeName)) {
                    auto r = readNumericSequence(node.getAttribute(attributeName));
                    CI_ASSERT_MSG(r.size() == 2, ("dvec2 readPointAttribute expects 2 components in seequence, got: " + str(r.size())).c_str());
                    return dvec2(r[0], r[1]);
                }

                return defaultValue;
            }

            dvec3 readPointAttribute(const XmlTree &node, const string &attributeName, dvec3 defaultValue) {
                if (node.hasAttribute(attributeName)) {
                    auto r = readNumericSequence(node.getAttribute(attributeName));
                    CI_ASSERT_MSG(r.size() == 3, ("dvec3 readPointAttribute expects 3 components in sequence, got: " + str(r.size())).c_str());
                    return dvec3(r[0], r[1], r[2]);
                }

                return defaultValue;
            }

            Color readColorAttribute(const XmlTree &node, const string &attributeName, Color defaultValue) {
                if (node.hasAttribute(attributeName)) {
                    auto r = readNumericSequence(node.getAttribute(attributeName));
                    CI_ASSERT_MSG(r.size() >= 3, ("Color readColorAttribute expects 3 components in sequence, got: " + str(r.size())).c_str());
                    return Color(r[0], r[1], r[2]) / 255;
                }

                return defaultValue;
            }

            ColorA readColorAttribute(const XmlTree &node, const string &attributeName, ColorA defaultValue) {
                if (node.hasAttribute(attributeName)) {
                    auto r = readNumericSequence(node.getAttribute(attributeName));
                    CI_ASSERT_MSG(r.size() >= 3, ("Color readColorAttribute expects 3 components in sequence, got: " + str(r.size())).c_str());
                    return ColorA(r[0], r[1], r[2], r[3]) / 255;
                }

                return defaultValue;
            }

            bool readBoolAttribute(const XmlTree &node, const string &attributeName, bool defaultValue) {
                if (node.hasAttribute(attributeName)) {
                    string v = strings::lowercase(node.getAttribute(attributeName));
                    return (v == "true") ? true : false;
                }

                return defaultValue;
            }
        }
    }
}
