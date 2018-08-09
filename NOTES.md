# Dev Notes

## Presently

    - Both blob types should have a circle collider on central mass
    - Amorphous blob can compact too much, maybe I need springs pushing from the central mass? maybe central mass needs a collider?

### Todo (HIGH):


## Todo (LOW)
    - use unowned_ptr<> for chipmunk pointers. https://github.com/coryshrmn/cgs/blob/master/include/cgs/unowned_ptr.hpp


## Optimization 
    - observe TerrainWorld::417 cpSpaceBBQuery - I might be able to use cpSPace queries to test if attachments are inside the cut volume. might be a lot faster than my approach, since chipmunk is pretty optimized.

## Bugs

## General:
    - Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
