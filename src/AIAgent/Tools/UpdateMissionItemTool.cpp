#include "UpdateMissionItemTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../Vehicle/Vehicle.h"
#include "../../MissionManager/SimpleMissionItem.h"
#include "../../MissionManager/PlanMasterController.h"
#include "../../MissionManager/VisualMissionItem.h"
#include "../../MissionManager/SpeedSection.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString UpdateMissionItemTool::description() const {
    return QStringLiteral(
        "Update properties of an existing mission item. Modify altitude, radius, speed, "
        "or other parameters by specifying the item's sequence number and the property to change."
    );
}

QJsonObject UpdateMissionItemTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["seq"] = QJsonObject{
        {"type", QStringLiteral("integer")},
        {"description", QStringLiteral("1-based sequence number of item to update (required)")}
    };

    props["altitude"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral("New altitude in meters")}
    };

    props["altitude_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for altitude value. Default = meters.")}
    };

    props["radius"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral("New loiter/orbit radius in meters")}
    };

    props["radius_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for radius value. Default = meters.")}
    };

    props["speed"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral("New speed in m/s")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray{QStringLiteral("seq")};

    return schema;
}

QString UpdateMissionItemTool::execute(const QJsonObject& args) {
    // 1. Get MissionController
    auto* ctrl = planController();
    if (!ctrl) return QStringLiteral("Error: PlanMasterController not available");
    auto* mc = ctrl->missionController();
    if (!mc) return QStringLiteral("Error: MissionController not available");

    // 2. Get sequence number (1-based from caller)
    const int seq = args["seq"].toInt();
    if (seq < 1) return QStringLiteral("Error: seq must be >= 1");

    // 3. Find the visual item (0-based index)
    const int index = seq - 1;
    auto* visualItem = mc->visualItems()->value<VisualMissionItem*>(index);
    if (!visualItem) return QStringLiteral("Error: Mission item %1 not found").arg(seq);

    // 4. Cast to SimpleMissionItem for property access
    auto* simpleItem = qobject_cast<SimpleMissionItem*>(visualItem);
    if (!simpleItem) return QStringLiteral("Error: Item %1 is not a simple mission item and cannot be updated").arg(seq);

    // 5. Update altitude if specified
    if (args.contains("altitude")) {
        double altitude = args["altitude"].toDouble();
        if (args.contains("altitude_units") && args["altitude_units"].toString() == QStringLiteral("feet")) {
            altitude *= 0.3048;  // feet to meters
        }
        simpleItem->altitude()->setRawValue(altitude);
    }

    // 6. Update radius if specified
    if (args.contains("radius")) {
        double radius = args["radius"].toDouble();
        if (args.contains("radius_units") && args["radius_units"].toString() == QStringLiteral("feet")) {
            radius *= 0.3048;  // feet to meters
        }
        simpleItem->setRadius(radius);
    }

    // 7. Update speed if specified (via SpeedSection)
    if (args.contains("speed")) {
        auto* speedSec = simpleItem->speedSection();
        if (speedSec) {
            speedSec->setSpecifyFlightSpeed(true);
            speedSec->flightSpeed()->setRawValue(args["speed"].toDouble());
        }
    }

    // 8. Build result message
    QStringList updates;
    if (args.contains("altitude")) {
        updates << QStringLiteral("altitude=%1 %2")
                      .arg(args["altitude"].toDouble())
                      .arg(args.contains("altitude_units") ? args["altitude_units"].toString() : QStringLiteral("meters"));
    }
    if (args.contains("radius")) {
        updates << QStringLiteral("radius=%1 %2")
                      .arg(args["radius"].toDouble())
                      .arg(args.contains("radius_units") ? args["radius_units"].toString() : QStringLiteral("meters"));
    }
    if (args.contains("speed")) {
        updates << QStringLiteral("speed=%1 m/s").arg(args["speed"].toDouble());
    }

    if (updates.isEmpty()) {
        return QStringLiteral("Warning: No properties specified to update for item %1").arg(seq);
    }

    return QStringLiteral("Mission item %1 updated: %2").arg(seq).arg(updates.join(", "));
}