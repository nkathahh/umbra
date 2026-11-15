// tour.hpp
// The camera route used by the walkthrough: through the first room, out by
// the door, along the long corridor, down the gap and into the lower hall.
// The waypoints sit in the open middle of each passage so the smooth
// Catmull-Rom path through them never touches a wall of levels/level1.txt.
#pragma once

#include <vector>

#include "umbra/polygon.hpp"  // Point2d

namespace umbra {

inline std::vector<Point2d> levelOneTour() {
    return {{1.5, 1.5},  {2.3, 2.7},  {3.5, 4.3},   {3.5, 5.6},  {4.5, 6.5},  {9.5, 6.4},
            {12.8, 6.3}, {14.5, 6.3}, {14.5, 7.6},  {14.5, 10.6}, {14.5, 12.4}, {12.0, 13.5},
            {8.5, 13.0}};
}

}  // namespace umbra
