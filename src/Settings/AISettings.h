#pragma once

#include <QtQmlIntegration/QtQmlIntegration>
#include "SettingsGroup.h"
#include <QObject>

/// @brief AI Agent settings — persistent configuration for the local LLM inference engine.
///
/// Settings include model path, hardware parameters (GPU layers, threads, context length),
/// sampling parameters (temperature, top-p), and behavior flags (auto-approve safe actions).
///
/// Follows QGC's FactSystem pattern exactly (see RTKSettings for reference).
class AISettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    AISettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(modelPath)
    DEFINE_SETTINGFACT(gpuLayers)
    DEFINE_SETTINGFACT(contextLength)
    DEFINE_SETTINGFACT(threadCount)
    DEFINE_SETTINGFACT(temperature)
    DEFINE_SETTINGFACT(topP)
    DEFINE_SETTINGFACT(autoApproveSafe)
};