# Dev Notes

## Presently

This player sucks. What about a blob player like the Shoggoths in Surfacer?
    1) Get house in order - do the yak shaving below
    2) Read up Surfacer code for Shoggoths
    3) Make ParticleSystem have option to render to texture, with a composite pass

### Todo (HIGH):
    - DONE: Make my #includes use paths, not just the filename
    - DONE: Switch to c++ 14? 17? right now we'r eusing c++ 11
    - DONE: Kill XmlMultiTree - I'm not even using it, and it adds complexity.
    - DONE: Svg Group needs a "find element by Id", because the path thing works but is awful
    
    - Make ParticleSystem have an option to render to texture, run shaders against it, and then composite (this will enable me to do the nice MotileFluid stuff)


## Todo (LOW)
    - use unowned_ptr<> for chipmunk pointers. https://github.com/coryshrmn/cgs/blob/master/include/cgs/unowned_ptr.hpp


## Optimization 
- observe TerrainWorld::417 cpSpaceBBQuery - I might be able to use cpSPace queries to test if attachments are inside the cut volume. might be a lot faster than my approach, since chipmunk is pretty optimized.

## Bugs

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	This is not a bug but the architecture acting as designed. The input is gobbled.
	Is this an ordering issue? who's gobbling the input?
	Is there a robust way to respond to this from a design standpoint?

## General

Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
