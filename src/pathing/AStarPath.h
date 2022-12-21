#pragma once

#include "Path.h"
#include "core/Map.h"
#include "diablo/D2Structs.h"
#include "pathing/reduction\PathReducer.h"

#include <cmath>
#include <queue>
#include <set>
#include <vector>

namespace Mapping {
namespace Pathing {

inline int __fastcall Manhattan(Point const& start, Point const& end) {
  return 10 * (std::abs(start.first - end.first) + std::abs(start.second - end.second));
}

inline int __fastcall DiagonalShortcut(Point const& start, Point const& end) {
  int xdist = std::abs(start.first - end.first);
  int ydist = std::abs(start.second - end.second);
  return ((xdist > ydist) ? 14 * ydist + 10 * (xdist - ydist) : 14 * xdist + 10 * (ydist - xdist));
}

inline int __fastcall Chebyshev(Point const& start, Point const& end) {
  int xdist = (start.first - end.first);
  int ydist = (start.second - end.second);
  return (xdist > ydist ? xdist : ydist);
}

inline int __fastcall Euclidean(Point const& start, Point const& end) {
  double dx = (double)(end.first - start.first);
  double dy = (double)(end.second - start.second);
  dx = pow(dx, 2);
  dy = pow(dy, 2);
  return static_cast<int>(sqrt(dx + dy) * 10);
}

inline int __fastcall Slope(Point const& start, Point const& end) {
  double dx = (double)(end.first - start.first);
  double dy = (double)(end.second - start.second);
  return static_cast<int>(dy / dx);
}

inline bool __fastcall checkFlag(int flag) {
  // avoid is a bitwise superset of the other flags, don't need this check
  return (/*((ActMap::Avoid & flag) == ActMap::Avoid) |*/ (((ActMap::BlockWalk | ActMap::BlockPlayer) & flag) > 0));
}

inline int __fastcall EstimateDistance(const Map*, const Point& point, const Point& end) {
  return DiagonalShortcut(point, end);
}

#pragma warning(disable : 4512)

class Node {
 public:
  Node* const parent;
  Point point;
  int g, h;

  Node() : parent(0) {}
  Node(Point s, Node* p, int _g, int _h) : point(s), parent(p), g(_g), h(_h) {}
};
struct NodeComparer {
  inline bool __fastcall operator()(const Node* left, const Node* right) const {
    return (left->g + left->h) > (right->g + right->h);
  }
};

template <class Allocator = std::allocator<Node>>
class AStarPath : public MapPath {
 private:
  Allocator alloc;
  using AllocTrait = std::allocator_traits<Allocator>;

  Map* map;
  Reducing::PathReducer* reducer;
  Estimator estimate;
  Distance distance;

  inline void ReverseList(Node* node, PointList& list) {
    list.clear();
    Node* current = node;
    do {
      list.insert(list.begin(), current->point);
    } while ((current = current->parent) != NULL);
  }

  void FindPath(Point const& start, Point const& end, Node** result, std::vector<Node*>& nodes, bool abs) {
    std::priority_queue<Node*, std::vector<Node*>, NodeComparer> open;
    std::set<Point> closed;
    PointList newNodes;
    Node* begin = AllocTrait::allocate(alloc, 1);
    UnitAny* player = D2CLIENT_GetPlayerUnit();
    DWORD startLvl = player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo;

    // if we don't get a valid node, just return
    if (!begin) return;

    AllocTrait::construct(alloc, begin, Node(start, NULL, 0, estimate(map, start, end)));
    nodes.push_back(begin);
    open.push(begin);
    DWORD ticks = GetTickCount();
    while (!open.empty()) {
      Node* current = open.top();
      open.pop();

      if (closed.find(current->point) != closed.end()) continue;

      if (current->point == end) {
        *result = current;
        return;
      }
      // this is to make d2 not completly freeze on long paths.
      if (GetTickCount() - ticks > 500) {
        map->AllowCritSpace();
        ticks = GetTickCount();
      }
      if (!GameReady() || startLvl != player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo) {
        Log(L"Pather lvl change while pathing");
        return;
      }
      if (Vars.bQuitting) return;

      bool _result = closed.insert(current->point).second;
      assert(_result == true);
      (void)(_result);  // shut up compiler about unused variable warning

      // getOpenNodes should be in map along with a filter
      reducer->GetOpenNodes(current->point, newNodes, end);
      while (!newNodes.empty()) {
        Point point = newNodes.back();
        newNodes.pop_back();

        if (point != end && reducer->Reject(point, abs)) {
          closed.insert(point);
          continue;
        }
        Node* next = AllocTrait::allocate(alloc, 1);
        // if we don't get a valid node, just return
        if (!next) return;
        int pointPenalty = reducer->GetPenalty(point, abs);
        AllocTrait::construct(alloc, next,
                              Node(point, current, current->g + distance(current->point, point) + pointPenalty,
                                   estimate(map, point, end)));
        nodes.push_back(next);
        open.push(next);
      }
    }
  }

 public:
  AStarPath(Map* _map, Reducing::PathReducer* _reducer, Estimator _estimate = EstimateDistance,
            Distance _distance = DiagonalShortcut)
      : map(_map), reducer(_reducer), estimate(_estimate), distance(_distance), alloc(Allocator()) {}

  AStarPath(const AStarPath&) = delete;
  AStarPath& operator=(const AStarPath&) = delete;

  inline Allocator const& GetAllocator() {
    return alloc;
  }
  inline void SetMap(Map* _map) {
    this->map = _map;
  }
  inline void SetPathReducer(Reducing::PathReducer* _reducer) {
    this->reducer = _reducer;
  }

  void GetPath(Point const& _start, Point const& _end, PointList& list, bool abs = true) {
    Node* result = NULL;
    Point start = _start, end = _end;

    // if we don't have a valid start and end, try mutating the points
    if (reducer->Reject(start, abs)) reducer->MutatePoint(start, abs);
    if (reducer->Reject(end, abs)) reducer->MutatePoint(end, abs);

    // if they still get rejected, forget it
    // if(!map->IsValidPoint(start, abs) || !map->IsValidPoint(end, abs)) return;

    std::vector<Node*> nodes;
    FindPath(start, end, &result, nodes, abs);
    if (result) {
      PointList in;
      ReverseList(result, in);
      reducer->Reduce(in, list, abs);
    } else
      list = PointList();

    std::vector<Node*>::iterator lbegin = nodes.begin(), lend = nodes.end();
    for (std::vector<Node*>::iterator it = lbegin; it != lend; it++) {
      AllocTrait::destroy(alloc, (*it));
      AllocTrait::deallocate(alloc, (*it), sizeof(*it));
    }
  }
};

#pragma warning(default : 4512)

}  // namespace Pathing
}  // namespace Mapping
