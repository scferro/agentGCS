#include "DeleteMissionItemTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../Vehicle/Vehicle.h"
#include "../../MissionManager/PlanMasterController.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString DeleteMissionItemTool::description() const {
    return QStringLiteral(
        "Delete a mission item from the plan by its sequence number. "
        "Removes the item and renumbers subsequent items."
    );
}

QJsonObject DeleteMissionItemTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["seq"] = QJsonObject{
        {"type", QStringLiteral("integer")},
        {"description", QStringLiteral("1-based sequence number of item to delete (required)")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray{QStringLiteral("seq")};

    return schema;
}

QString DeleteMissionItemTool::execute(const QJsonObject& args) {
    // 1. Get MissionController
    auto* ctrl = planController();
    if (!ctrl) return QStringLiteral("Error: PlanMasterController not available");
    auto* mc = ctrl->missionController();
    if (!mc) return QStringLiteral("Error: MissionController not available");

    // 2. Get sequence number (1-based from caller)
    const int seq = args["seq"].toInt();
    if (seq < 1) return QStringLiteral("Error: seq must be >= 1");

    // 3. Check item exists (0-based index)
    const int index = seq - 1;
    if (index >= mc->visualItems()->count()) {
        return QStringLiteral("Error: Mission item %1 not found (only %2 items in plan)")
            .arg(seq).arg(mc->visualItems()->count());
    }

    // 4. Remove the item
    mc->removeVisualItem(index);

    return QStringLiteral("Mission item %1 deleted. Plan now has %2 items.")
        .arg(seq).arg(mc->visualItems()->count());
}