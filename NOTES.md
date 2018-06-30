# Dev Notes

## Presently

- Player Features
    - needs weapon
    - particle emitter needs a "velocity" value which is added to each emitted particle to accommodate moving emitters

- Player Bugs
    - group origin dot is being drawn! Why!?

- need a viewport controller which follows Player.
    - should "aim" camera somewhere between player and the planetoid center
    - should manage an up vector
    
- Make my #includes use paths, not just the filename

- Svg Group needs a "find element by Id", because the path thing works but is awful

- Kill XmlMultiTree - I'm not even using it, and it adds complexity.

- Explosions should be re-done. 
    - erase a "noisy" circle at impact point, and use spider webbing to make rubble around the noisy center.

## Todo


## Optimization 
- observe TerrainWorld::417 cpSpaceBBQuery - I might be able to use cpSPace queries to test if attachments are inside the cut volume. might be a lot faster than my approach, since chipmunk is pretty optimized.

## Bugs

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	This is not a bug but the architecture acting as designed. The input is gobbled.
	Is this an ordering issue? who's gobbling the input?
	Is there a robust way to respond to this from a design standpoint?

## General

Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
