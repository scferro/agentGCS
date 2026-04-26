#include "MoveItemTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../Vehicle/Vehicle.h"
#include "../../MissionManager/PlanMasterController.h"
#include "../../MissionManager/VisualMissionItem.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString MoveItemTool::description() const {
    return QStringLiteral(
        "Move a mission item to a new GPS coordinate. "
        "Changes the waypoint's position without modifying other properties."
    );
}

QJsonObject MoveItemTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["seq"] = QJsonObject{
        {"type", QStringLiteral("integer")},
        {"description", QStringLiteral("1-based sequence number of item to move (required)")}
    };

    props["coordinates"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"description", QStringLiteral("New GPS coordinates as 'lat,lon' (e.g., '40.7128,-74.0060') (required)")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray{QStringLiteral("seq"), QStringLiteral("coordinates")};

    return schema;
}

QString MoveItemTool::execute(const QJsonObject& args) {
    // 1. Get MissionController
    auto* ctrl = planController();
    if (!ctrl) return QStringLiteral("Error: PlanMasterController not available");
    auto* mc = ctrl->missionController();
    if (!mc) return QStringLiteral("Error: MissionController not available");

    // 2. Get sequence number (1-based from caller)
    const int seq = args["seq"].toInt();
    if (seq < 1) return QStringLiteral("Error: seq must be >= 1");

    // 3. Parse coordinates
    if (!args.contains("coordinates") || args["coordinates"].toString().isEmpty()) {
        return QStringLiteral("Error: coordinates parameter is required");
    }
    const QString coordStr = args["coordinates"].toString();
    const QStringList parts = coordStr.split(',');
    if (parts.size() != 2) {
        return QStringLiteral("Error: Invalid coordinates format. Use 'lat,lon' (e.g., '40.7128,-74.0060')");
    }
    bool ok1 = false, ok2 = false;
    const double lat = parts[0].trimmed().toDouble(&ok1);
    const double lon = parts[1].trimmed().toDouble(&ok2);
    if (!ok1 || !ok2) {
        return QStringLiteral("Error: Invalid coordinates format. Use 'lat,lon' (e.g., '40.7128,-74.0060')");
    }
    QGeoCoordinate newCoord(lat, lon);
    if (!newCoord.isValid()) {
        return QStringLiteral("Error: Invalid coordinates. Latitude must be -90 to 90, longitude -180 to 180");
    }

    // 4. Find the visual item (0-based index)
    const int index = seq - 1;
    auto* visualItem = mc->visualItems()->value<VisualMissionItem*>(index);
    if (!visualItem) return QStringLiteral("Error: Mission item %1 not found").arg(seq);

    // 5. Set the new coordinate
    visualItem->setCoordinate(newCoord);

    return QStringLiteral("Mission item %1 moved to (%2, %3)")
        .arg(seq)
        .arg(newCoord.latitude(), 0, 'f', 6)
        .arg(newCoord.longitude(), 0, 'f', 6);
}