//
//  GlslProgLoader.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/5/18.
//

#include "GlslProgLoader.hpp"

#include "Core.hpp"

namespace core {
    namespace util {
        
        gl::GlslProgRef loadGlsl(const DataSourceRef &glslDataSource) {
            BufferRef buffer = glslDataSource->getBuffer();
            std::string bufferStr(static_cast<char*>(buffer->getData()),buffer->getSize());
            vector<std::string> bufferLines = strings::split(bufferStr, "\n");
            
            std::string vertex, fragment;
            std::string *current = nullptr;
            for (auto line : bufferLines) {
                if (line.find("vertex:") != string::npos) {
                    current = &vertex;
                } else if (line.find("fragment:") != string::npos) {
                    current = &fragment;
                } else if (current != nullptr) {
                    *current += line + "\n";
                }
            }
            
            if (vertex.empty()) {
                CI_LOG_E("GLSL file missing \"vertex:\" shader section");
                return nullptr;
            }
            
            if (fragment.empty()) {
                CI_LOG_E("GLSL file missing \"fragment:\" shader section");
                return nullptr;
            }
            
            return gl::GlslProg::create(gl::GlslProg::Format().vertex(vertex).fragment(fragment));
        }

        
        gl::GlslProgRef loadGlslAsset(const std::string &assetName) {
            DataSourceRef asset = app::loadAsset(assetName);
            return loadGlsl(asset);
        }
        
    }
}
