#include "GuidedLandTool.h"
#include "../../Vehicle/Vehicle.h"

QString GuidedLandTool::description() const {
    return QStringLiteral(
        "Command the vehicle to land at its current position. The vehicle will descend and touch down. "
        "Requires live vehicle connection."
    );
}

QJsonObject GuidedLandTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;
    schema["properties"] = props;
    schema["required"] = QJsonArray();

    return schema;
}

QString GuidedLandTool::execute(const QJsonObject& args) {
    Q_UNUSED(args);
    auto* vehicle = activeVehicle();
    if (!vehicle) return QStringLiteral("Error: No active vehicle connection");

    vehicle->guidedModeLand();

    return QStringLiteral("Land commanded: descending at current position");
}