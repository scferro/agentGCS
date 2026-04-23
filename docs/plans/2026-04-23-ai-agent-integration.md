# AI Agent Integration — agentGCS Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Port the mav-agent ReAct agent loop and toolset into agentGCS as a C++ module with QML chat sidebar, using llama.cpp for local LLM inference with Gemma 4 E2B/E4B models.

**Architecture:** A new `AIAgent` C++ module integrated into the QGC build system. The module contains (1) an LLM engine wrapping llama.cpp's C API, (2) a tool registry mirroring mav-agent's 11 tools but executing via QGC's Vehicle/PlanMasterController APIs, (3) an AgentController orchestrating the ReAct loop on a background thread, and (4) a QML chat sidebar injected into MainWindow. All proposed actions are staged for user approval before execution.

**Tech Stack:** C++17, Qt6/QML, llama.cpp C API, Gemma 4 E2B/E4B (GGUF Q4_K_M/Q5_K_M), nlohmann/json

---

## Phase 0 — Build Infrastructure & Dependencies

### Task 0.1: Add llama.cpp as a CMake subproject

**Objective:** Make llama.cpp buildable as a static library from within the agentGCS CMake tree.

**Files:**
- Create: `cmake/FindLlamaCpp.cmake` (or use `add_subdirectory`)
- Modify: `CMakeLists.txt` (top-level, ~line 50)

**Step 1: Add llama.cpp as subdirectory in top-level CMakeLists.txt**

After the existing `add_subdirectory(src)` line, add:

```cmake
# --- llama.cpp (local LLM inference) ---
set(LLAMA_CURL ON CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(GGML_OPENMP OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../llama.cpp llama.cpp EXCLUDE_FROM_ALL)
```

**Step 2: Verify llama.cpp builds**

Run:
```bash
cd ~/agent_gcs/agentGCS && mkdir -p build-llama-test && cd build-llama-test
cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -20
cmake --build . --target llama 2>&1 | tail -5
```
Expected: ` Built target llama ` (the static library compiles)

**Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: add llama.cpp as CMake subproject"
```

---

### Task 0.2: Create the AIAgent module skeleton

**Objective:** Create the `src/AIAgent/` directory with CMakeLists.txt following QGC's module pattern, and wire it into the build.

**Files:**
- Create: `src/AIAgent/CMakeLists.txt`
- Modify: `src/CMakeLists.txt` (add `add_subdirectory(AIAgent)` and link target)

**Step 1: Create src/AIAgent/CMakeLists.txt**

```cmake
qt_add_library(AIAgentModule STATIC)

qt_add_qml_module(AIAgentModule
    URI QGroundControl.AIAgent
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        AIAgentSidebar.qml
        AIAgentChatView.qml
        AIAgentActionCard.qml
        AIAgentSlideToAccept.qml
    NO_PLUGIN
)

target_include_directories(AIAgentModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(AIAgentModule
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        llama
        nlohmann_json::nlohmann_json
)
```

**Step 2: Register module in src/CMakeLists.txt**

In `src/CMakeLists.txt`, add `AIAgent` to the `add_subdirectory` list and add `AIAgentModule` to the `target_link_libraries` of `QGroundControlModule`.

Find the line with all the `add_subdirectory()` calls (around line 40-70) and add:
```cmake
add_subdirectory(AIAgent)
```

Find the `target_link_libraries` block for `QGroundControlModule` and add `AIAgentModule`.

**Step 3: Create placeholder QML files**

Create minimal stub files so the build succeeds:
- `src/AIAgent/AIAgentSidebar.qml` — `import QtQuick 2.15; Item {}`
- `src/AIAgent/AIAgentChatView.qml` — same pattern
- `src/AIAgent/AIAgentActionCard.qml` — same pattern
- `src/AIAgent/AIAgentSlideToAccept.qml` — same pattern

**Step 4: Verify build**

Run: `cmake --build . 2>&1 | grep -i "aiagent\|error" | head -20`
Expected: AIAgentModule builds with no errors.

**Step 5: Commit**

```bash
git add src/AIAgent/ src/CMakeLists.txt
git commit -m "feat: add AIAgent module skeleton with CMake build"
```

---

## Phase 1 — LLM Engine (C++ Backend)

### Task 1.1: Create LLMEngine class — model loading and context

**Objective:** Wrap llama.cpp's model loading and context creation in a Qt-friendly C++ class.

**Files:**
- Create: `src/AIAgent/LLMEngine.h`
- Create: `src/AIAgent/LLMEngine.cpp`

**Step 1: Write LLMEngine.h**

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QThread>

struct llama_model;
struct llama_context;
struct llama_sampler;
struct common_chat_templates;

class LLMEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoaded READ isLoaded NOTIFY isLoadedChanged)
    Q_PROPERTY(bool isGenerating READ isGenerating NOTIFY isGeneratingChanged)
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(int contextLength READ contextLength WRITE setContextLength NOTIFY contextLengthChanged)
    Q_PROPERTY(int gpuLayers READ gpuLayers WRITE setGpuLayers NOTIFY gpuLayersChanged)

public:
    explicit LLMEngine(QObject* parent = nullptr);
    ~LLMEngine();

    bool isLoaded() const { return m_isLoaded; }
    bool isGenerating() const { return m_isGenerating; }
    QString modelPath() const { return m_modelPath; }
    int contextLength() const { return m_contextLength; }
    int gpuLayers() const { return m_gpuLayers; }

    void setModelPath(const QString& path);
    void setContextLength(int n);
    void setGpuLayers(int n);

    Q_INVOKABLE bool loadModel();
    Q_INVOKABLE void unloadModel();

signals:
    void isLoadedChanged();
    void isGeneratingChanged();
    void modelPathChanged();
    void contextLengthChanged();
    void gpuLayersChanged();
    void tokenGenerated(const QString& token);
    void generationComplete(const QString& fullText);
    void toolCallDetected(const QString& toolName, const QString& arguments);
    void loadFailed(const QString& error);

private:
    bool m_isLoaded = false;
    bool m_isGenerating = false;
    QString m_modelPath;
    int m_contextLength = 4096;
    int m_gpuLayers = 0;

    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    llama_sampler* m_sampler = nullptr;
    common_chat_templates* m_chatTemplates = nullptr;
};
```

**Step 2: Write LLMEngine.cpp — model loading**

Implement `loadModel()` using:
- `llama_model_load_from_file(path.toUtf8().constData(), params)` 
- `llama_init_from_model(model, ctx_params)`
- `common_chat_templates_init(model, "")` for chat template support
- Set `llama_model_params.n_gpu_layers = m_gpuLayers`

Implement `unloadModel()` using `llama_free(ctx)` and `llama_model_free(model)`.

**Step 3: Commit**

```bash
git add src/AIAgent/LLMEngine.h src/AIAgent/LLMEngine.cpp
git commit -m "feat: add LLMEngine class for model loading and context management"
```

---

### Task 1.2: Add chat completion with token streaming

**Objective:** Implement the core completion loop with streaming token emission and chat template formatting.

**Files:**
- Modify: `src/AIAgent/LLMEngine.h` (add completion methods)
- Modify: `src/AIAgent/LLMEngine.cpp`

**Step 1: Add completion API to LLMEngine.h**

```cpp
struct ChatMessage {
    QString role;    // "system", "user", "assistant", "tool"
    QString content;
    QString toolName;   // for role="tool"
    QString toolCallId;  // for role="tool"
};

Q_INVOKABLE void startCompletion(const QList<ChatMessage>& messages);
Q_INVOKABLE void cancelCompletion();
```

**Step 2: Implement completion loop**

- Convert `ChatMessage` list to `common_chat_msg` structs
- Call `common_chat_templates_apply()` with tools to get formatted prompt
- Use `llama_tokenize()` to encode prompt tokens
- Use `llama_decode()` in a loop, calling `llama_sampler_sample()` to get next token
- Emit `tokenGenerated(QString)` for each token (for streaming UI)
- Check for tool-call patterns using `common_chat_parse()` from `common/chat.h`
- When tool call detected: emit `toolCallDetected(name, args)` and pause
- When EOS or stop token: emit `generationComplete(fullText)`

**Step 3: Run on background thread**

The completion loop must run on a QThread to avoid blocking the UI. Add a `QThread m_workerThread` member, move LLMEngine to it via `moveToThread()`.

**Step 4: Commit**

```bash
git add src/AIAgent/
git commit -m "feat: add streaming chat completion with tool call detection"
```

---

### Task 1.3: Add tool definition schema support

**Objective:** Allow the LLM to understand available tools via the chat template's tool-calling format.

**Files:**
- Modify: `src/AIAgent/LLMEngine.h`
- Modify: `src/AIAgent/LLMEngine.cpp`

**Step 1: Define AgentTool struct**

```cpp
struct AgentTool {
    QString name;
    QString description;
    QString parametersJson;  // JSON schema string
};
```

**Step 2: Register tools with the completion**

In `startCompletion()`, convert `AgentTool` list to `common_chat_tool` structs and pass them via `common_chat_templates_inputs.tools`. The Gemma 4 chat template in the GGUF metadata handles the rest — `COMMON_CHAT_FORMAT_PEG_GEMMA4` in llama.cpp's common/chat.h explicitly supports Gemma 4 tool calling format.

**Step 3: Commit**

```bash
git add src/AIAgent/
git commit -m "feat: add tool definition schema support for Gemma 4 tool calling"
```

---

## Phase 2 — Tool Registry (C++ Port of mav-agent Tools)

### Task 2.1: Create AgentTool base class and registry

**Objective:** Define the C++ equivalent of mav-agent's `MAVLinkToolBase` and a registry that filters tools by vehicle type and mode.

**Files:**
- Create: `src/AIAgent/AgentToolBase.h`
- Create: `src/AIAgent/AgentToolRegistry.h`
- Create: `src/AIAgent/AgentToolRegistry.cpp`

**Step 1: Write AgentToolBase.h**

```cpp
#pragma once
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class Vehicle;
class PlanMasterController;

class AgentToolBase : public QObject {
    Q_OBJECT
public:
    explicit AgentToolBase(QObject* parent = nullptr) : QObject(parent) {}

    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual QString parametersJson() const = 0;   // JSON schema
    virtual QString execute(const QJsonObject& args) = 0;  // Returns result string

    virtual bool availableInMode(const QString& mode) const { return true; }
    virtual bool availableForVehicle(const QString& vehicleType) const { return true; }

protected:
    Vehicle* activeVehicle() const;
    PlanMasterController* planController() const;
};
```

**Step 2: Write AgentToolRegistry**

Mirrors mav-agent's `get_tools_for_mode()` / `_filter_tools_for_vehicle()` pattern:

- `registerTool(AgentToolBase*)` — add tool to registry
- `getToolsForMode(const QString& mode, const QString& vehicleType)` — returns filtered list
- `getToolDefinitions(mode, vehicleType)` — returns `QList<AgentTool>` for LLM
- Command mode: only "add" tools (add_waypoint, add_takeoff, add_loiter, add_survey, add_rtl, add_land, add_transition)
- Mission mode: all 11 tools (the above + update, delete, move, reorder)

**Step 3: Commit**

```bash
git add src/AIAgent/AgentToolBase.h src/AIAgent/AgentToolRegistry.h src/AIAgent/AgentToolRegistry.cpp
git commit -m "feat: add AgentTool base class and registry with mode/vehicle filtering"
```

---

### Task 2.2: Implement mission-item tools (add_waypoint, add_takeoff, add_loiter, add_rtl, add_survey, add_land, add_transition)

**Objective:** Port the 7 "add" tools from mav-agent, executing against QGC's `PlanMasterController` and `MissionController` APIs.

**Files:**
- Create: `src/AIAgent/Tools/AddWaypointTool.h/.cpp`
- Create: `src/AIAgent/Tools/AddTakeoffTool.h/.cpp`
- Create: `src/AIAgent/Tools/AddLoiterTool.h/.cpp`
- Create: `src/AIAgent/Tools/AddRTLTool.h/.cpp`
- Create: `src/AIAgent/Tools/AddSurveyTool.h/.cpp`
- Create: `src/AIAgent/Tools/AddLandTool.h/.cpp`
- Create: `src/AIAgent/Tools/AddTransitionTool.h/.cpp`
- Create: `src/AIAgent/Tools/CMakeLists.txt`
- Modify: `src/AIAgent/CMakeLists.txt` (add subdirectory)

**Each tool follows this pattern:**

```cpp
// AddWaypointTool.h
class AddWaypointTool : public AgentToolBase {
    Q_OBJECT
public:
    QString name() const override { return "add_waypoint"; }
    QString description() const override { return "Add a waypoint to the mission at specified coordinates and altitude."; }
    QString parametersJson() const override;  // JSON schema matching mav-agent's Pydantic model
    QString execute(const QJsonObject& args) override;
    bool availableForVehicle(const QString& vt) const override {
        return vt != "ground"; // ground vehicles still use waypoints, actually all do
    }
};
```

**Execution strategy per tool:**

| Tool | QGC API to call |
|------|----------------|
| add_waypoint | `MissionController::insertSimpleMissionItem(coordinate, index)` |
| add_takeoff | `MissionController::insertTakeoffMissionItem(altitude, coordinate, index)` |
| add_loiter | `MissionController::insertLoiterMissionItem(coordinate, altitude, radius, index)` |
| add_rtl | `MissionController::insertRLTMissionItem(coordinate, index)` |
| add_survey | `MissionController::insertSurveyMissionItem(coordinate, index)` then set survey parameters |
| add_land | `MissionController::insertLandMissionItem(coordinate, index)` |
| add_transition | `MissionController::insertTransitionMissionItem(targetState, index)` |

**Step 1: Implement AddWaypointTool as reference implementation**

Parse args for latitude, longitude, altitude (relative). Use `activeVehicle()->homePosition()` as reference. Call `MissionController::insertSimpleMissionItem()`. Set altitude via the item's `QGCMapCircle` or `VisualMissionItem` altitude property. Return result as JSON string.

**Step 2: Implement remaining 6 tools following same pattern**

**Step 3: Update CMakeLists.txt for Tools subdirectory**

**Step 4: Commit**

```bash
git add src/AIAgent/Tools/ src/AIAgent/CMakeLists.txt
git commit -m "feat: implement 7 mission-item add tools ported from mav-agent"
```

---

### Task 2.3: Implement editing tools (update, delete, move, reorder)

**Objective:** Port the 4 editing tools that only appear in mission mode.

**Files:**
- Create: `src/AIAgent/Tools/UpdateMissionItemTool.h/.cpp`
- Create: `src/AIAgent/Tools/DeleteMissionItemTool.h/.cpp`
- Create: `src/AIAgent/Tools/MoveItemTool.h/.cpp`
- Create: `src/AIAgent/Tools/ReorderItemTool.h/.cpp`

| Tool | QGC API to call |
|------|----------------|
| update_mission_item | Modify `VisualMissionItem` properties (altitude, radius, etc.) by seq index |
| delete_mission_item | `MissionController::removeMissionItem(index)` |
| move_item | Change `VisualMissionItem::coordinate` by seq index |
| reorder_item | Swap positions in `MissionController::visualItems` model |

All 4 set `availableInMode("mission")` only and are excluded from command mode.

**Step 1: Implement each tool**

**Step 2: Commit**

```bash
git add src/AIAgent/Tools/
git commit -m "feat: implement 4 mission editing tools (update, delete, move, reorder)"
```

---

### Task 2.4: Implement command-mode guided actions

**Objective:** For command mode (one-shot actions on a connected vehicle), tools should issue guided commands directly via Vehicle's API rather than building mission items.

**Files:**
- Create: `src/AIAgent/Tools/GuidedTakeoffTool.h/.cpp`
- Create: `src/AIAgent/Tools/GuidedGoToTool.h/.cpp`
- Create: `src/AIAgent/Tools/GuidedRTLTool.h/.cpp`
- Create: `src/AIAgent/Tools/GuidedLandTool.h/.cpp`
- Create: `src/AIAgent/Tools/GuidedOrbitTool.h/.cpp`

These wrap `Vehicle::guidedModeTakeoff()`, `Vehicle::guidedModeGotoLocation()`, `Vehicle::guidedModeRTL()`, `Vehicle::guidedModeLand()`, `Vehicle::guidedModeOrbit()`. They stage the action for user approval rather than executing immediately.

**Step 1: Implement each guided tool**

**Step 2: Register in AgentToolRegistry — these replace add_* tools when vehicle is connected and in command mode**

**Step 3: Commit**

```bash
git add src/AIAgent/Tools/
git commit -m "feat: implement guided action tools for command mode with live vehicle"
```

---

## Phase 3 — Agent Controller (ReAct Loop)

### Task 3.1: Create AgentController class

**Objective:** Orchestrate the ReAct (Reason+Act) loop: user message → LLM reasoning → tool call → tool result → LLM reasoning → ... → final response.

**Files:**
- Create: `src/AIAgent/AgentController.h`
- Create: `src/AIAgent/AgentController.cpp`

**Step 1: Write AgentController.h**

```cpp
class AgentController : public QObject {
    Q_OBJECT
    Q_PROPERTY(LLMEngine* llmEngine READ llmEngine CONSTANT)
    Q_PROPERTY(AgentToolRegistry* toolRegistry READ toolRegistry CONSTANT)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString vehicleType READ vehicleType WRITE setVehicleType NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(QVariantList pendingActions READ pendingActions NOTIFY pendingActionsChanged)
    Q_PROPERTY(QVariantList chatHistory READ chatHistory NOTIFY chatHistoryChanged)

public:
    explicit AgentController(QObject* parent = nullptr);

    Q_INVOKABLE void sendMessage(const QString& text);
    Q_INVOKABLE void approveAction(int actionIndex);
    Q_INVOKABLE void rejectAction(int actionIndex);
    Q_INVOKABLE void approveAllActions();
    Q_INVOKABLE void clearChat();
    Q_INVOKABLE void setModeForView(const QString& view); // "plan" → mission mode, "fly" → command mode

    // ... property accessors

signals:
    void modeChanged();
    void vehicleTypeChanged();
    void isProcessingChanged();
    void pendingActionsChanged();
    void chatHistoryChanged();
    void assistantMessage(const QString& text);
    void actionRequiresApproval(const QString& actionSummary);

private slots:
    void onTokenGenerated(const QString& token);
    void onToolCallDetected(const QString& toolName, const QString& arguments);
    void onGenerationComplete(const QString& fullText);

private:
    void runReActLoop();
    void stageActionForApproval(const QString& toolName, const QJsonObject& args, const QString& description);
    void executeApprovedAction(int actionIndex);
    void appendChatMessage(const QString& role, const QString& content);
    void injectSystemPrompt();
    void injectMissionState();
    void injectVehicleState();

    LLMEngine* m_llmEngine;
    AgentToolRegistry* m_toolRegistry;
    QString m_mode = "mission";  // or "command"
    QString m_vehicleType = "multicopter";
    bool m_isProcessing = false;
    QVariantList m_pendingActions; // actions awaiting user approval
    QVariantList m_chatHistory;    // list of {role, content} dicts
    QList<ChatMessage> m_messages; // for LLM context
};
```

**Step 2: Implement the ReAct loop**

```
User sends message
  → Append to chat history
  → Inject current mission state + vehicle state as context
  → Call LLM with all messages + available tools
  → LLM responds:
    Case A: Text only → display to user, done
    Case B: Tool call → 
      → Stage action for approval (with description of what it does)
      → Wait for user approval
      → If approved: execute tool, append tool result to messages
      → If rejected: append "Action rejected by user" to messages
      → Re-call LLM with tool result → repeat loop
```

**Step 3: Implement mode switching**

When user switches to PlanView: set mode="mission", load mission tools.
When user switches to FlyView with connected vehicle: set mode="command", load guided tools.

**Step 4: Commit**

```bash
git add src/AIAgent/AgentController.h src/AIAgent/AgentController.cpp
git commit -m "feat: add AgentController with ReAct loop and action approval staging"
```

---

### Task 3.2: System prompt and context injection

**Objective:** Port mav-agent's system prompts and add real-time vehicle state context.

**Files:**
- Modify: `src/AIAgent/AgentController.cpp`

**Step 1: Port system prompts from mav-agent**

Translate `MISSION_SYSTEM_PROMPT` and `COMMAND_SYSTEM_PROMPT` from `mav-agent/prompts/system_prompt.py` into C++ string constants. Add vehicle-specific prompt additions from `VEHICLE_PROMPTS`.

**Step 2: Implement context injection**

Before each LLM call, append current state as a user context message:
- Mission mode: serialize current mission items (from `MissionController::visualItems`) as JSON, similar to mav-agent's `get_mission_state_summary()`
- Command mode: serialize current vehicle state (position, armed, flight mode, battery) as JSON
- Include home position for coordinate reference

**Step 3: Commit**

```bash
git add src/AIAgent/
git commit -m "feat: add system prompts and real-time state context injection"
```

---

## Phase 4 — QML Chat Sidebar UI

### Task 4.1: Create AIAgentSidebar — sliding panel component

**Objective:** Build the collapsible sidebar that slides in from the left, pushing the main UI content.

**Files:**
- Create: `src/AIAgent/AIAgentSidebar.qml` (replace stub)

**Step 1: Implement sidebar with toggle button**

Pattern adapted from `PlanViewRightPanel.qml` but anchored left:

```qml
Item {
    id: root
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: expanded ? ScreenTools.defaultFontPixelWidth * 35 : 0

    property bool expanded: false

    // Semi-transparent background
    Rectangle {
        anchors.fill: parent
        color: qgcPal.window
        opacity: 0.95
    }

    // Toggle button (visible on the right edge when collapsed, left edge when expanded)
    Rectangle {
        id: toggleButton
        width: ScreenTools.defaultFontPixelWidth * 2
        height: ScreenTools.defaultFontPixelHeight * 3
        anchors.right: expanded ? parent.right : parent.left
        anchors.rightMargin: expanded ? 0 : -width
        anchors.verticalCenter: parent.verticalCenter
        color: qgcPal.button
        radius: 4

        QGCMouseArea {
            anchors.fill: parent
            onClicked: root.expanded = !root.expanded
        }

        QGCLabel {
            anchors.centerIn: parent
            text: expanded ? "◀" : "▶"
            font.pixelSize: ScreenTools.defaultFontPixelSize
        }
    }

    // Chat content when expanded
    AIAgentChatView {
        visible: root.expanded
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelWidth
    }
}
```

**Step 2: Commit**

```bash
git add src/AIAgent/AIAgentSidebar.qml
git commit -m "feat: implement AIAgentSidebar sliding panel component"
```

---

### Task 4.2: Create AIAgentChatView — message list and input

**Objective:** Chat interface with scrollable message history, user input field, and mode indicator.

**Files:**
- Create: `src/AIAgent/AIAgentChatView.qml` (replace stub)

**Step 1: Implement chat view**

```qml
Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: ScreenTools.defaultFontPixelHeight * 0.5

        // Header with mode indicator
        RowLayout {
            Layout.fillWidth: true
            QGCLabel { text: "AI Agent"; font.bold: true }
            QGCLabel {
                text: agentController.mode === "mission" ? "✈ Mission" : "⚡ Command"
                color: qgcPal.text
            }
        }

        // Message list
        ListView {
            id: messageList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: agentController.chatHistory
            delegate: ChatMessageDelegate {}
            clip: true
            onCountChanged: positionViewAtEnd()
        }

        // Pending actions area
        ListView {
            id: actionsList
            Layout.fillWidth: true
            Layout.maximumHeight: contentHeight
            model: agentController.pendingActions
            delegate: AIAgentActionCard {}
            visible: count > 0
        }

        // User input
        RowLayout {
            Layout.fillWidth: true
            QGCTextField {
                id: inputField
                Layout.fillWidth: true
                placeholderText: "Tell the agent what to do..."
                onAccepted: {
                    if (text.trim()) {
                        agentController.sendMessage(text.trim())
                        text = ""
                    }
                }
            }
            QGCButton {
                text: "Send"
                onClicked: inputField.accepted()
            }
        }
    }
}
```

**Step 2: Commit**

```bash
git add src/AIAgent/AIAgentChatView.qml
git commit -m "feat: implement AIAgentChatView with message list and input"
```

---

### Task 4.3: Create AIAgentActionCard — proposed action with approve/reject

**Objective:** Display each proposed action with a description, visual preview on map, and approve/reject controls.

**Files:**
- Create: `src/AIAgent/AIAgentActionCard.qml` (replace stub)

**Step 1: Implement action card**

```qml
Rectangle {
    id: root
    width: parent.width
    height: contentColumn.height + ScreenTools.defaultFontPixelHeight
    color: qgcPal.windowShade
    radius: 4
    border.color: qgcPal.text
    border.width: 1

    required property string actionName
    required property string actionDescription
    required property string actionParams
    required property int actionIndex

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
        spacing: 2

        QGCLabel {
            text: actionName
            font.bold: true
            font.pixelSize: ScreenTools.defaultFontPixelSize * 0.9
        }
        QGCLabel {
            text: actionDescription
            font.pixelSize: ScreenTools.defaultFontPixelSize * 0.8
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            QGCButton {
                text: "✗ Reject"
                onClicked: agentController.rejectAction(actionIndex)
            }
            AIAgentSlideToAccept {
                onAccepted: agentController.approveAction(actionIndex)
            }
        }
    }
}
```

**Step 2: Commit**

```bash
git add src/AIAgent/AIAgentActionCard.qml
git commit -m "feat: implement AIAgentActionCard with approve/reject controls"
```

---

### Task 4.4: Create AIAgentSlideToAccept — safety confirmation widget

**Objective:** Implement the slide-to-accept gesture that prevents accidental action approval.

**Files:**
- Create: `src/AIAgent/AIAgentSlideToAccept.qml` (replace stub)

**Step 1: Implement slide-to-accept**

A horizontal drag gesture: user drags a thumb from left to right across a track. When thumb reaches the end, `accepted()` signal fires.

```qml
Item {
    id: root
    height: ScreenTools.defaultFontPixelHeight * 2
    width: ScreenTools.defaultFontPixelWidth * 18

    signal accepted()

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: qgcPal.button
        border.color: qgcPal.buttonHighlight

        // Track fill
        Rectangle {
            width: thumb.x + thumb.width
            height: parent.height
            radius: parent.radius
            color: qgcPal.colorGreen
            opacity: 0.3
        }

        QGCLabel {
            anchors.centerIn: parent
            text: "→ Slide to Approve"
            font.pixelSize: ScreenTools.defaultFontPixelSize * 0.7
            opacity: 0.7
        }

        Rectangle {
            id: thumb
            x: 2
            y: 2
            width: parent.height - 4
            height: parent.height - 4
            radius: height / 2
            color: qgcPal.buttonHighlight

            DragHandler {
                id: dragHandler
                xAxis.minimum: 2
                xAxis.maximum: root.width - thumb.width - 2
                onActiveChanged: {
                    if (!active && thumb.x >= xAxis.maximum * 0.85) {
                        root.accepted()
                        thumb.x = 2
                    } else if (!active) {
                        // Animate back
                        anim.restart()
                    }
                }
            }
            NumberAnimation on x { id: anim; to: 2; duration: 200 }
        }
    }
}
```

**Step 2: Commit**

```bash
git add src/AIAgent/AIAgentSlideToAccept.qml
git commit -m "feat: implement slide-to-accept safety confirmation widget"
```

---

### Task 4.5: Integrate sidebar into MainWindow

**Objective:** Add the AIAgentSidebar to `MainWindow.qml` and expose the AgentController to QML.

**Files:**
- Modify: `src/AIAgent/CMakeLists.txt` (add C++ source files)
- Modify: `src/UI/MainWindow.qml`
- Create: `src/AIAgent/AIAgentQmlGlobal.h` (QML singleton registration)

**Step 1: Register AgentController as QML singleton**

Create a QML-accessible singleton similar to `QGroundControlQmlGlobal`:

```cpp
// AIAgentQmlGlobal.h
class AIAgentQmlGlobal : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(AIAgent)
    QML_SINGLETON
    Q_PROPERTY(AgentController* controller READ controller CONSTANT)
public:
    static AIAgentQmlGlobal* instance();
    AgentController* controller() const { return m_controller; }
private:
    AgentController* m_controller;
};
```

**Step 2: Add sidebar to MainWindow.qml**

In `MainWindow.qml`, add the sidebar as a sibling to FlyView and PlanView:

```qml
// After the FlyView/PlanView, add:
AIAgentSidebar {
    id: aiSidebar
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.left: parent.left
}
```

Also adjust FlyView/PlanView left anchor to respect sidebar width when expanded.

**Step 3: Connect mode switching to view changes**

In `showFlyView()` / `showPlanView()` handlers, call `AIAgent.controller.setModeForView()`.

**Step 4: Commit**

```bash
git add src/AIAgent/ src/UI/MainWindow.qml
git commit -m "feat: integrate AIAgentSidebar into MainWindow with mode switching"
```

---

## Phase 5 — Model Management & Settings

### Task 5.1: Add AI Agent settings panel

**Objective:** Create a settings page for model path, GPU layers, context length, and model download.

**Files:**
- Create: `src/AIAgent/AIAgentSettings.h/.cpp`
- Create: `src/AIAgent/AIAgentSettingsPanel.qml`
- Modify: `src/AIAgent/CMakeLists.txt`

**Step 1: Define settings FactGroup**

Following QGC's `FactSystem` pattern, create a `AIAgentSettings` FactGroup with:
- `modelPath` (string) — path to GGUF file
- `gpuLayers` (int32, default 0)
- `contextLength` (int32, default 4096)
- `temperature` (double, default 0.7)
- `topP` (double, default 0.9)
- `autoApproveSafe` (bool, default false) — auto-approve non-flight-critical actions

**Step 2: Create settings QML panel**

Simple form with fields for each setting, a model path browser, and a "Download Models" button that opens a dialog for fetching Gemma 4 GGUF files from HuggingFace.

**Step 3: Register in QGC settings menu**

**Step 4: Commit**

```bash
git add src/AIAgent/
git commit -m "feat: add AI Agent settings panel with model configuration"
```

---

### Task 5.2: Add Gemma 4 model download helper

**Objective:** Allow users to download Gemma 4 E2B/E4B GGUF models from HuggingFace with one click.

**Files:**
- Create: `src/AIAgent/ModelDownloader.h/.cpp`
- Create: `src/AIAgent/ModelDownloadDialog.qml`

**Step 1: Implement ModelDownloader**

Uses Qt's `QNetworkAccessManager` to download from HuggingFace:
- `google/gemma-4-E2B-it-GGUF` (Q4_K_M quant, ~1.8GB)
- `google/gemma-4-E4B-it-GGUF` (Q4_K_M quant, ~3.5GB)

Shows progress bar, validates SHA256 checksum, saves to `~/.local/share/agentGCS/models/`.

**Step 2: Create download dialog QML**

Two buttons: "Download Gemma 4 E2B (1.8GB)" and "Download Gemma 4 E4B (3.5GB)" with progress indicators.

**Step 3: Commit**

```bash
git add src/AIAgent/
git commit -m "feat: add Gemma 4 model downloader with HuggingFace integration"
```

---

## Phase 6 — Polish & Safety

### Task 6.1: Implement action preview on map

**Objective:** When proposed actions are staged, show visual cues on the QGC map (e.g., new waypoints, takeoff marker, survey polygon).

**Files:**
- Create: `src/AIAgent/AIAgentMapOverlay.h/.cpp`
- Create: `src/AIAgent/AIAgentMapOverlay.qml`
- Modify: `src/FlyView/FlyView.qml` or `src/FlightMap/FlightMap.qml`

**Step 1: Create map overlay controller**

When an action is staged, create temporary map items:
- Waypoints: circle markers with dashed lines connecting them
- Takeoff: upward arrow marker
- Survey: polygon outline
- RTL: home marker with dashed return line

Items appear in a distinct "preview" style (dashed, lighter color) that becomes solid when approved and committed to the real mission.

**Step 2: Add overlay to the map view**

**Step 3: Commit**

```bash
git add src/AIAgent/ src/FlightMap/
git commit -m "feat: add map overlay preview for proposed AI agent actions"
```

---

### Task 6.2: Safety validation layer

**Objective:** Before any action is presented to the user for approval, run safety checks and display warnings.

**Files:**
- Create: `src/AIAgent/SafetyValidator.h/.cpp`
- Modify: `src/AIAgent/AgentController.cpp`

**Step 1: Implement SafetyValidator**

Checks before staging an action:
- Takeoff altitude below minimum? → Warning
- Waypoint outside geofence? → Block
- RTL when already at home? → Warning
- Survey area too large? → Warning
- Action would arm vehicle without GPS lock? → Block
- Battery too low for proposed mission? → Block

Returns a `SafetyResult` with severity (OK / Warning / Block). Blocked actions are never presented. Warnings are displayed with the action card.

**Step 2: Integrate into AgentController**

In `stageActionForApproval()`, run safety validation first. If blocked, report to user without staging. If warning, attach warning to the action card.

**Step 3: Commit**

```bash
git add src/AIAgent/
git commit -m "feat: add safety validation layer for proposed agent actions"
```

---

### Task 6.3: Multimodal input (future-proofing)

**Objective:** Since Gemma 4 is multimodal (any-to-any), prepare the architecture for future video/image input.

**Files:**
- Modify: `src/AIAgent/LLMEngine.h` (add `llava_image_embed` support)
- Modify: `src/AIAgent/LLMEngine.cpp`

**Step 1: Add image embedding stub**

Add `ChatMessage` support for `content_parts` (type: "image" + data). When an image is present, use `llava_image_embed_make_with_pixels()` or `llava_image_embed_make_from_data_at()` from llama.cpp's llava extension to create image embeddings and inject them into the context.

**Note:** This is a stub for now — llava integration requires `llama-cpp.h` and the llava extension headers. The infrastructure is in place for when we want to feed QGC video streams or camera captures to the model.

**Step 2: Commit**

```bash
git add src/AIAgent/
git commit -m "feat: add multimodal input stub for future video/image support"
```

---

## Phase 7 — Testing & Documentation

> **Note:** All testing is done inside the Docker container (`./deploy/docker/run-docker-ubuntu.sh`). Build with `just docker` or `make docker`, then run the binary or tests inside the container. See the README Docker section for setup instructions.

### Task 7.1: Write unit tests for tool registry

**Objective:** Verify tool registration, mode/vehicle filtering, and parameter validation.

**Files:**
- Create: `test/AIAgent/test_agent_tool_registry.cpp`
- Create: `test/AIAgent/CMakeLists.txt`

**Step 1: Write tests**

```cpp
void testMissionModeGetsAllTools();
void testCommandModeGetsOnlyAddTools();
void testGroundVehicleExcludesLoiterAndSurvey();
void testVTOLGetsTransitionTool();
void testToolParameterValidation();
```

**Step 2: Commit**

```bash
git add test/AIAgent/
git commit -m "test: add unit tests for agent tool registry"
```

---

### Task 7.2: Write integration test for ReAct loop

**Objective:** End-to-end test of the agent loop with a mock LLM that returns predetermined tool calls.

**Files:**
- Create: `test/AIAgent/test_react_loop.cpp`

**Step 1: Create mock LLM**

A `MockLLMEngine` that returns scripted responses:
1. First call: returns a tool call for `add_waypoint`
2. After tool result: returns a text summary

**Step 2: Test the full cycle**

```cpp
void testSingleToolCallAndResponse();
void testActionApprovalAndExecution();
void testActionRejection();
void testSafetyBlock();
void testModeSwitchingClearsState();
```

**Step 3: Commit**

```bash
git add test/AIAgent/
git commit -m "test: add integration tests for ReAct loop with mock LLM"
```

---

### Task 7.3: Write README for the AIAgent module

**Objective:** Document the module's architecture, configuration, and how to extend it.

**Files:**
- Create: `src/AIAgent/README.md`

**Step 1: Write the README**

Cover:
- Architecture overview (LLMEngine → AgentController → ToolRegistry → QGC APIs)
- How to add a new tool
- Model setup guide (downloading Gemma 4, configuring GPU layers)
- Safety model explanation
- Portability (how to use this module in other QGC forks)

**Step 2: Commit**

```bash
git add src/AIAgent/README.md
git commit -m "docs: add AIAgent module README"
```

---

## PR Breakdown Summary

| PR | Phase | Tasks | Description | Est. Size |
|----|-------|-------|-------------|-----------|
| 1 | 0 | 0.1–0.2 | Build infrastructure + module skeleton | Small |
| 2 | 1 | 1.1–1.3 | LLM Engine (load, stream, tool calling) | Large |
| 3 | 2 | 2.1–2.2 | Tool base + 7 add tools (mission items) | Large |
| 4 | 2 | 2.3–2.4 | Edit tools + guided action tools | Medium |
| 5 | 3 | 3.1–3.2 | AgentController + ReAct loop + prompts | Large |
| 6 | 4 | 4.1–4.5 | Full QML UI (sidebar, chat, actions, slide-to-accept) | Large |
| 7 | 5 | 5.1–5.2 | Settings + model downloader | Medium |
| 8 | 6 | 6.1–6.3 | Map preview + safety + multimodal stub | Medium |
| 9 | 7 | 7.1–7.3 | Tests + documentation | Medium |

**Total: ~9 PRs, 23 tasks**

---

## Key Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| llama.cpp build conflicts with QGC's Qt6/CMake | Use `EXCLUDE_FROM_ALL` and only link `llama` static lib; test early in Phase 0 |
| Gemma 4 tool calling format not well-supported | llama.cpp has explicit `COMMON_CHAT_FORMAT_PEG_GEMMA4`; fallback: use generic grammar-constrained JSON output |
| QGC MissionController API not well-documented | Read `MissionController.h` thoroughly; prototype tool calls in a test before full integration |
| Blocking UI during LLM inference | LLMEngine runs on dedicated QThread; all signals cross thread boundary via Qt::QueuedConnection |
| Tool execution race conditions | Proposed actions are queued; only one executes at a time; AgentController is single-threaded for state |
| Model download requires HuggingFace auth | Gemma 4 gated models may need HF token; add token field to settings panel |