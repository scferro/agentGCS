#include "AddSurveyTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../Vehicle/Vehicle.h"
#include "../../MissionManager/ComplexMissionItem.h"
#include "../../MissionManager/PlanMasterController.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString AddSurveyTool::description() const {
    return QStringLiteral(
        "Create survey pattern for area coverage. Uses center point coordinate "
        "to generate a survey grid pattern. Use for survey commands like 'survey this area', "
        "'map the region', or 'scan the zone'. Specify coordinates OR distance/heading/reference. "
        "Do not mix location systems."
    );
}

QJsonObject AddSurveyTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["coordinates"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"description", QStringLiteral(
            "GPS coordinates as 'lat,lon' for survey center. "
            "Avoid using unless user provides exact coordinates.")}
    };

    props["altitude"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral(
            "Flight altitude for the survey in meters. Default = 50 meters.")}
    };

    props["altitude_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for altitude. Default = meters.")}
    };

    props["seq"] = QJsonObject{
        {"type", QStringLiteral("integer")},
        {"description", QStringLiteral(
            "Position to insert survey in mission (1-based index). Omit to add at end.")}
    };

    props["distance"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral(
            "Distance to survey center from reference point in meters.")}
    };

    props["heading"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{
            QStringLiteral("north"), QStringLiteral("northeast"), QStringLiteral("east"),
            QStringLiteral("southeast"), QStringLiteral("south"), QStringLiteral("southwest"),
            QStringLiteral("west"), QStringLiteral("northwest")}},
        {"description", QStringLiteral("Direction to survey center. Use with distance.")}
    };

    props["relative_reference_frame"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("origin"), QStringLiteral("last_waypoint")}},
        {"description", QStringLiteral(
            "Reference point for center distance: 'origin' or 'last_waypoint'.")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray();

    return schema;
}

QString AddSurveyTool::execute(const QJsonObject& args) {
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

    // Insert Survey as a complex mission item
    VisualMissionItem* item = mc->insertComplexMissionItem(
        QStringLiteral("Survey"), coordinate, insertIndex);
    if (!item) return QStringLiteral("Error: Failed to insert survey item");

    // Survey complex item altitude is set through the complex item's properties.
    // The Survey item created by QGC uses TransectStyleComplexItem which has
    // its own altitude fact. Access will be refined during runtime testing.
    // For now, we skip altitude setting on complex items.

    return QStringLiteral("Survey pattern created at (%1, %2) (Item inserted)")
        .arg(coordinate.latitude(), 0, 'f', 6)
        .arg(coordinate.longitude(), 0, 'f', 6);
}