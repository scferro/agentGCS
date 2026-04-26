#include "AddRTLTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../Vehicle/Vehicle.h"
#include "../../MissionManager/SimpleMissionItem.h"
#include "../../MissionManager/PlanMasterController.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString AddRTLTool::description() const {
    return QStringLiteral(
        "Add return to launch command to automatically fly back to takeoff point and land. "
        "Always inserted as the LAST mission item. Use when the drone should return home, "
        "land, or come back."
    );
}

QJsonObject AddRTLTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["altitude"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral(
            "Return altitude in meters. Specify only if user mentions specific "
            "return height. Default = 50 meters.")}
    };

    props["altitude_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for altitude. Default = meters.")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray();

    return schema;
}

QString AddRTLTool::execute(const QJsonObject& args) {
    auto* ctrl = planController();
    if (!ctrl) return QStringLiteral("Error: PlanMasterController not available");
    auto* mc = ctrl->missionController();
    if (!mc) return QStringLiteral("Error: MissionController not available");

    // RTL uses home position as coordinate, always appended at end (-1)
    QGeoCoordinate coordinate;
    auto* vehicle = activeVehicle();
    if (vehicle) coordinate = vehicle->homePosition();

    // Insert simple item then change command to RETURN_TO_LAUNCH
    VisualMissionItem* item = mc->insertSimpleMissionItem(coordinate, -1);
    if (!item) return QStringLiteral("Error: Failed to insert RTL item");

    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem) {
        simpleItem->setCommand(MAV_CMD_NAV_RETURN_TO_LAUNCH);

        if (args.contains("altitude")) {
            double alt = args["altitude"].toDouble();
            if (args.contains("altitude_units") && args["altitude_units"].toString() == QStringLiteral("feet")) {
                alt *= 0.3048;
            }
            simpleItem->altitude()->setRawValue(alt);
        }
    }

    const int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    QString altMsg;
    if (args.contains("altitude")) {
        altMsg = QStringLiteral(" at %1 %2")
            .arg(args["altitude"].toDouble())
            .arg(args["altitude_units"].toString("meters"));
    }

    return QStringLiteral("Return to Launch command added to mission%1 (Item %2)")
        .arg(altMsg)
        .arg(seqNum);
}