#include "GuidedGoToTool.h"
#include "../../Vehicle/Vehicle.h"

#include <QGeoCoordinate>

QString GuidedGoToTool::description() const {
    return QStringLiteral(
        "Command the vehicle to fly to specific GPS coordinates. The vehicle will navigate to "
        "the given location in guided mode. Requires live vehicle connection."
    );
}

QJsonObject GuidedGoToTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["coordinates"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"description", QStringLiteral("GPS coordinates as 'lat,lon' (e.g., '40.7128,-74.0060') (required)")}
    };

    props["altitude"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral("Altitude to fly at in meters (optional, maintains current if not specified)")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray{QStringLiteral("coordinates")};

    return schema;
}

QString GuidedGoToTool::execute(const QJsonObject& args) {
    auto* vehicle = activeVehicle();
    if (!vehicle) return QStringLiteral("Error: No active vehicle connection");

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
        return QStringLiteral("Error: Invalid coordinates values. Use numeric lat,lon");
    }

    const QGeoCoordinate coord(lat, lon);
    if (!coord.isValid()) {
        return QStringLiteral("Error: Coordinates out of valid range");
    }

    vehicle->guidedModeGotoLocation(coord);

    return QStringLiteral("Go-to commanded: flying to (%1, %2)")
        .arg(lat, 0, 'f', 6)
        .arg(lon, 0, 'f', 6);
}