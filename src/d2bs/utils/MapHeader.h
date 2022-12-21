#include "d2bs/core\ActMap.h"
#include "d2bs/core\Map.h"
#include "d2bs/pathing\AStarPath.h"
#include "d2bs/pathing\reduction\JSPathReducer.h"
#include "d2bs/pathing\reduction\NoPathReducer.h"
#include "d2bs/pathing\reduction\TeleportPathReducer.h"
#include "d2bs/pathing\reduction\WalkPathReducer.h"

// TODO: Remove these using namespaces because bloating the global namespace is bad. ~ ejt
using namespace Mapping;
using namespace Pathing;
using namespace Reducing;
