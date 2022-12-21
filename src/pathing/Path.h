#pragma once

#if defined(__WINE__) || defined(__unix__)
#include "core/Map.h"
#include "reduction/PathReducer.h"
#else
#include "core/Map.h"
#include "reduction\PathReducer.h"
#endif

namespace Mapping {
namespace Pathing {

typedef int(__fastcall* Estimator)(Map const*, Point const&, Point const&);
typedef int(__fastcall* Distance)(Point const&, Point const&);

class MapPath {
  public:
    virtual void GetPath(Point const& start, Point const& end, PointList& list, bool abs) = 0;

    virtual void SetMap(Map* map) = 0;
    virtual void SetPathReducer(Reducing::PathReducer* reducer) = 0;
};

} // namespace Pathing
} // namespace Mapping
