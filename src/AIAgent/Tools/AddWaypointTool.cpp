#include "AddWaypointTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../Vehicle/Vehicle.h"
#include "../../MissionManager/SimpleMissionItem.h"
#include "../../MissionManager/PlanMasterController.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString AddWaypointTool::description() const {
    return QStringLiteral(
        "Add waypoint for drone navigation to specific location. Use when user wants drone to fly to a "
        "location using exact GPS coordinates or relative directions. Creates flight path point where "
        "drone flies to location, flies THROUGH it, then continues to the next mission item. "
        "Specify coordinates OR distance/heading/reference_frame. Do not mix location systems."
    );
}

QJsonObject AddWaypointTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["coordinates"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"description", QStringLiteral(
            "GPS coordinates as 'lat,lon' (e.g., '40.7128,-74.0060'). "
            "Avoid using unless user provides exact coordinates. "
            "Prefer distance/heading/reference_frame for more intuitive positioning.")}
    };

    props["altitude"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral(
            "Flight altitude for this waypoint in meters. "
            "Specify only if user mentions altitude. Default = 50 meters.")}
    };

    props["altitude_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for altitude value. Default = meters.")}
    };

    props["seq"] = QJsonObject{
        {"type", QStringLiteral("integer")},
        {"description", QStringLiteral(
            "Position to insert waypoint in mission (1-based index). "
            "The waypoint will be inserted AT this position, shifting existing items down. "
            "Omit to add at end.")}
    };

    props["distance"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral(
            "Distance value in meters for relative positioning. "
            "Always use with heading parameter.")}
    };

    props["heading"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{
            QStringLiteral("north"), QStringLiteral("northeast"), QStringLiteral("east"),
            QStringLiteral("southeast"), QStringLiteral("south"), QStringLiteral("southwest"),
            QStringLiteral("west"), QStringLiteral("northwest")}},
        {"description", QStringLiteral("Compass direction for relative positioning. Always use with distance.")}
    };

    props["relative_reference_frame"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("origin"), QStringLiteral("last_waypoint")}},
        {"description", QStringLiteral(
            "Reference point for distance: 'origin' (takeoff) or 'last_waypoint'. "
            "Use 'origin' when user references start/takeoff/here. "
            "Otherwise assume last_waypoint.")}
    };

    schema["properties"] = props;
    // No required fields — coordinates OR relative positioning can be used
    schema["required"] = QJsonArray();

    return schema;
}

QString AddWaypointTool::execute(const QJsonObject& args) {
    // 1. Get MissionController from the base class helper
    auto* ctrl = planController();
    if (!ctrl) return QStringLiteral("Error: PlanMasterController not available");
    auto* mc = ctrl->missionController();
    if (!mc) return QStringLiteral("Error: MissionController not available");

    // 2. Parse coordinates
    QGeoCoordinate coordinate;
    if (args.contains("coordinates") && !args["coordinates"].toString().isEmpty()) {
        const QString coordStr = args["coordinates"].toString();
        const QStringList parts = coordStr.split(',');
        if (parts.size() == 2) {
            bool ok1 = false, ok2 = false;
            const double lat = parts[0].trimmed().toDouble(&ok1);
            const double lon = parts[1].trimmed().toDouble(&ok2);
            if (ok1 && ok2) {
                coordinate = QGeoCoordinate(lat, lon);
            }
        }
        if (!coordinate.isValid()) {
            return QStringLiteral("Error: Invalid coordinates format. Use 'lat,lon' (e.g., '40.7128,-74.0060')");
        }
    } else if (args.contains("distance")) {
        // TODO (future PR): Resolve relative coordinates using home/last waypoint
        return QStringLiteral("Error: Relative positioning not yet supported. Provide coordinates as 'lat,lon'.");
    } else {
        // Fallback to home position
        auto* vehicle = activeVehicle();
        if (vehicle) coordinate = vehicle->homePosition();
        if (!coordinate.isValid()) {
            return QStringLiteral("Error: No coordinates provided and no home position available");
        }
    }

    // 3. Determine insertion index
    int insertIndex = -1;  // -1 = append to end
    if (args.contains("seq")) {
        insertIndex = args["seq"].toInt();
    }

    // 4. Insert waypoint via MissionController
    VisualMissionItem* item = mc->insertSimpleMissionItem(coordinate, insertIndex);
    if (!item) {
        return QStringLiteral("Error: Failed to insert waypoint");
    }

    // 5. Set altitude if specified
    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem && args.contains("altitude")) {
        double altitude = args["altitude"].toDouble();
        if (args.contains("altitude_units") && args["altitude_units"].toString() == QStringLiteral("feet")) {
            altitude *= 0.3048;  // feet to meters
        }
        simpleItem->altitude()->setRawValue(altitude);
    }

    // 6. Build result message
    const int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    const QString altMsg = args.contains("altitude")
        ? QString("%1 %2").arg(args["altitude"].toDouble()).arg(args["altitude_units"].toString("meters"))
        : QStringLiteral("default");

    return QStringLiteral("Waypoint added to mission: (%1, %2), Alt=%3 (Item %4)")
        .arg(coordinate.latitude(), 0, 'f', 6)
        .arg(coordinate.longitude(), 0, 'f', 6)
        .arg(altMsg)
        .arg(seqNum);
}