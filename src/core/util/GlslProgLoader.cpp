//
//  GlslProgLoader.cpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/5/18.
//

#include <cinder/app/App.h>

#include "core/util/GlslProgLoader.hpp"
#include "core/Strings.hpp"

using namespace cinder;
using namespace std;

namespace core {
    namespace util {
        
        namespace {
            
            void replace( string &str, const string &token, const string &with )
            {
                std::string::size_type pos = str.find( token );
                while( pos != std::string::npos )
                {
                    str.replace( pos, token.size(), with );
                    pos = str.find( token, pos );
                }
            }

            
            void apply_substitutions(string &src, const map<string,string> &substitutions) {
                for (const auto &s : substitutions) {
                    replace(src, s.first, s.second);
                }
            }
            
        }
        
        gl::GlslProgRef loadGlsl(const DataSourceRef &glslDataSource, const map<string,string> &substitutions) {
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
            
            CI_ASSERT_MSG(!vertex.empty(), "GLSL file missing \"vertex:\" shader section");
            CI_ASSERT_MSG(!fragment.empty(), "GLSL file missing \"fragment:\" shader section");
            
            apply_substitutions(vertex, substitutions);
            apply_substitutions(fragment, substitutions);
            
            return gl::GlslProg::create(gl::GlslProg::Format().vertex(vertex).fragment(fragment));
        }

        
        gl::GlslProgRef loadGlslAsset(const std::string &assetName, const map<string,string> &substitutions) {
            DataSourceRef asset = app::loadAsset(assetName);
            return loadGlsl(asset, substitutions);
        }
        
    }
}
