#include "AddTransitionTool.h"
#include "../../MissionManager/MissionController.h"
#include "../../MissionManager/SimpleMissionItem.h"
#include "../../MissionManager/PlanMasterController.h"

#include <QGeoCoordinate>
#include <QJsonArray>

QString AddTransitionTool::description() const {
    return QStringLiteral(
        "Add VTOL transition command (multirotor to fixed-wing or vice versa). "
        "Use when user wants to switch flight mode for a VTOL drone. "
        "For commands like 'transition to fixed wing' or 'switch to multirotor mode'."
    );
}

QJsonObject AddTransitionTool::parameters() const {
    QJsonObject schema;
    schema["type"] = QStringLiteral("object");

    QJsonObject props;

    props["target_state"] = QJsonObject{
        {"type", QStringLiteral("string")},
        {"enum", QJsonArray{QStringLiteral("vtol_mc_to_fw"), QStringLiteral("vtol_fw_to_mc")}},
        {"description", QStringLiteral(
            "Transition direction: 'vtol_mc_to_fw' (multirotor to fixed-wing) "
            "or 'vtol_fw_to_mc' (fixed-wing to multirotor).")}
    };

    props["seq"] = QJsonObject{
        {"type", QStringLiteral("integer")},
        {"description", QStringLiteral(
            "Position to insert in mission (1-based index). Omit to add at end.")}
    };

    schema["properties"] = props;

    // target_state is required
    QJsonArray required;
    required.append(QStringLiteral("target_state"));
    schema["required"] = required;

    return schema;
}

QString AddTransitionTool::execute(const QJsonObject& args) {
    auto* ctrl = planController();
    if (!ctrl) return QStringLiteral("Error: PlanMasterController not available");
    auto* mc = ctrl->missionController();
    if (!mc) return QStringLiteral("Error: MissionController not available");

    if (!args.contains("target_state")) {
        return QStringLiteral("Error: target_state is required. Use 'vtol_mc_to_fw' or 'vtol_fw_to_mc'.");
    }

    const QString targetState = args["target_state"].toString();
    if (targetState != QStringLiteral("vtol_mc_to_fw") && targetState != QStringLiteral("vtol_fw_to_mc")) {
        return QStringLiteral("Error: Invalid target_state. Use 'vtol_mc_to_fw' or 'vtol_fw_to_mc'.");
    }

    int insertIndex = args.contains("seq") ? args["seq"].toInt() : -1;

    // Transition items use coordinate (0,0) — they're command items, not nav items
    QGeoCoordinate coordinate(0, 0);
    VisualMissionItem* item = mc->insertSimpleMissionItem(coordinate, insertIndex);
    if (!item) return QStringLiteral("Error: Failed to insert transition item");

    auto* simpleItem = qobject_cast<SimpleMissionItem*>(item);
    if (simpleItem) {
        simpleItem->setCommand(MAV_CMD_DO_VTOL_TRANSITION);
        // param1: MAV_VTOL_STATE — 3=MC→FW transition, 4=FW→MC transition
        const int transitionState = (targetState == QStringLiteral("vtol_mc_to_fw")) ? 3 : 4;
        simpleItem->missionItem().setParam1(transitionState);
    }

    const int seqNum = simpleItem ? simpleItem->sequenceNumber() : 0;
    return QStringLiteral("VTOL transition (%1) added to mission (Item %2)")
        .arg(targetState)
        .arg(seqNum);
}