import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Settings
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.Palette

// Hand-coded AI Agent settings page with model management UI
Rectangle {
    id:         aiSettingsRect
    color:      qgcPal.window
    visible:    aiSettings !== undefined

    property var aiSettings: QGroundControl.settingsManager ? QGroundControl.settingsManager.aiSettings : undefined

    ColumnLayout {
        id:             mainColumn
        anchors.fill:   parent
        anchors.margins: ScreenTools.defaultFontPixelWidth
        spacing:        ScreenTools.defaultFontPixelHeight / 2

        // Auto-generated settings from AI.SettingsUI.json
        SettingsGroupLayout {
            id:             settingsGroup
            Layout.fillWidth: true
            heading:        qsTr("AI Agent Settings")
            pageDefinition: "AI.SettingsUI.json"
        }

        // Model management section (hand-coded)
        SectionHeader {
            id:             modelHeader
            Layout.fillWidth: true
            text:           qsTr("Model Management")
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible:        modelHeader.checked
            spacing:        ScreenTools.defaultFontPixelHeight / 2

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("Available Models:")
                    Layout.alignment: Qt.AlignTop
                }

                ListView {
                    id:                 modelListView
                    Layout.fillWidth:   true
                    Layout.preferredHeight: contentHeight
                    clip:               true
                    model:              aiSettings ? ModelManager.discoveredModels : []

                    delegate: QGCButton {
                        text: modelData
                        width: modelListView.width
                        onClicked: aiSettings.modelPath = modelData
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:       qsTr("Scan Models")
                    visible:    aiSettings !== undefined
                    onClicked:  ModelManager.scanModels(aiSettings ? aiSettings.modelPath : "")
                }

                QGCLabel {
                    id:         modelStatusLabel
                    visible:    text !== ""
                    text:       ModelManager.statusMessage
                }
            }
        }
    }
}