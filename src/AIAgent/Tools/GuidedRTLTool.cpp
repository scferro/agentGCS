#include "GuidedRTLTool.h"
#include "../../Vehicle/Vehicle.h"

QString GuidedRTLTool::description() const {
    return QStringLiteral(
        "Command the vehicle to return to launch/home position. The vehicle will navigate back to "
        "its home coordinates and land or loiter depending on settings. Requires live vehicle connection."
    );
}

QJsonObject GuidedRTLTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["smart_rtl"] = QJsonObject{
        {"type", QStringLiteral("boolean")},
        {"description", QStringLiteral("Use smart RTL (path-aware return) if available. Default=false")}
    };

    schema["properties"] = props;
    schema["required"] = QJsonArray();

    return schema;
}

QString GuidedRTLTool::execute(const QJsonObject& args) {
    auto* vehicle = activeVehicle();
    if (!vehicle) return QStringLiteral("Error: No active vehicle connection");

    bool smartRtl = false;
    if (args.contains("smart_rtl")) {
        smartRtl = args["smart_rtl"].toBool();
    }

    vehicle->guidedModeRTL(smartRtl);

    return smartRtl
        ? QStringLiteral("Smart RTL commanded: returning to launch via path-aware route")
        : QStringLiteral("RTL commanded: returning to launch/home position");
}