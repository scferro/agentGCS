#include "GuidedTakeoffTool.h"
#include "../../Vehicle/Vehicle.h"

QString GuidedTakeoffTool::description() const {
    return QStringLiteral(
        "Command the vehicle to take off to a specified altitude. The vehicle will enter guided mode "
        "and ascend. Requires live vehicle connection. Only for fixed_wing and vtol vehicles — "
        "multi-rotor takeoffs vertically by default."
    );
}

QJsonObject GuidedTakeoffTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["altitude"] = QJsonObject{
        {"type", QStringLiteral("number")},
        {"description", QStringLiteral("Takeoff altitude in meters above launch (required)")}
    };

    props["altitude_units"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("meters"), QStringLiteral("feet")}},
        {"description", QStringLiteral("Unit for altitude. Default=meters")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray{QStringLiteral("altitude")};

    return schema;
}

QString GuidedTakeoffTool::execute(const QJsonObject& args) {
    auto* vehicle = activeVehicle();
    if (!vehicle) return QStringLiteral("Error: No active vehicle connection");

    if (!args.contains("altitude")) {
        return QStringLiteral("Error: altitude parameter is required");
    }

    double altitude = args["altitude"].toDouble();
    if (args.contains("altitude_units") && args["altitude_units"].toString() == QStringLiteral("feet")) {
        altitude *= 0.3048;  // feet to meters
    }

    vehicle->guidedModeTakeoff(altitude);

    return QStringLiteral("Takeoff commanded: ascending to %1 meters")
        .arg(altitude, 0, 'f', 1);
}