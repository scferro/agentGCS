#include "RegisterTools.h"
#include "../AgentToolRegistry.h"
#include "AddWaypointTool.h"
#include "AddTakeoffTool.h"
#include "AddLandTool.h"
#include "AddLoiterTool.h"
#include "AddRTLTool.h"
#include "AddSurveyTool.h"
#include "AddTransitionTool.h"
#include "UpdateMissionItemTool.h"
#include "DeleteMissionItemTool.h"
#include "MoveItemTool.h"
#include "ReorderItemTool.h"
#include "GuidedTakeoffTool.h"
#include "GuidedGoToTool.h"
#include "GuidedRTLTool.h"
#include "GuidedLandTool.h"
#include "GuidedOrbitTool.h"

void registerAllTools(AgentToolRegistry* registry) {
    if (!registry) return;

    // Mission add tools
    registry->registerTool(new AddWaypointTool(registry));
    registry->registerTool(new AddTakeoffTool(registry));
    registry->registerTool(new AddLandTool(registry));
    registry->registerTool(new AddLoiterTool(registry));
    registry->registerTool(new AddRTLTool(registry));
    registry->registerTool(new AddSurveyTool(registry));
    registry->registerTool(new AddTransitionTool(registry));

    // Mission edit tools (mission mode only)
    registry->registerTool(new UpdateMissionItemTool(registry));
    registry->registerTool(new DeleteMissionItemTool(registry));
    registry->registerTool(new MoveItemTool(registry));
    registry->registerTool(new ReorderItemTool(registry));

    // Guided action tools (command mode only)
    registry->registerTool(new GuidedTakeoffTool(registry));
    registry->registerTool(new GuidedGoToTool(registry));
    registry->registerTool(new GuidedRTLTool(registry));
    registry->registerTool(new GuidedLandTool(registry));
    registry->registerTool(new GuidedOrbitTool(registry));
}