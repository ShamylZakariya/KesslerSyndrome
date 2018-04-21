# Dev Notes

## PRESENTLY

Multiple Viewports
Right now, vis culling is done in Stage::update (539)
Needs to be done per-viewport, probably should be done during draw pass, not update()
Do I still need Scenario::clear????
COnsider that viewportCompositor always acts on Viewport instances, and screenViewportCompositor always acts on ScreenViewport instances
ViewportController doesn't scale about mouse correctly, feels like tracking is not at 1.0, or some error is creeping in

Terrain
Consider adding a low-magnitude, high-frequency perlin noise to "roughen up" the terrain perimeter


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
