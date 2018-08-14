# Dev Notes

## Presently

    - Blob is looking good
        - test how well my maths accommodate, e.g., higher gravity, different sizes, etc
        - @DONE "shephard" - when a particle gets too far away from the blob central body, turn off that particle's collision detection and let the spring bring it back. Might want to scale to zero while this is happening.
        - rendering!

## Todo (HIGH):
    - need multiple ids for controller input matchings - different PS4 controllers have differing vendor IDs!
        "Sony Interactive Entertainment Wireless Controller"


## Todo (LOW)
    - use unowned_ptr<> for chipmunk pointers. https://github.com/coryshrmn/cgs/blob/master/include/cgs/unowned_ptr.hpp


## Optimization 
    - observe TerrainWorld::417 cpSpaceBBQuery - I might be able to use cpSPace queries to test if attachments are inside the cut volume. might be a lot faster than my approach, since chipmunk is pretty optimized.

## Bugs

## General:
    - Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
