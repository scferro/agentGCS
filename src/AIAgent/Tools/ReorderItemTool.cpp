#include "ReorderItemTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../Vehicle/Vehicle.h"
#include "../../MissionManager/PlanMasterController.h"
#include "../../MissionManager/VisualMissionItem.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString ReorderItemTool::description() const {
    return QStringLiteral(
        "Change the order of mission items by moving an item from one position to another. "
        "Specify the current sequence number and the desired new position."
    );
}

QJsonObject ReorderItemTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["from_seq"] = QJsonObject{
        {"type", QStringLiteral("integer")},
        {"description", QStringLiteral("Current 1-based sequence number of item to move (required)")}
    };

    props["to_seq"] = QJsonObject{
        {"type", QStringLiteral("integer")},
        {"description", QStringLiteral("Target 1-based position to move the item to (required)")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray{QStringLiteral("from_seq"), QStringLiteral("to_seq")};

    return schema;
}

QString ReorderItemTool::execute(const QJsonObject& args) {
    // 1. Get MissionController
    auto* ctrl = planController();
    if (!ctrl) return QStringLiteral("Error: PlanMasterController not available");
    auto* mc = ctrl->missionController();
    if (!mc) return QStringLiteral("Error: MissionController not available");

    // 2. Get sequence numbers (1-based from caller)
    const int fromSeq = args["from_seq"].toInt();
    const int toSeq = args["to_seq"].toInt();
    if (fromSeq < 1) return QStringLiteral("Error: from_seq must be >= 1");
    if (toSeq < 1) return QStringLiteral("Error: to_seq must be >= 1");

    // 3. Convert to 0-based indices
    const int fromIndex = fromSeq - 1;
    const int toIndex = toSeq - 1;

    // 4. Validate indices
    auto* items = mc->visualItems();
    const int count = items->count();
    if (fromIndex >= count) {
        return QStringLiteral("Error: from_seq %1 is out of range (only %2 items in plan)")
            .arg(fromSeq).arg(count);
    }
    if (toIndex >= count) {
        return QStringLiteral("Error: to_seq %1 is out of range (only %2 items in plan)")
            .arg(toSeq).arg(count);
    }
    if (fromIndex == toIndex) {
        return QStringLiteral("Item %1 is already at position %2, no change needed")
            .arg(fromSeq).arg(toSeq);
    }

    // 5. Move the item using QmlObjectListModel::move()
    items->move(fromIndex, toIndex);

    return QStringLiteral("Mission item moved from position %1 to position %2")
        .arg(fromSeq).arg(toSeq);
}