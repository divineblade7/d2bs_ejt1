#include "core\ActMap.h"
#include "core\Map.h"
#include "pathing\AStarPath.h"
#include "pathing\reduction\JSPathReducer.h"
#include "pathing\reduction\NoPathReducer.h"
#include "pathing\reduction\TeleportPathReducer.h"
#include "pathing\reduction\WalkPathReducer.h"

// TODO: Remove these using namespaces because bloating the global namespace is bad. ~ ejt
using namespace Mapping;
using namespace Pathing;
using namespace Reducing;
