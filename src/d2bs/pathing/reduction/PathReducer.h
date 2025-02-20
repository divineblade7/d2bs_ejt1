#pragma once

#include "d2bs/core/Map.h"

namespace Mapping {
namespace Pathing {
namespace Reducing {

class PathReducer {
 public:
  PathReducer() {}
  virtual ~PathReducer() {}

  PathReducer(const PathReducer&) = delete;
  PathReducer& operator=(const PathReducer&) = delete;

  virtual void Reduce(PointList const& in, PointList& out, bool abs) = 0;
  virtual void GetOpenNodes(Point const& center, PointList& out, Point const& endpoint) = 0;
  virtual bool Reject(Point const& pt, bool abs) = 0;
  virtual int GetPenalty(Point const& pt, bool abs) = 0;
  virtual void MutatePoint(Point& pt, bool abs) = 0;
};

}  // namespace Reducing
}  // namespace Pathing
}  // namespace Mapping
