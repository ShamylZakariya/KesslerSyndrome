# Dev Notes

## PRESENTLY

Viewport::look needs a reference point. Right now, setting a look in world space centers that point in the viewport. 
What I need is a way of saying the center is (0,0) and looking specifying a look reference point as (-100,-200) would put the look target 100px left of center.x, and 200px up from center.y

Do I still need Scenario::clear????
Consider that viewportCompositor always acts on Viewport instances, and screenViewportCompositor always acts on ScreenViewport instances

ScreenComposite
Look into glBlendFuncSeparate - it allows separate SRC/DST factors for RGB vs ALPHA. ci::gl::ScopedBlend supports it.


## BUGS PRIORITY HIGH
- observe TerrainWorld::417 cpSpaceBBQuery - I might be able to use cpSPace queries to test if attachments are inside the cut volume. might be a lot faster than my approach, since chipmunk is pretty optimized.

## BUGS PRIORITY LOW

Consider rewriting DrawDispatcher using raw pointers and not shared_ptr<> ?

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	This is not a bug but the architecture acting as designed. The input is gobbled.
	Is this an ordering issue? who's gobbling the input?
	Is there a robust way to respond to this from a design standpoint?

## NOTES
Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
