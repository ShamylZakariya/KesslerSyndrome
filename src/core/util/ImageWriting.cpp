//
//  ImageWriting.cpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/27/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#include "ImageWriting.hpp"

#include "core/InputDispatcher.hpp"
#include "core/Strings.hpp"

namespace core {
    namespace util {
        
        namespace {
            
            const string SaveFormat = "png";
            
            // https://stackoverflow.com/questions/33224941/expanding-user-path-with-boostfilesystem
            fs::path expand(fs::path in) {
                if (in.string().size() < 1) {
                    return in;
                }
                
                const char *home = getenv("HOME");
                if (!home) {
                    CI_LOG_E("error: HOME variable not set.");
                    throw std::invalid_argument ("error: HOME environment variable not set.");
                }
                
                string s = in.string();
                if (s[0] == '~') {
                    s = string(home) + s.substr (1, s.size () - 1);
                    return fs::path (s);
                } else {
                    return in;
                }
            }

            fs::path create_path(fs::path folderPath, const string &namingPrefix, const string format) {
                // expand "~" at root of path
                folderPath = expand(folderPath);
                
                // establish that folderPath exists
                fs::create_directories(folderPath);
                
                size_t index = 0;
                fs::path fullPath;

                do {
                    fullPath = folderPath / (namingPrefix + "_" + str(index++) + "." + format);
                } while (fs::exists(fullPath));
                
                return fullPath;
            }
            
        }
        
        void saveScreenshot(const fs::path &folderPath, const string &namingPrefix) {
            fs::path fullPath = create_path(folderPath, namingPrefix, SaveFormat);
            writeImage(fullPath.string(), app::copyWindowSurface(), ImageTarget::Options(), SaveFormat);
        }
        
        void saveFbo(const ci::gl::FboRef &fbo, const fs::path &folderPath, const string &namingPrefix) {
            fs::path fullPath = create_path(folderPath, namingPrefix, SaveFormat);
            writeImage(fullPath, fbo->getColorTexture()->createSource(), ImageTarget::Options(), SaveFormat);
        }
        
        void saveFboIfKeyPressed(const ci::gl::FboRef &fbo, int keyCode, const fs::path &folderPath, const string &namingPrefix) {
            if (InputDispatcher::get()->wasKeyPressed(keyCode)) {
                saveFbo(fbo, folderPath, namingPrefix);
            }
        }

    }
}
