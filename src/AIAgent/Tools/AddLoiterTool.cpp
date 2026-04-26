#include "AddLoiterTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../Vehicle/Vehicle.h"
#include "../../MissionManager/SimpleMissionItem.h"
#include "../../MissionManager/PlanMasterController.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString AddLoiterTool::description() const {
    return QStringLiteral(
        "Add circular orbit/loiter pattern at specified location. Use when user wants "
        "drone to fly in circles, orbit, or loiter. Use for commands like 'orbit', 'circle', "
        "'loiter', or when radius is mentioned. Specify coordinates OR distance/heading/reference. "
        "Do not mix location systems."
    );
}

QJsonObject AddLoiterTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["coordinates"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"description", QStringLiteral(
            "GPS coordinates as 'lat,lon' for orbit center. "
            "Avoid using unless user provides exact coordinates.")}
    };

    props["altitude"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral("Altitude for the orbit pattern in meters. Default = 50 meters.")}
    };

    props["altitude_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for altitude. Default = meters.")}
    };

    props["radius"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral("Radius of the circular orbit in meters. Default = 50 meters.")}
    };

    props["radius_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for radius. Default = meters.")}
    };

    props["seq"] = QJsonObject{
        {"type", QStringLiteral("integer")},
        {"description", QStringLiteral(
            "Position to insert in mission (1-based index). Omit to add at end.")}
    };

    props["distance"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral(
            "Distance to orbit center from reference point in meters.")}
    };

    props["heading"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{
            QStringLiteral("north"), QStringLiteral("northeast"), QStringLiteral("east"),
            QStringLiteral("southeast"), QStringLiteral("south"), QStringLiteral("southwest"),
            QStringLiteral("west"), QStringLiteral("northwest")}},
        {"description", QStringLiteral("Direction to orbit center. Use with distance.")}
    };

    props["relative_reference_frame"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("origin"), QStringLiteral("last_waypoint")}},
        {"description", QStringLiteral(
            "Reference point for distance: 'origin' (takeoff) or 'last_waypoint'.")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray();

    return schema;
}

QString AddLoiterTool::execute(const QJsonObject& args) {
    auto* ctrl = planController();
    if (!ctrl) return QStringLiteral("Error: PlanMasterController not available");
    auto* mc = ctrl->missionController();
    if (!mc) return QStringLiteral("Error: MissionController not available");

    // Parse coordinates
    QGeoCoordinate coordinate;
    if (args.contains("coordinates") && !args["coordinates"].toString().isEmpty()) {
        const QString coordStr = args["coordinates"].toString();
        const QStringList parts = coordStr.split(',');
        if (parts.size() == 2) {
            coordinate = QGeoCoordinate(parts[0].trimmed().toDouble(),
                                        parts[1].trimmed().toDouble());
        }
    }
    if (!coordinate.isValid()) {
        auto* vehicle = activeVehicle();
        if (vehicle) coordinate = vehicle->homePosition();
    }
    if (!coordinate.isValid()) return QStringLiteral("Error: No coordinates provided");

    int insertIndex = args.contains("seq") ? args["seq"].toInt() : -1;

    // Insert as simple item, then change command to LOITER_TURNS
    VisualMissionItem* item = mc->insertSimpleMissionItem(coordinate, insertIndex);
    if (!item) return QStringLiteral("Error: Failed to insert loiter item");

    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem) {
        // Change command from WAYPOINT to LOITER_TURNS
        simpleItem->setCommand(MAV_CMD_NAV_LOITER_TURNS);

        // Set altitude
        if (args.contains("altitude")) {
            double alt = args["altitude"].toDouble();
            if (args.contains("altitude_units") && args["altitude_units"].toString() == QStringLiteral("feet")) {
                alt *= 0.3048;
            }
            simpleItem->altitude()->setRawValue(alt);
        }

        // Set radius
        if (args.contains("radius")) {
            double radius = args["radius"].toDouble();
            if (args.contains("radius_units") && args["radius_units"].toString() == QStringLiteral("feet")) {
                radius *= 0.3048;
            }
            simpleItem->setRadius(radius);
        }
    }

    const int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    const QString altMsg = args.contains("altitude")
        ? QString("%1 %2").arg(args["altitude"].toDouble()).arg(args["altitude_units"].toString("meters"))
        : QStringLiteral("default");
    const QString radMsg = args.contains("radius")
        ? QString("%1 %2").arg(args["radius"].toDouble()).arg(args["radius_units"].toString("meters"))
        : QStringLiteral("default");

    return QStringLiteral("Loiter command added to mission: (%1, %2), Alt=%3, Radius=%4 (Item %5)")
        .arg(coordinate.latitude(), 0, 'f', 6)
        .arg(coordinate.longitude(), 0, 'f', 6)
        .arg(altMsg)
        .arg(radMsg)
        .arg(seqNum);
}