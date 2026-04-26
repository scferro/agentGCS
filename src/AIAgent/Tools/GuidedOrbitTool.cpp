#include "GuidedOrbitTool.h"
#include "../../Vehicle/Vehicle.h"

#include <QGeoCoordinate>

QString GuidedOrbitTool::description() const {
    return QStringLiteral(
        "Command the vehicle to orbit around a specific GPS coordinate at a given radius and altitude. "
        "Best for surveillance or area monitoring. Requires live vehicle connection. "
        "Only for multi-rotor and VTOL vehicles."
    );
}

QJsonObject GuidedOrbitTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["coordinates"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"description", QStringLiteral("Center point GPS coordinates as 'lat,lon' (required)")}
    };

    props["radius"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral("Orbit radius in meters (required)")}
    };

    props["radius_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for radius. Default=meters")}
    };

    props["altitude"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral("Orbit altitude above mean sea level in meters (required)")}
    };

    props["altitude_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for altitude. Default=meters")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray{QStringLiteral("coordinates"), QStringLiteral("radius"), QStringLiteral("altitude")};

    return schema;
}

QString GuidedOrbitTool::execute(const QJsonObject& args) {
    auto* vehicle = activeVehicle();
    if (!vehicle) return QStringLiteral("Error: No active vehicle connection");

    if (!args.contains("coordinates") || args["coordinates"].toString().isEmpty()) {
        return QStringLiteral("Error: coordinates parameter is required");
    }
    if (!args.contains("radius")) {
        return QStringLiteral("Error: radius parameter is required");
    }
    if (!args.contains("altitude")) {
        return QStringLiteral("Error: altitude parameter is required");
    }

    // Parse coordinates
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

    // Parse radius with unit conversion
    double radius = args["radius"].toDouble();
    if (args.contains("radius_units") && args["radius_units"].toString() == QStringLiteral("feet")) {
        radius *= 0.3048;  // feet to meters
    }

    // Parse altitude with unit conversion
    double altitude = args["altitude"].toDouble();
    if (args.contains("altitude_units") && args["altitude_units"].toString() == QStringLiteral("feet")) {
        altitude *= 0.3048;  // feet to meters
    }

    vehicle->guidedModeOrbit(coord, radius, altitude);

    return QStringLiteral("Orbit commanded: center (%1, %2), radius=%3 m, altitude=%4 m AMSL")
        .arg(lat, 0, 'f', 6)
        .arg(lon, 0, 'f', 6)
        .arg(radius, 0, 'f', 1)
        .arg(altitude, 0, 'f', 1);
}