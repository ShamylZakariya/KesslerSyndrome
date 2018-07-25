//
//  GlslProgLoader.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 4/5/18.
//

#ifndef GlslProgLoader_hpp
#define GlslProgLoader_hpp

#include <map>

#include <cinder/gl/GlslProg.h>

namespace core {
    namespace util {

        ci::gl::GlslProgRef loadGlsl(const ci::DataSourceRef &glslDataSource, const std::map<std::string,std::string> &substitutions);

        ci::gl::GlslProgRef loadGlslAsset(const std::string &assetName, const std::map<std::string,std::string> &substitutions = std::map<std::string,std::string>());
        
    }
}

#endif /* GlslProgLoader_hpp */
