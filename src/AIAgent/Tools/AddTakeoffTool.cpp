#include "AddTakeoffTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../Vehicle/Vehicle.h"
#include "../../MissionManager/SimpleMissionItem.h"
#include "../../MissionManager/PlanMasterController.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString AddTakeoffTool::description() const {
    return QStringLiteral(
        "Add takeoff command to launch drone from ground to flight altitude. "
        "Always inserted as the FIRST mission item. Use when user wants drone to take off, "
        "launch, or lift off, especially when altitude is specified like 'takeoff to 200 feet'."
    );
}

QJsonObject AddTakeoffTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["altitude"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral(
            "Target takeoff altitude in meters. "
            "DO NOT include unless directly specified by the user. Default = 30 meters.")}
    };

    props["altitude_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for altitude. Default = meters.")}
    };

    props["heading"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{
            QStringLiteral("north"), QStringLiteral("northeast"), QStringLiteral("east"),
            QStringLiteral("southeast"), QStringLiteral("south"), QStringLiteral("southwest"),
            QStringLiteral("west"), QStringLiteral("northwest")}},
        {"description", QStringLiteral(
            "Direction VTOL will point during transition to forward flight. "
            "Typically into the wind. Use ONLY when direction is specified and vehicle is VTOL.")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray();

    return schema;
}

QString AddTakeoffTool::execute(const QJsonObject& args) {
    auto* ctrl = planController();
    if (!ctrl) return QStringLiteral("Error: PlanMasterController not available");
    auto* mc = ctrl->missionController();
    if (!mc) return QStringLiteral("Error: MissionController not available");

    QGeoCoordinate coordinate;
    auto* vehicle = activeVehicle();
    if (vehicle) coordinate = vehicle->homePosition();

    // Takeoff is always inserted at index 1 (after home position item)
    VisualMissionItem* item = mc->insertTakeoffItem(coordinate, 1);
    if (!item) return QStringLiteral("Error: Failed to insert takeoff item");

    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem && args.contains("altitude")) {
        double altitude = args["altitude"].toDouble();
        if (args.contains("altitude_units") && args["altitude_units"].toString() == QStringLiteral("feet")) {
            altitude *= 0.3048;
        }
        simpleItem->altitude()->setRawValue(altitude);
    }

    const int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    const QString altMsg = args.contains("altitude")
        ? QString("%1 %2").arg(args["altitude"].toDouble()).arg(args["altitude_units"].toString("meters"))
        : QStringLiteral("default");

    return QStringLiteral("Takeoff command added to mission, Alt=%1 (Item %2)")
        .arg(altMsg)
        .arg(seqNum);
}