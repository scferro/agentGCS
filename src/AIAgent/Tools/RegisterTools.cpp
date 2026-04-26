#include "RegisterTools.h"
#include "../AgentToolRegistry.h"
#include "AddWaypointTool.h"
#include "AddTakeoffTool.h"
#include "AddLandTool.h"
#include "AddLoiterTool.h"
#include "AddRTLTool.h"
#include "AddSurveyTool.h"
#include "AddTransitionTool.h"

void registerAllTools(AgentToolRegistry* registry) {
    if (!registry) return;

    registry->registerTool(new AddWaypointTool(registry));
    registry->registerTool(new AddTakeoffTool(registry));
    registry->registerTool(new AddLandTool(registry));
    registry->registerTool(new AddLoiterTool(registry));
    registry->registerTool(new AddRTLTool(registry));
    registry->registerTool(new AddSurveyTool(registry));
    registry->registerTool(new AddTransitionTool(registry));
}