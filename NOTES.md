# Dev Notes

## Presently

    - Blob is looking good but control needs help
        - @DONE need to prevent/dampen angular rotation about center of mass
        - @DONE need to apply a general left/right force
            - @DONE current approach works, but when targetSpeed == 0, it acts as a damper causing blob mass to cease horizontal motion
        - need to apply "jetpack" force to all particles (maybe central body mass?)

### Todo (HIGH):


## Todo (LOW)
    - use unowned_ptr<> for chipmunk pointers. https://github.com/coryshrmn/cgs/blob/master/include/cgs/unowned_ptr.hpp


## Optimization 
    - observe TerrainWorld::417 cpSpaceBBQuery - I might be able to use cpSPace queries to test if attachments are inside the cut volume. might be a lot faster than my approach, since chipmunk is pretty optimized.

## Bugs

## General:
    - Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
