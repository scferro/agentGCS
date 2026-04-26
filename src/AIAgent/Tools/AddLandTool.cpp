#include "AddLandTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../Vehicle/Vehicle.h"
#include "../../MissionManager/SimpleMissionItem.h"
#include "../../MissionManager/PlanMasterController.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString AddLandTool::description() const {
    return QStringLiteral(
        "Add land command at specified location or current position. "
        "Always inserted as the LAST mission item. Use when the drone should land. "
        "For multirotor this creates an RTL command; for fixed-wing a landing pattern; "
        "for VTOL a VTOL landing pattern."
    );
}

QJsonObject AddLandTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["coordinates"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"description", QStringLiteral(
            "GPS coordinates as 'lat,lon' for landing location. "
            "Leave empty for default landing at home position.")}
    };

    props["altitude"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral("Landing altitude in meters. Default = 0.")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray();

    return schema;
}

QString AddLandTool::execute(const QJsonObject& args) {
    auto* ctrl = planController();
    if (!ctrl) return QStringLiteral("Error: PlanMasterController not available");
    auto* mc = ctrl->missionController();
    if (!mc) return QStringLiteral("Error: MissionController not available");

    // Parse coordinates if provided
    QGeoCoordinate coordinate;
    if (args.contains("coordinates") && !args["coordinates"].toString().isEmpty()) {
        const QString coordStr = args["coordinates"].toString();
        const QStringList parts = coordStr.split(',');
        if (parts.size() == 2) {
            coordinate = QGeoCoordinate(parts[0].trimmed().toDouble(),
                                        parts[1].trimmed().toDouble());
        }
    }
    // Fall back to home position if no coordinates
    if (!coordinate.isValid()) {
        auto* vehicle = activeVehicle();
        if (vehicle) coordinate = vehicle->homePosition();
    }

    // insertLandItem() handles vehicle-type routing internally:
    //   fixed-wing → FixedWingLandingComplexItem
    //   VTOL       → VTOLLandingComplexItem
    //   multirotor → MAV_CMD_NAV_RETURN_TO_LAUNCH
    VisualMissionItem* item = mc->insertLandItem(coordinate, -1);
    if (!item) return QStringLiteral("Error: Failed to insert land item");

    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem && args.contains("altitude")) {
        simpleItem->altitude()->setRawValue(args["altitude"].toDouble());
    }

    const int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    return QStringLiteral("Land command added to mission (Item %1)").arg(seqNum);
}