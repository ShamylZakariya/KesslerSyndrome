# Dev Notes

## Presently

- Do I still need Scenario::clear????

- Having DrawComponent subclasses override getLayer is stupid, compared to having them make a call to setLayer, or some such. 
    - confirm that getBB() is sanely implemented - e.g., maybe default implementation should return cpBBInvalid for NEVER, infinity for ALWAYS, and ... assert false for CULLING?
    - can I kill Object::getBB()???? It might not be getting called now

- Greeble atlasses need some kind of probability bias

## Todo

- Make some kind of component which simplifies toggling of Gizmo masks at runtime

## Optimization 
- observe TerrainWorld::417 cpSpaceBBQuery - I might be able to use cpSPace queries to test if attachments are inside the cut volume. might be a lot faster than my approach, since chipmunk is pretty optimized.

## Bugs

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	This is not a bug but the architecture acting as designed. The input is gobbled.
	Is this an ordering issue? who's gobbling the input?
	Is there a robust way to respond to this from a design standpoint?

## General

Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
