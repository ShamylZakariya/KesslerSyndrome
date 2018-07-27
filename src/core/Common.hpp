//
//  Common.hpp
//  Kessler Syndrome
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#ifndef Common_h
#define Common_h

using namespace std;
using namespace ci;
//using namespace ci::app;

#define CLASS_NAME(ptr)        ((typeid(*(ptr))).name())
#define SMART_PTR(cname)    class cname; typedef std::shared_ptr< class cname > cname ## Ref; typedef std::weak_ptr< class cname > cname ## WeakRef;

#define ALPHA_EPSILON (1.0/255.0)

#endif /* Common_h */
