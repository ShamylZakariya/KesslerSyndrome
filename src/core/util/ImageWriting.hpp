//
//  ImageWriting.hpp
//  KesslerSyndrome
//
//  Created by Shamyl Zakariya on 7/27/18.
//  Copyright © 2018 Shamyl Zakariya. All rights reserved.
//

#ifndef ImageWriting_hpp
#define ImageWriting_hpp

#include <cinder/app/App.h>
#include <cinder/gl/Fbo.h>

#include "core/Common.hpp"

namespace core {
    namespace util {

        /**
         Saves a PNG screenshot of the window.
         @param folderPath the folder to contain the screenshot
         @param namingPrefix the naming of the screenshot file, e.g., "Screenshot"
         
         If namingPrefix is "Screenshot", the saved file will be named "Screenshot_X.png" where X will be the lowest
         index that doesn't clobber an existing screenshot, e.g., "Screenshot_0.png", "Screenshot_1.png", etc.
         */
        void saveScreenshot(const fs::path &folderPath, const string &namingPrefix);
        
        /**
         Saves a PNG of an Fbo to disk
         @param folderPath the folder to contain the screenshot
         @param namingPrefix the naming of the screenshot file, e.g., "TestFboRender"
         
         If namingPrefix is "TestFboRender", the saved file will be named "TestFboRender_X.png" where X will be the lowest
         index that doesn't clobber an existing screenshot, e.g., "TestFboRender_0.png", "TestFboRender_1.png", etc.
         */
        void saveFbo(const ci::gl::FboRef &fbo, const fs::path &folderPath, const string &namingPrefix);

        /**
         Saves a PNG of an Fbo to disk, if a given key was pressed.
         @param folderPath the folder to contain the screenshot
         @param namingPrefix the naming of the screenshot file, e.g., "TestFboRender"

         If namingPrefix is "TestFboRender", the saved file will be named "TestFboRender_X.png" where X will be the lowest
         index that doesn't clobber an existing screenshot, e.g., "TestFboRender_0.png", "TestFboRender_1.png", etc.

         This is meant to be used for debugging rendering output, and should most likely be called in a draw() call. E.g.:

         @code Foo::draw() {
            auto fbo = renderFbo();
            saveFboIfKeyPressed(fbo, app::KeyEvent::KEY_space, "~/Tmp/shots/", "test_fbo");
            useFbo(fbo);
         }
         
         */
        void saveFboIfKeyPressed(const ci::gl::FboRef &fbo, int keyCode, const fs::path &folderPath, const string &namingPrefix);

    }
}

#endif /* ImageWriting_hpp */
