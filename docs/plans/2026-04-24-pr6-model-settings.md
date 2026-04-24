# PR 6 — Model Management & Settings: Detailed Task Plan

**Branch:** `feat/ai-agent-pr6-model-settings` (from `feat/ai-agent-pr5-tool-calling`)
**Parent branch has:** PRs 1–5 committed (LLMEngine, chat UI, streaming, tool calling)

## Overview

PR 6 adds persistent AI agent settings, model discovery/validation, model downloading, a settings QML panel, and integration between settings and LLMEngine. It follows QGC's existing `SettingsGroup`/`FactSystem` pattern exactly.

### Components

1. **AISettings** — `SettingsGroup` subclass with 7 `Fact`s for model/inference configuration
2. **ModelManager** — Scans directories for `.gguf` files, validates GGUF magic header, exposes QML model
3. **ModelDownloader** — Downloads models from HuggingFace via `QNetworkAccessManager` with progress signals
4. **AI Settings QML Panel** — Settings page in QGC's AppSettings with model browser and download UI
5. **LLMEngine Integration** — AISettings → LLMEngine property binding on load

---

## Current State

✅ `LLMEngine.h/.cpp` — model load/streaming/tool calling complete
✅ `AIAgentModule` builds and links (278KB QML module)
✅ QGC unit tests pass (`LLMEngineLoadTest`, `LLMEngineStreamingTest`, `LLMEngineToolCallTest`)
✅ Standalone smoke tests pass (`test_model_load`, `test_tool_call`)
❌ No persistent settings — `LLMEngine` properties are hardcoded or runtime-only
❌ No model discovery — user must know the exact path
❌ No model downloader — manual download from HuggingFace
❌ No settings UI panel

---

## Settings Facts

| Fact Name      | Type     | Default | Description                                            |
|----------------|----------|---------|--------------------------------------------------------|
| `modelPath`    | string   | `""`    | Path to GGUF model file                                |
| `gpuLayers`    | int32    | `0`     | Number of GPU layers (0 = CPU only)                     |
| `contextLength`| int32    | `4096`  | Context window size in tokens                          |
| `threadCount`  | int32    | `0`     | Thread count (0 = auto-detect)                         |
| `temperature`  | double   | `0.7`   | Sampling temperature                                   |
| `topP`         | double   | `0.9`   | Top-p (nucleus) sampling                               |
| `autoApproveSafe` | bool | `false` | Auto-approve non-flight-critical tool actions          |

---

## Tasks

### Task 6.1: Create AISettings SettingsGroup (Header)

**Objective:** Define `AISettings` as a QGC `SettingsGroup` subclass with 7 facts.

**File:** Create `src/Settings/AISettings.h`

**Pattern:** Follow `RTKSettings.h` exactly — `DECLARE_SETTINGGROUP` + `DEFINE_SETTINGFACT` macros.

```cpp
#pragma once

#include <QtQmlIntegration/QtQmlIntegration>
#include "SettingsGroup.h"
#include <QObject>

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
```

**Key points:**
- `DEFINE_SETTING_NAME_GROUP()` declares `static const char* name` and `static const char* settingsGroup`
- `DEFINE_SETTINGFACT(X)` expands to: `Q_PROPERTY(Fact* X READ X CONSTANT)`, `Fact* X()`, `static const char* XName`, and `SettingsFact* _XFact = nullptr`
- `QML_ELEMENT` makes it accessible from QML via the Settings manager property
- `QML_UNCREATABLE("")` prevents direct QML instantiation (must go through SettingsManager)

---

### Task 6.2: Create AISettings SettingsGroup (Implementation)

**Objective:** Implement the `AISettings` class using QGC's `DECLARE_SETTINGGROUP` and `DECLARE_SETTINGSFACT` macros.

**File:** Create `src/Settings/AISettings.cpp`

**Pattern:** Follow `RTKSettings.cc` exactly.

```cpp
#include "AISettings.h"

DECLARE_SETTINGGROUP(AI, "AI")

DECLARE_SETTINGSFACT(AISettings, modelPath)
DECLARE_SETTINGSFACT(AISettings, gpuLayers)
DECLARE_SETTINGSFACT(AISettings, contextLength)
DECLARE_SETTINGSFACT(AISettings, threadCount)
DECLARE_SETTINGSFACT(AISettings, temperature)
DECLARE_SETTINGSFACT(AISettings, topP)
DECLARE_SETTINGSFACT(AISettings, autoApproveSafe)
```

**Key points:**
- `DECLARE_SETTINGGROUP(AI, "AI")`:
  - Defines `const char* AISettings::name = "AI"` (used for JSON file lookup: `AI.SettingsGroup.json`)
  - Defines `const char* AISettings::settingsGroup = "AI"` (QSettings group key)
  - Defines constructor: `AISettings::AISettings(QObject* parent) : SettingsGroup(name, settingsGroup, parent)`
- `DECLARE_SETTINGSFACT(AISettings, X)`:
  - Defines `const char* AISettings::XName = "X"`
  - Implements `Fact* AISettings::X()` — lazy-instantiates the `SettingsFact` from JSON metadata

---

### Task 6.3: Create AI.SettingsGroup.json (FactMetaData)

**Objective:** Define FactMetaData for all 7 settings in JSON format. The `SettingsGroup` base class loads this from `:/json/AI.SettingsGroup.json` (compiled from `src/Settings/`).

**File:** Create `src/Settings/AI.SettingsGroup.json`

**Pattern:** Follow `RTK.SettingsGroup.json` exactly.

```json
{
    "version": 1,
    "fileType": "FactMetaData",
    "QGC.MetaData.Facts": [
        {
            "name": "modelPath",
            "shortDesc": "Model path",
            "longDesc": "Path to the GGUF model file used for AI inference.",
            "type": "string",
            "default": "",
            "label": "Model Path"
        },
        {
            "name": "gpuLayers",
            "shortDesc": "GPU layers",
            "longDesc": "Number of model layers to offload to GPU. Set to 0 for CPU-only inference.",
            "type": "int32",
            "default": 0,
            "min": 0,
            "max": 999,
            "increment": 1,
            "decimalPlaces": 0,
            "label": "GPU Layers"
        },
        {
            "name": "contextLength",
            "shortDesc": "Context length",
            "longDesc": "Context window size in tokens. Larger values use more memory.",
            "type": "int32",
            "default": 4096,
            "min": 128,
            "max": 131072,
            "increment": 128,
            "decimalPlaces": 0,
            "label": "Context Length"
        },
        {
            "name": "threadCount",
            "shortDesc": "Thread count",
            "longDesc": "Number of threads for inference. 0 means auto-detect based on CPU cores.",
            "type": "int32",
            "default": 0,
            "min": 0,
            "max": 256,
            "increment": 1,
            "decimalPlaces": 0,
            "label": "Thread Count"
        },
        {
            "name": "temperature",
            "shortDesc": "Temperature",
            "longDesc": "Sampling temperature. Higher values produce more random output. Range: 0.0 to 2.0.",
            "type": "double",
            "default": 0.7,
            "min": 0.0,
            "max": 2.0,
            "increment": 0.05,
            "decimalPlaces": 2,
            "label": "Temperature"
        },
        {
            "name": "topP",
            "shortDesc": "Top P",
            "longDesc": "Top-p (nucleus) sampling threshold. Lower values focus on more likely tokens. Range: 0.0 to 1.0.",
            "type": "double",
            "default": 0.9,
            "min": 0.0,
            "max": 1.0,
            "increment": 0.05,
            "decimalPlaces": 2,
            "label": "Top P"
        },
        {
            "name": "autoApproveSafe",
            "shortDesc": "Auto-approve safe actions",
            "longDesc": "Automatically approve non-flight-critical tool actions without user confirmation. Only enable in trusted environments.",
            "type": "bool",
            "default": false,
            "label": "Auto-approve Safe Actions"
        }
    ]
}
```

**Key points:**
- JSON file is compiled into Qt resources via `file(GLOB_RECURSE ... *.json)` in `src/Settings/CMakeLists.txt`
- Template path in `SettingsGroup.h`: `kJsonFileTemplate = ":/json/%1.SettingsGroup.json"` → `":/json/AI.SettingsGroup.json"`
- Every fact needs: `name`, `shortDesc`, `type`, `default`, `label`
- Numeric facts need: `min`, `max`, `increment`, `decimalPlaces`
- `qgcRebootRequired` is not needed here (settings take effect on next model load)

---

### Task 6.4: Register AISettings in SettingsManager

**Objective:** Add `AISettings` to QGC's `SettingsManager` so it's accessible from C++ and QML.

**Files to modify:**
- `src/Settings/SettingsManager.h` — Add forward declaration, Q_PROPERTY, getter, member
- `src/Settings/SettingsManager.cc` — Add include, instantiation, getter

#### 6.4a: Modify `src/Settings/SettingsManager.h`

Add after `class Viewer3DSettings;` (line 27):
```cpp
class AISettings;
```

Add after `Q_MOC_INCLUDE("Viewer3DSettings.h")` (line 61):
```cpp
Q_MOC_INCLUDE("AISettings.h")
```

Add after `Q_PROPERTY(QObject *viewer3DSettings ...)` (line 85):
```cpp
Q_PROPERTY(QObject *aiSettings READ aiSettings CONSTANT)
```

Add after `Viewer3DSettings *viewer3DSettings() const;` (line 123):
```cpp
AISettings *aiSettings() const;
```

Add after `Viewer3DSettings *_viewer3DSettings = nullptr;` (line 151):
```cpp
AISettings *_aiSettings = nullptr;
```

#### 6.4b: Modify `src/Settings/SettingsManager.cc`

Add after `#include "Viewer3DSettings.h"` (line 27):
```cpp
#include "AISettings.h"
```

Add after `_viewer3DSettings = new Viewer3DSettings(this);` in `SettingsManager::init()` (line 78):
```cpp
_aiSettings = new AISettings(this);
```

Add after `Viewer3DSettings *SettingsManager::viewer3DSettings() const { return _viewer3DSettings; }` (line 108):
```cpp
AISettings *SettingsManager::aiSettings() const { return _aiSettings; }
```

#### 6.4c: Modify `src/Settings/CMakeLists.txt`

Add to `target_sources(${CMAKE_PROJECT_NAME} PRIVATE ...)` list (alphabetically near top, after `ADSBVehicleManagerSettings.h`):
```cmake
AISettings.cc
AISettings.h
```

**Verification:** `ninja -C /project/build QGroundControlModule` compiles and links without error. Settings are accessible via `SettingsManager::instance()->aiSettings()` and `QGroundControl.settingsManager.aiSettings` in QML.

---

### Task 6.5: Create ModelManager

**Objective:** Class to scan a directory for `.gguf` files, validate GGUF magic header, and expose list to QML.

**File:** Create `src/AIAgent/ModelManager.h`

```cpp
#pragma once

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

/// @brief Discovers and validates GGUF model files in configurable directories.
///
/// Scans a directory for .gguf files, validates each has the GGUF magic header
/// (bytes "GGUF" at offset 0), and exposes the valid list to QML for display
/// in the AI settings panel.
class ModelManager : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /// List of valid GGUF model file paths found in the scan directory.
    Q_PROPERTY(QStringList modelFiles READ modelFiles NOTIFY modelFilesChanged)

    /// Directory to scan for models. Defaults to ~/.local/share/agentGCS/models/
    Q_PROPERTY(QString scanDirectory READ scanDirectory WRITE setScanDirectory NOTIFY scanDirectoryChanged)

public:
    explicit ModelManager(QObject* parent = nullptr);

    QStringList modelFiles() const { return m_modelFiles; }
    QString scanDirectory() const { return m_scanDirectory; }
    void setScanDirectory(const QString& dir);

    /// Scan the configured directory and update modelFiles().
    /// Removes files that no longer exist and adds newly discovered ones.
    Q_INVOKABLE void refresh();

    /// Check if a file has a valid GGUF magic header (first 4 bytes == "GGUF").
    static bool validateGgufHeader(const QString& filePath);

    /// Get the default model storage directory.
    static QString defaultModelDirectory();

signals:
    void modelFilesChanged();
    void scanDirectoryChanged();

private:
    QStringList m_modelFiles;
    QString m_scanDirectory;
};
```

**File:** Create `src/AIAgent/ModelManager.cpp`

```cpp
#include "ModelManager.h"

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>

// GGUF magic bytes: first 4 bytes of a valid GGUF file
static const char kGgufMagic[] = "GGUF";

ModelManager::ModelManager(QObject* parent)
    : QObject(parent)
    , m_scanDirectory(defaultModelDirectory())
{
}

QString ModelManager::defaultModelDirectory()
{
    // Follow XDG data home convention: ~/.local/share/agentGCS/models/
    QString dataHome = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataHome.isEmpty()) {
        dataHome = QDir::homePath() + "/.local/share/agentGCS";
    }
    return dataHome + "/models";
}

void ModelManager::setScanDirectory(const QString& dir)
{
    if (m_scanDirectory == dir) return;
    m_scanDirectory = dir;
    emit scanDirectoryChanged();
}

void ModelManager::refresh()
{
    QStringList foundFiles;
    QDir dir(m_scanDirectory);

    if (!dir.exists()) {
        if (!m_modelFiles.isEmpty()) {
            m_modelFiles.clear();
            emit modelFilesChanged();
        }
        return;
    }

    QDirIterator it(dir.absolutePath(), QStringList() << "*.gguf",
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        if (validateGgufHeader(filePath)) {
            foundFiles.append(filePath);
        } else {
            qWarning() << "ModelManager: Skipping invalid GGUF file:" << filePath;
        }
    }

    foundFiles.sort();

    if (foundFiles != m_modelFiles) {
        m_modelFiles = foundFiles;
        emit modelFilesChanged();
    }
}

bool ModelManager::validateGgufHeader(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    char magic[4] = {0};
    if (file.read(magic, 4) != 4) {
        return false;
    }

    return (magic[0] == kGgufMagic[0] &&
            magic[1] == kGgufMagic[1] &&
            magic[2] == kGgufMagic[2] &&
            magic[3] == kGgufMagic[3]);
}
```

**Key points:**
- GGUF format starts with 4-byte magic `0x47 0x47 0x55 0x46` ("GGUF"). Validate by reading first 4 bytes.
- `QStandardPaths::AppDataLocation` maps to `~/.local/share/<org>/<app>` on Linux.
- `QDirIterator` with `Subdirectories` flag handles nested model directories (e.g., `models/gemma-4-E2B/`).
- Sort the list for consistent QML display.
- `validateGgufHeader` is `static` so it can be used in tests without an instance.

---

### Task 6.6: Create ModelDownloader

**Objective:** Download GGUF models from HuggingFace with progress tracking via `QNetworkAccessManager`.

**File:** Create `src/AIAgent/ModelDownloader.h`

```cpp
#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

/// @brief Downloads GGUF model files from HuggingFace with progress tracking.
///
/// Supports downloading from HuggingFace repositories with automatic progress
/// reporting. Files are saved to the ModelManager's scan directory.
class ModelDownloader : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /// True when a download is in progress.
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged)

    /// Download progress as a fraction 0.0–1.0.
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)

    /// Human-readable download status (e.g., "Downloading gemma-4-E2B... 45%").
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

    /// Target directory for downloaded files.
    Q_PROPERTY(QString targetDirectory READ targetDirectory WRITE setTargetDirectory NOTIFY targetDirectoryChanged)

public:
    explicit ModelDownloader(QObject* parent = nullptr);

    bool downloading() const { return m_downloading; }
    double progress() const { return m_progress; }
    QString statusText() const { return m_statusText; }
    QString targetDirectory() const { return m_targetDirectory; }
    void setTargetDirectory(const QString& dir);

    /// Start downloading a model file from the given URL.
    /// The file is saved to targetDirectory() using the filename from the URL.
    /// @param url Full HuggingFace download URL
    /// @param expectedSha256 Optional SHA256 hex string for verification
    Q_INVOKABLE void download(const QUrl& url, const QString& expectedSha256 = QString());

    /// Cancel the current download.
    Q_INVOKABLE void cancel();

    /// Build a HuggingFace download URL from repo and filename.
    /// E.g., buildHfUrl("google/gemma-4-E2B-it-GGUF", "gemma-4-E2B-it-Q4_K_M.gguf")
    /// → "https://huggingface.co/google/gemma-4-E2B-it-GGUF/resolve/main/gemma-4-E2B-it-Q4_K_M.gguf"
    Q_INVOKABLE static QUrl buildHfUrl(const QString& repo, const QString& filename);

signals:
    void downloadingChanged();
    void progressChanged();
    void statusTextChanged();
    void targetDirectoryChanged();

    /// Emitted when download completes successfully.
    /// @param filePath Path to the downloaded file on disk.
    void downloadComplete(const QString& filePath);

    /// Emitted when download fails or is cancelled.
    /// @param error Human-readable error message.
    void downloadFailed(const QString& error);

private slots:
    void _onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void _onDownloadFinished();
    void _onReadyRead();

private:
    void setDownloading(bool downloading);
    void setProgress(double progress);
    void setStatusText(const QString& text);

    QNetworkAccessManager m_nam;
    QNetworkReply* m_currentReply = nullptr;
    QFile m_outputFile;

    bool m_downloading = false;
    double m_progress = 0.0;
    QString m_statusText;
    QString m_targetDirectory;
    QString m_expectedSha256;
};
```

**File:** Create `src/AIAgent/ModelDownloader.cpp`

```cpp
#include "ModelDownloader.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QCryptographicHash>
#include <QtNetwork/QNetworkRequest>

ModelDownloader::ModelDownloader(QObject* parent)
    : QObject(parent)
    , m_targetDirectory(ModelManager::defaultModelDirectory())
{
}

void ModelDownloader::setTargetDirectory(const QString& dir)
{
    if (m_targetDirectory == dir) return;
    m_targetDirectory = dir;
    emit targetDirectoryChanged();
}

QUrl ModelDownloader::buildHfUrl(const QString& repo, const QString& filename)
{
    // HuggingFace raw file URL format:
    // https://huggingface.co/{repo}/resolve/main/{filename}
    QString urlStr = QStringLiteral("https://huggingface.co/%1/resolve/main/%2")
                         .arg(repo, filename);
    return QUrl(urlStr);
}

void ModelDownloader::download(const QUrl& url, const QString& expectedSha256)
{
    if (m_downloading) {
        emit downloadFailed(QStringLiteral("Download already in progress"));
        return;
    }

    // Extract filename from URL path
    const QString fileName = url.fileName();
    if (fileName.isEmpty()) {
        emit downloadFailed(QStringLiteral("Cannot determine filename from URL"));
        return;
    }

    // Ensure target directory exists
    QDir().mkpath(m_targetDirectory);

    const QString filePath = QDir(m_targetDirectory).filePath(fileName);
    m_outputFile.setFileName(filePath);
    if (!m_outputFile.open(QIODevice::WriteOnly)) {
        emit downloadFailed(QStringLiteral("Cannot open file for writing: %1").arg(filePath));
        return;
    }

    m_expectedSha256 = expectedSha256;

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    m_currentReply = m_nam.get(request);

    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &ModelDownloader::_onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::readyRead,
            this, &ModelDownloader::_onReadyRead);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &ModelDownloader::_onDownloadFinished);

    setDownloading(true);
    setProgress(0.0);
    setStatusText(QStringLiteral("Downloading %1...").arg(fileName));
}

void ModelDownloader::cancel()
{
    if (m_currentReply) {
        m_currentReply->abort();
    }
}

void ModelDownloader::_onReadyRead()
{
    if (m_currentReply) {
        m_outputFile.write(m_currentReply->readAll());
    }
}

void ModelDownloader::_onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        const double pct = static_cast<double>(bytesReceived) / static_cast<double>(bytesTotal);
        setProgress(pct);
        setStatusText(QStringLiteral("Downloading %1... %2%")
                          .arg(m_outputFile.fileName())
                          .arg(static_cast<int>(pct * 100)));
    }
}

void ModelDownloader::_onDownloadFinished()
{
    m_outputFile.close();

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;

    setDownloading(false);

    if (reply->error() != QNetworkReply::NoError) {
        // Clean up partial file
        m_outputFile.remove();

        const QString error = reply->error() == QNetworkReply::OperationCanceledError
            ? QStringLiteral("Download cancelled")
            : QStringLiteral("Download failed: %1").arg(reply->errorString());

        reply->deleteLater();
        setProgress(0.0);
        setStatusText(error);
        emit downloadFailed(error);
        return;
    }

    reply->deleteLater();

    // Optional SHA256 verification
    if (!m_expectedSha256.isEmpty()) {
        setStatusText(QStringLiteral("Verifying checksum..."));
        // Note: For large files, we'd need a streaming hash during download.
        // For now, read the file back for verification.
        QFile verifyFile(m_outputFile.fileName());
        if (!verifyFile.open(QIODevice::ReadOnly)) {
            const QString err = QStringLiteral("Cannot verify checksum: file not readable");
            emit downloadFailed(err);
            return;
        }
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!hash.addData(&verifyFile)) {
            const QString err = QStringLiteral("Checksum computation failed");
            verifyFile.close();
            emit downloadFailed(err);
            return;
        }
        verifyFile.close();

        const QString computed = QString::fromLatin1(hash.result().toHex());
        if (computed != m_expectedSha256.toLower()) {
            m_outputFile.remove();
            const QString err = QStringLiteral("SHA256 mismatch: expected %1, got %2")
                                    .arg(m_expectedSha256, computed);
            setStatusText(err);
            emit downloadFailed(err);
            return;
        }
    }

    setProgress(1.0);
    setStatusText(QStringLiteral("Download complete: %1").arg(m_outputFile.fileName()));
    emit downloadComplete(m_outputFile.fileName());
}

void ModelDownloader::setDownloading(bool downloading)
{
    if (m_downloading == downloading) return;
    m_downloading = downloading;
    emit downloadingChanged();
}

void ModelDownloader::setProgress(double progress)
{
    if (qFuzzyCompare(m_progress, progress)) return;
    m_progress = progress;
    emit progressChanged();
}

void ModelDownloader::setStatusText(const QString& text)
{
    if (m_statusText == text) return;
    m_statusText = text;
    emit statusTextChanged();
}
```

**Key points:**
- HuggingFace URL format: `https://huggingface.co/{repo}/resolve/main/{filename}`
- `QNetworkRequest::RedirectPolicyAttribute` allows redirect from HF CDN
- SHA256 verification reads the file back (acceptable for first implementation; streaming hash optimization in future PR)
- `cancel()` calls `QNetworkReply::abort()` which triggers `finished()` with `OperationCanceledError`
- `readyRead` signal writes data incrementally (memory-efficient for large files)
- Include `ModelManager.h` for `defaultModelDirectory()`

---

### Task 6.7: Update AIAgent CMakeLists.txt

**Objective:** Add `ModelManager` and `ModelDownloader` to `AIAgentModule`.

**File:** Modify `src/AIAgent/CMakeLists.txt`

Add to `target_sources(AIAgentModule PRIVATE ...)`:
```cmake
ModelManager.h
ModelManager.cpp
ModelDownloader.h
ModelDownloader.cpp
```

Add `Qt6::Network` to `target_link_libraries`:
```cmake
target_link_libraries(AIAgentModule
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        Qt6::Network
        llama
        llama-common
        nlohmann_json::nlohmann_json
)
```

Also add `Settings` include directory so `AIAgentModule` can access `AISettings`:
```cmake
target_include_directories(AIAgentModule
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/Settings
)
```

This is needed because `ModelDownloader.cpp` includes `ModelManager.h` which references `ModelManager::defaultModelDirectory()`.

---

### Task 6.8: Create AI Settings QML Panel

**Objective:** Create the AI settings page in QGC's settings menu with model browser, fact controls, and download section.

This involves three sub-tasks: a `SettingsUI.json` definition, a `SettingsPages.json` entry, and a hand-coded QML component for the download section.

#### 6.8a: Create `src/UI/AppSettings/pages/AI.SettingsUI.json`

**Pattern:** Follow `General.SettingsUI.json` and `NTRIP.SettingsUI.json`.

```json
{
    "version": 1,
    "fileType": "SettingsUI",
    "bindings": {
        "_aiSettings": "QGroundControl.settingsManager.aiSettings"
    },
    "groups": [
        {
            "heading": "Model",
            "keywords": ["ai", "model", "gguf", "llm", "path", "file"],
            "controls": [
                {
                    "setting": "aiSettings.modelPath",
                    "control": "browse",
                    "showWhen": "!ScreenTools.isMobile"
                }
            ]
        },
        {
            "heading": "Inference",
            "keywords": ["ai", "gpu", "layers", "context", "threads", "temperature", "topp", "sampling"],
            "controls": [
                {
                    "setting": "aiSettings.gpuLayers"
                },
                {
                    "setting": "aiSettings.contextLength"
                },
                {
                    "setting": "aiSettings.threadCount"
                },
                {
                    "setting": "aiSettings.temperature"
                },
                {
                    "setting": "aiSettings.topP"
                }
            ]
        },
        {
            "heading": "Safety",
            "keywords": ["ai", "auto", "approve", "safe", "tool", "action"],
            "controls": [
                {
                    "setting": "aiSettings.autoApproveSafe"
                }
            ]
        }
    ]
}
```

**Note:** The `browse` control type for `modelPath` is handled by the QML code generator. If the generator doesn't support `browse` for string types, we'll need a hand-coded QML component instead (see Task 6.8c).

#### 6.8b: Add AI page to `src/UI/AppSettings/pages/SettingsPages.json`

Add a new entry in the `pages` array. Insert after the "NTRIP/RTK" entry (line 62), before "PX4 Log Transfer":

```json
{
    "name": "AI Agent",
    "qml": "AISettings.qml",
    "icon": "qrc:/qmlimages/PaperPlane.svg",
    "visible": "QGroundControl.settingsManager && QGroundControl.settingsManager.aiSettings !== undefined",
    "pageDefinition": "AI.SettingsUI.json"
}
```

**Note:** The `AISettings.qml` page will be generated by the JSON-based code generator plus hand-coded components. If we need a custom model browser or download section, we add a hand-coded component.

#### 6.8c: Create hand-coded `src/UI/AppSettings/AIModelDownloadSection.qml`

This is a hand-coded QML component for the model discovery and download section. It's embedded in the generated `AISettings.qml` page via the `SettingsUI.json` `component` entry.

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    Layout.fillWidth:   true
    heading:            qsTr("Model Management")
    visible:            true

    property var _aiSettings:    QGroundControl.settingsManager.aiSettings
    property var _modelManager:  _aiSettings ? _aiSettings.modelManager : null
    property var _downloader:    _aiSettings ? _aiSettings.modelDownloader : null

    // --- Available Models ---
    QGCLabel {
        text:           qsTr("Available Models")
        font.bold:       true
        visible:        _modelManager && _modelManager.modelFiles.length > 0
    }

    Repeater {
        model: _modelManager ? _modelManager.modelFiles : []

        QGCButton {
            text:       modelData.split('/').pop()  // filename only
            onClicked:  _aiSettings.modelPath.rawValue = modelData
            visible:    _modelManager !== null
        }
    }

    QGCButton {
        text:       qsTr("Refresh Models")
        onClicked:  _modelManager ? _modelManager.refresh() : {}
        visible:    _modelManager !== null
    }

    // --- Download Section ---
    Item { Layout.fillWidth: true; height: ScreenTools.defaultFontPixelHeight }

    QGCLabel {
        text:           qsTr("Download Models")
        font.bold:       true
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelWidth

        QGCButton {
            text:       qsTr("Gemma 4 E2B (Q4_K_M, ~1.8GB)")
            onClicked:  _downloader ? _downloader.download(
                           _downloader.buildHfUrl("google/gemma-4-E2B-it-GGUF",
                                                  "gemma-4-E2B-it-Q4_K_M.gguf")
                        ) : {}
            enabled:    _downloader && !_downloader.downloading
        }

        QGCButton {
            text:       qsTr("Gemma 4 E4B (Q4_K_M, ~3.5GB)")
            onClicked:  _downloader ? _downloader.download(
                           _downloader.buildHfUrl("google/gemma-4-E4B-it-GGUF",
                                                  "gemma-4-E4B-it-Q4_K_M.gguf")
                        ) : {}
            enabled:    _downloader && !_downloader.downloading
        }

        QGCButton {
            text:       qsTr("Cancel")
            onClicked:  _downloader ? _downloader.cancel() : {}
            visible:    _downloader && _downloader.downloading
        }
    }

    // Progress bar
    ProgressBar {
        Layout.fillWidth: true
        from: 0; to: 1
        value: _downloader ? _downloader.progress : 0
        visible: _downloader && _downloader.downloading
    }

    QGCLabel {
        text:       _downloader ? _downloader.statusText : ""
        visible:    _downloader && _downloader.statusText !== ""
        font.pointSize: ScreenTools.smallFontPointSize
    }
}
```

**Note:** The QML needs `ModelManager` and `ModelDownloader` instances exposed from `AISettings`. We add these as child objects in Task 6.9.

#### 6.8d: Add `AIModelDownloadSection.qml` to AppSettings CMakeLists

Add to `src/UI/AppSettings/CMakeLists.txt` in the hand-coded QML_FILES section:
```cmake
AIModelDownloadSection.qml
```

Also add `AISettings.qml` to the `_generated_qml_names` list if it will be generated, OR add it as a hand-coded file. Decision: the settings page is generated from `AI.SettingsUI.json`, so add `AISettings.qml` to `_generated_qml_names`:

```cmake
set(_generated_qml_names
    ...
    AISettings.qml
)
```

---

### Task 6.9: Expose ModelManager and ModelDownloader via AISettings

**Objective:** Make `ModelManager` and `ModelDownloader` accessible from QML through `AISettings`.

**File:** Modify `src/Settings/AISettings.h`

Add Q_PROPERTY declarations and child objects:

```cpp
#pragma once

#include <QtQmlIntegration/QtQmlIntegration>
#include "SettingsGroup.h"
#include <QObject>

class ModelManager;
class ModelDownloader;

class AISettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(ModelManager* modelManager READ modelManager CONSTANT)
    Q_PROPERTY(ModelDownloader* modelDownloader READ modelDownloader CONSTANT)
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

    ModelManager* modelManager() const { return m_modelManager; }
    ModelDownloader* modelDownloader() const { return m_modelDownloader; }
private:
    ModelManager* m_modelManager = nullptr;
    ModelDownloader* m_modelDownloader = nullptr;
};
```

**File:** Modify `src/Settings/AISettings.cpp`

Add includes and create child objects in constructor:

```cpp
#include "AISettings.h"
#include "ModelManager.h"
#include "ModelDownloader.h"

DECLARE_SETTINGGROUP(AI, "AI")
{
    m_modelManager = new ModelManager(this);
    m_modelDownloader = new ModelDownloader(this);
    // Trigger initial scan
    m_modelManager->refresh();
}

DECLARE_SETTINGSFACT(AISettings, modelPath)
DECLARE_SETTINGSFACT(AISettings, gpuLayers)
DECLARE_SETTINGSFACT(AISettings, contextLength)
DECLARE_SETTINGSFACT(AISettings, threadCount)
DECLARE_SETTINGSFACT(AISettings, temperature)
DECLARE_SETTINGSFACT(AISettings, topP)
DECLARE_SETTINGSFACT(AISettings, autoApproveSafe)
```

**Key points:**
- `DECLARE_SETTINGGROUP(AI, "AI")` expands to the constructor. The `{ }` block after it is the constructor body.
- `ModelManager` and `ModelDownloader` are parented to `AISettings`, so they're destroyed with it.
- `AISettings` lives in the `Settings` module, but `ModelManager` and `ModelDownloader` live in the `AIAgent` module. This creates a cross-module dependency: `Settings` module needs to link against `AIAgentModule`.

**Cross-module link issue:** Since `AISettings` is in the `Settings` module (part of `QGroundControlModule`) and `ModelManager`/`ModelDownloader` are in `AIAgentModule`, we need to:
1. Add `AIAgentModule` to `Settings` module's link dependencies, OR
2. Move `ModelManager`/`ModelDownloader` creation out of `AISettings` and expose them through `QGCCorePlugin` or a singleton, OR
3. Move `ModelManager`/`ModelDownloader` into the `Settings` module (simpler, but mixes concerns)

**Recommended approach:** Option 1 — add `AIAgentModule` to Settings link dependencies. The modules are both static libraries linked into the final `QGroundControlModule`, so circular dependencies shouldn't occur (Settings ↔ AIAgentModule is one-way at link time since both end up in the same binary).

OR: Use a simpler approach — make `ModelManager` and `ModelDownloader` singletons independent of both modules, and expose them via QML context properties. But this doesn't follow QGC patterns.

**Simplest approach:** Move `ModelManager.h/.cpp` and `ModelDownloader.h/.cpp` into `src/Settings/` instead of `src/AIAgent/`. They don't depend on llama.cpp, only Qt Core/Network. This avoids cross-module dependency entirely.

**Decision: Move ModelManager and ModelDownloader to `src/Settings/`** since:
- They only use Qt Core + Qt Network (no llama.cpp dependency)
- They're tightly coupled to the settings panel
- This avoids cross-module link issues
- Update `src/Settings/CMakeLists.txt` to include them and add `Qt6::Network`

---

### Task 6.10: Integrate AISettings with LLMEngine

**Objective:** When the user loads a model, LLMEngine reads parameters from `AISettings` Facts instead of hardcoded defaults.

**File:** Modify `src/AIAgent/LLMEngine.h`

Add new properties and methods:
```cpp
/// Thread count for inference (0 = auto)
Q_PROPERTY(int threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)
/// Sampling temperature
Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)
/// Top-p sampling threshold
Q_PROPERTY(double topP READ topP WRITE setTopP NOTIFY topPChanged)

// New getters/setters
int threadCount() const { return m_threadCount; }
double temperature() const { return m_temperature; }
double topP() const { return m_topP; }
void setThreadCount(int n);
void setTemperature(double t);
void setTopP(double p);

// New signals
void threadCountChanged();
void temperatureChanged();
void topPChanged();

// New members
int m_threadCount = 0;
double m_temperature = 0.7;
double m_topP = 0.9;
```

**File:** Modify `src/AIAgent/LLMEngine.cpp`

1. Add setter implementations:
```cpp
void LLMEngine::setThreadCount(int n)
{
    if (m_threadCount == n) return;
    m_threadCount = n;
    emit threadCountChanged();
}

void LLMEngine::setTemperature(double t)
{
    if (qFuzzyCompare(m_temperature, t)) return;
    m_temperature = t;
    emit temperatureChanged();
}

void LLMEngine::setTopP(double p)
{
    if (qFuzzyCompare(m_topP, p)) return;
    m_topP = p;
    emit topPChanged();
}
```

2. Update `resetSampler()` to use `m_temperature` and `m_topP`:
```cpp
void LLMEngine::resetSampler()
{
    if (m_sampler) {
        llama_sampler_free(m_sampler);
        m_sampler = nullptr;
    }

    auto sparams = llama_sampler_chain_default_params();
    sparams.no_perf = true;
    m_sampler = llama_sampler_chain_init(sparams);

    // Use settings-based temperature and top_p
    llama_sampler_chain_add(m_sampler, llama_sampler_init_top_k(50));
    llama_sampler_chain_add(m_sampler, llama_sampler_init_top_p(static_cast<float>(m_topP), 1));
    llama_sampler_chain_add(m_sampler, llama_sampler_init_temp(static_cast<float>(m_temperature)));
    llama_sampler_chain_add(m_sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
}
```

3. Update `loadModel()` to use `m_threadCount`:
```cpp
// In loadModel(), after creating context with contextLength:
auto ctxParams = llama_context_default_params();
ctxParams.n_ctx = static_cast<uint32_t>(m_contextLength);
ctxParams.n_batch = 512;
ctxParams.n_ubatch = 128;
ctxParams.n_seq_max = 1;
ctxParams.offload_kqv = true;
ctxParams.n_threads = m_threadCount > 0 ? m_threadCount : 0;  // 0 = auto
ctxParams.n_threads_batch = m_threadCount > 0 ? m_threadCount : 0;
```

**Note:** The QML binding between `AISettings` Facts and `LLMEngine` properties happens in QML:
```qml
// In AIAgentSidebar.qml or wherever LLMEngine is instantiated:
engine.modelPath = QGroundControl.settingsManager.aiSettings.modelPath.rawValue
engine.gpuLayers = QGroundControl.settingsManager.aiSettings.gpuLayers.rawValue
engine.contextLength = QGroundControl.settingsManager.aiSettings.contextLength.rawValue
engine.threadCount = QGroundControl.settingsManager.aiSettings.threadCount.rawValue
engine.temperature = QGroundControl.settingsManager.aiSettings.temperature.rawValue
engine.topP = QGroundControl.settingsManager.aiSettings.topP.rawValue
```

Or use `Binding` QML elements for declarative binding:
```qml
Binding { target: engine; property: "modelPath"; value: aiSettings.modelPath.rawValue }
Binding { target: engine; property: "gpuLayers"; value: aiSettings.gpuLayers.rawValue }
// etc.
```

This QML wiring will be done in the AIAgentSidebar or a new AIAgentController, but is **not** part of this PR's core scope. This PR only ensures LLMEngine has the API to accept these values.

---

### Task 6.11: Add Settings QML generator support for AI page

**Objective:** Ensure the code generator produces `AISettings.qml` from `AI.SettingsUI.json`.

The generator is at `tools/generators/settings_qml/generate_pages.py`. It reads all `.SettingsUI.json` files from `src/UI/AppSettings/pages/` and all `.SettingsGroup.json` files from `src/Settings/`, then generates QML pages.

Since we're following the exact pattern of existing pages, the generator should pick up `AI.SettingsUI.json` automatically. However, we need to:

1. Add `AISettings.qml` to the `_generated_qml_names` list in `src/UI/AppSettings/CMakeLists.txt`
2. Ensure the generated page references `AIModelDownloadSection.qml` as a component (add a `component` group entry in `AI.SettingsUI.json`)

Update `AI.SettingsUI.json` to include the download component:

```json
{
    "version": 1,
    "fileType": "SettingsUI",
    "bindings": {
        "_aiSettings": "QGroundControl.settingsManager.aiSettings"
    },
    "groups": [
        {
            "component": "AIModelDownloadSection",
            "sectionName": "Model Management",
            "keywords": ["ai", "model", "download", "huggingface", "gguf"]
        },
        {
            "heading": "Model",
            "keywords": ["ai", "model", "gguf", "llm", "path"],
            "controls": [
                {
                    "setting": "aiSettings.modelPath",
                    "control": "browse",
                    "showWhen": "!ScreenTools.isMobile"
                }
            ]
        },
        {
            "heading": "Inference",
            "keywords": ["ai", "gpu", "layers", "context", "threads", "temperature", "topp"],
            "controls": [
                {
                    "setting": "aiSettings.gpuLayers"
                },
                {
                    "setting": "aiSettings.contextLength"
                },
                {
                    "setting": "aiSettings.threadCount"
                },
                {
                    "setting": "aiSettings.temperature"
                },
                {
                    "setting": "aiSettings.topP"
                }
            ]
        },
        {
            "heading": "Safety",
            "keywords": ["ai", "auto", "approve", "safe"],
            "controls": [
                {
                    "setting": "aiSettings.autoApproveSafe"
                }
            ]
        }
    ]
}
```

**Note:** The `component` field references a hand-coded QML file by its QML type name. The generator creates a `Loader` or direct instantiation of that component.

---

### Task 6.12: Create Unit Tests

**Objective:** Write QGC unit tests for AISettings persistence, ModelManager validation, and ModelDownloader.

**File:** Create `test/AIAgent/AISettingsTest.h`

```cpp
#pragma once
#include "UnitTest.h"

class AISettingsTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _testDefaultValues();
    void _testSettingsPersist();
    void _testModelPathValidation();
};
```

**File:** Create `test/AIAgent/AISettingsTest.cc`

```cpp
#include "AISettingsTest.h"
#include "AISettings.h"
#include "SettingsManager.h"

void AISettingsTest::_testDefaultValues()
{
    auto* ai = SettingsManager::instance()->aiSettings();
    QVERIFY(ai != nullptr);

    QCOMPARE(ai->gpuLayers()->rawValue().toInt(), 0);
    QCOMPARE(ai->contextLength()->rawValue().toInt(), 4096);
    QCOMPARE(ai->threadCount()->rawValue().toInt(), 0);
    QCOMPARE(ai->temperature()->rawValue().toDouble(), 0.7);
    QCOMPARE(ai->topP()->rawValue().toDouble(), 0.9);
    QCOMPARE(ai->autoApproveSafe()->rawValue().toBool(), false);
    QCOMPARE(ai->modelPath()->rawValue().toString(), QString(""));
}
```

**File:** Create `test/AIAgent/ModelManagerTest.h`

```cpp
#pragma once
#include "UnitTest.h"

class ModelManagerTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _testValidateGgufHeader();
    void _testValidateInvalidFile();
    void _testScanDirectory();
    void _testDefaultModelDirectory();
};
```

**File:** Create `test/AIAgent/ModelManagerTest.cc`

```cpp
#include "ModelManagerTest.h"
#include "ModelManager.h"
#include <QtCore/QTemporaryDir>
#include <QtCore/QFile>

void ModelManagerTest::_testValidateGgufHeader()
{
    QTemporaryDir dir;
    const QString validFile = dir.path() + "/test.gguf";
    QFile f(validFile);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("GGUF"); // Write magic header
    f.write("some model data");
    f.close();
    QVERIFY(ModelManager::validateGgufHeader(validFile));
}

void ModelManagerTest::_testValidateInvalidFile()
{
    QTemporaryDir dir;
    const QString invalidFile = dir.path() + "/invalid.gguf";
    QFile f(invalidFile);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("NOT_GGUF_DATA");
    f.close();
    QVERIFY(!ModelManager::validateGgufHeader(invalidFile));

    // Empty file
    const QString emptyFile = dir.path() + "/empty.gguf";
    QFile ef(emptyFile);
    QVERIFY(ef.open(QIODevice::WriteOnly));
    ef.close();
    QVERIFY(!ModelManager::validateGgufHeader(emptyFile));
}

void ModelManagerTest::_testScanDirectory()
{
    QTemporaryDir dir;
    // Create valid GGUF file
    const QString valid = dir.path() + "/model.gguf";
    QFile f(valid);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("GGUF");
    f.close();

    // Create invalid file
    const QString invalid = dir.path() + "/bad.gguf";
    QFile bf(invalid);
    QVERIFY(bf.open(QIODevice::WriteOnly));
    bf.write("NOT");
    bf.close();

    ModelManager mgr;
    mgr.setScanDirectory(dir.path());
    mgr.refresh();

    QCOMPARE(mgr.modelFiles().size(), 1);
    QCOMPARE(mgr.modelFiles().first(), valid);
}

void ModelManagerTest::_testDefaultModelDirectory()
{
    const QString dir = ModelManager::defaultModelDirectory();
    QVERIFY(!dir.isEmpty());
    QVERIFY(dir.contains("models"));
}
```

**File:** Create `test/AIAgent/ModelDownloaderTest.h`

```cpp
#pragma once
#include "UnitTest.h"

class ModelDownloaderTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _testBuildHfUrl();
    void _testDownloadSmallFile();
    void _testCancelDownload();
};
```

**File:** Create `test/AIAgent/ModelDownloaderTest.cc`

```cpp
#include "ModelDownloaderTest.h"
#include "ModelDownloader.h"
#include "MultiSignalSpy.h"

void ModelDownloaderTest::_testBuildHfUrl()
{
    QUrl url = ModelDownloader::buildHfUrl(
        "google/gemma-4-E2B-it-GGUF", "gemma-4-E2B-it-Q4_K_M.gguf");
    QCOMPARE(url.scheme(), QStringLiteral("https"));
    QCOMPARE(url.host(), QStringLiteral("huggingface.co"));
    QVERIFY(url.path().contains("google/gemma-4-E2B-it-GGUF"));
    QVERIFY(url.path().contains("gemma-4-E2B-it-Q4_K_M.gguf"));
}

void ModelDownloaderTest::_testDownloadSmallFile()
{
    // Download a small public file to test progress and completion.
    // Use a reliable small URL (e.g., a small public domain file).
    QTemporaryDir dir;
    ModelDownloader downloader;
    downloader.setTargetDirectory(dir.path());

    MultiSignalSpy spy;
    QVERIFY(spy.init(&downloader, {downloader.downloadComplete(static_cast<QString())}));

    // Download a small file (~100 bytes) from a reliable public URL
    QUrl smallUrl("https://huggingface.co/api/models");
    downloader.download(smallUrl);

    // Wait for completion (with timeout)
    QCOMPARE(spy.waitForSignalByIndex(0, 10000), true);

    QVERIFY(!downloader.downloading());
    // Note: This test may be flaky in CI due to network. Mark as Integration/Network label.
}

void ModelDownloaderTest::_testCancelDownload()
{
    QTemporaryDir dir;
    ModelDownloader downloader;
    downloader.setTargetDirectory(dir.path());

    // Start a download and cancel immediately
    QUrl url = ModelDownloader::buildHfUrl("google/gemma-4-E2B-it-GGUF", "gemma-4-E2B-it-Q4_K_M.gguf");
    downloader.download(url);
    QVERIFY(downloader.downloading());

    MultiSignalSpy spy;
    QVERIFY(spy.init(&downloader, {downloader.downloadFailed(static_cast<QString>())}));

    downloader.cancel();

    QCOMPARE(spy.waitForSignalByIndex(0, 5000), true);
    QVERIFY(!downloader.downloading());
}
```

---

### Task 6.13: Register Tests in test/AIAgent/CMakeLists.txt

**File:** Modify `test/AIAgent/CMakeLists.txt`

Add to `target_sources(${CMAKE_PROJECT_NAME} PRIVATE ...)`:
```cmake
AISettingsTest.cc
AISettingsTest.h
ModelManagerTest.cc
ModelManagerTest.h
ModelDownloaderTest.cc
ModelDownloaderTest.h
```

Add Settings include directory for AISettingsTest:
```cmake
target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/Settings
)
```

Add `Qt6::Network` to `target_link_libraries` for `ModelDownloader` tests:
```cmake
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        nlohmann_json::nlohmann_json
        Qt6::Network
)
```

**File:** Modify `test/CMakeLists.txt`

Add test registration after the AIAgent section:
```cmake
add_qgc_test(AISettingsTest LABELS Unit AIAgent RESOURCE_LOCK Settings)
add_qgc_test(ModelManagerTest LABELS Unit AIAgent)
add_qgc_test(ModelDownloaderTest LABELS Integration Network AIAgent)
```

---

### Task 6.14: Docker Build Verification

**Objective:** Full Docker rebuild to confirm everything compiles and links.

**Commands:**

```bash
# 1. Reconfigure CMake (Debug required for QGC_BUILD_TESTING)
sg docker -c "docker run --rm --entrypoint bash \
  -v ${PWD}/agentGCS:/project/source \
  -v ${PWD}/build:/project/build \
  -v ${PWD}/models:/models \
  qgc-ubuntu-docker -l -c '\
    qt-cmake -S /project/source -B /project/build -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON \
  '"

# 2. Build main module + settings
sg docker -c "docker run --rm --entrypoint bash \
  -v ${PWD}/agentGCS:/project/source \
  -v ${PWD}/build:/project/build \
  -v ${PWD}/models:/models \
  qgc-ubuntu-docker -l -c '\
    ninja -C /project/build QGroundControlModule AIAgentModule AppSettingsModule \
  '"

# 3. Build test targets
sg docker -c "docker run --rm --entrypoint bash \
  -v ${PWD}/agentGCS:/project/source \
  -v ${PWD}/build:/project/build \
  -v ${PWD}/models:/models \
  qgc-ubuntu-docker -l -c '\
    ninja -C /project/build \
      test_model_load test_tool_call \
  '"

# 4. Run QGC unit tests
sg docker -c "docker run --rm --entrypoint bash \
  -v ${PWD}/agentGCS:/project/source \
  -v ${PWD}/build:/project/build \
  -v ${PWD}/models:/models \
  qgc-ubuntu-docker -l -c '\
    /project/build/Debug/QGroundControl --unittest:AISettingsTest && \
    /project/build/Debug/QGroundControl --unittest:ModelManagerTest \
  '"

# 5. Run ModelDownloader test (may need network access — skip in offline CI)
sg docker -c "docker run --rm --entrypoint bash \
  -v ${PWD}/agentGCS:/project/source \
  -v ${PWD}/build:/project/build \
  -v ${PWD}/models:/models \
  qgc-ubuntu-docker -l -c '\
    /project/build/Debug/QGroundControl --unittest:ModelDownloaderTest \
  '"
```

**Verification criteria:**
- All targets compile and link without errors
- `AISettingsTest` defaults and persistence pass
- `ModelManagerTest` GGUF validation passes
- `ModelDownloaderTest::testBuildHfUrl` passes
- `ModelDownloaderTest::testDownloadSmallFile` passes if network is available (Integration label)
- `ModelDownloaderTest::testCancelDownload` passes

---

### Task 6.15: Commit

**Objective:** Commit all PR 6 changes.

**Files to commit:**
```
src/Settings/AISettings.h
src/Settings/AISettings.cpp
src/Settings/AI.SettingsGroup.json
src/Settings/ModelManager.h
src/Settings/ModelManager.cpp
src/Settings/ModelDownloader.h
src/Settings/ModelDownloader.cpp
src/Settings/SettingsManager.h
src/Settings/SettingsManager.cc
src/Settings/CMakeLists.txt
src/AIAgent/LLMEngine.h
src/AIAgent/LLMEngine.cpp
src/AIAgent/CMakeLists.txt
src/UI/AppSettings/pages/AI.SettingsUI.json
src/UI/AppSettings/pages/SettingsPages.json
src/UI/AppSettings/AIModelDownloadSection.qml
src/UI/AppSettings/CMakeLists.txt
test/AIAgent/AISettingsTest.h
test/AIAgent/AISettingsTest.cc
test/AIAgent/ModelManagerTest.h
test/AIAgent/ModelManagerTest.cc
test/AIAgent/ModelDownloaderTest.h
test/AIAgent/ModelDownloaderTest.cc
test/AIAgent/CMakeLists.txt
test/CMakeLists.txt
```

```bash
git add src/Settings/AISettings.h src/Settings/AISettings.cpp \
        src/Settings/AI.SettingsGroup.json \
        src/Settings/ModelManager.h src/Settings/ModelManager.cpp \
        src/Settings/ModelDownloader.h src/Settings/ModelDownloader.cpp \
        src/Settings/SettingsManager.h src/Settings/SettingsManager.cc \
        src/Settings/CMakeLists.txt \
        src/AIAgent/LLMEngine.h src/AIAgent/LLMEngine.cpp \
        src/AIAgent/CMakeLists.txt \
        src/UI/AppSettings/pages/AI.SettingsUI.json \
        src/UI/AppSettings/pages/SettingsPages.json \
        src/UI/AppSettings/AIModelDownloadSection.qml \
        src/UI/AppSettings/CMakeLists.txt \
        test/AIAgent/AISettingsTest.h test/AIAgent/AISettingsTest.cc \
        test/AIAgent/ModelManagerTest.h test/AIAgent/ModelManagerTest.cc \
        test/AIAgent/ModelDownloaderTest.h test/AIAgent/ModelDownloaderTest.cc \
        test/AIAgent/CMakeLists.txt test/CMakeLists.txt
git commit -m "feat: add AI settings FactGroup, ModelManager, ModelDownloader, and settings UI panel"
```

---

## File Summary

### New Files

| File | Purpose |
|------|---------|
| `src/Settings/AISettings.h` | SettingsGroup header with 7 facts |
| `src/Settings/AISettings.cpp` | SettingsGroup implementation |
| `src/Settings/AI.SettingsGroup.json` | FactMetaData JSON for all settings |
| `src/Settings/ModelManager.h` | Model directory scanner & GGUF validator |
| `src/Settings/ModelManager.cpp` | ModelManager implementation |
| `src/Settings/ModelDownloader.h` | HuggingFace download with progress |
| `src/Settings/ModelDownloader.cpp` | ModelDownloader implementation |
| `src/UI/AppSettings/pages/AI.SettingsUI.json` | Settings page JSON definition |
| `src/UI/AppSettings/AIModelDownloadSection.qml` | Hand-coded model management QML |
| `test/AIAgent/AISettingsTest.h` | Settings persistence unit test header |
| `test/AIAgent/AISettingsTest.cc` | Settings persistence unit test |
| `test/AIAgent/ModelManagerTest.h` | ModelManager unit test header |
| `test/AIAgent/ModelManagerTest.cc` | ModelManager unit test |
| `test/AIAgent/ModelDownloaderTest.h` | ModelDownloader unit test header |
| `test/AIAgent/ModelDownloaderTest.cc` | ModelDownloader unit test |

### Modified Files

| File | Change |
|------|--------|
| `src/Settings/SettingsManager.h` | Add AISettings forward decl, Q_PROPERTY, getter, member |
| `src/Settings/SettingsManager.cc` | Add include, instantiation, getter |
| `src/Settings/CMakeLists.txt` | Add AISettings, ModelManager, ModelDownloader sources |
| `src/AIAgent/LLMEngine.h` | Add threadCount, temperature, topP properties |
| `src/AIAgent/LLMEngine.cpp` | Add setters, use settings-based sampler/threads |
| `src/AIAgent/CMakeLists.txt` | (no changes needed — ModelManager/ModelDownloader are in Settings) |
| `src/UI/AppSettings/pages/SettingsPages.json` | Add "AI Agent" page entry |
| `src/UI/AppSettings/CMakeLists.txt` | Add AISettings.qml to generated list, AIModelDownloadSection.qml to hand-coded |
| `test/AIAgent/CMakeLists.txt` | Add test sources, Settings include, Qt6::Network |
| `test/CMakeLists.txt` | Register 3 new test targets |

---

## Dependencies & Risks

- **Task 6.4 → 6.8:** SettingsManager must be updated before QML can reference `aiSettings`
- **Task 6.8 depends on code generator:** The Settings QML generator must support `AI.SettingsUI.json`. If it doesn't handle all control types (especially `browse` for string paths), we need to generate the page manually or extend the generator.
- **Cross-module placement:** ModelManager and ModelDownloader placed in `src/Settings/` to avoid circular dependencies between Settings and AIAgent modules.
- **Network tests:** `ModelDownloaderTest` requires network access; marked with `Integration Network` label. May need a mock/stub for CI environments without internet.
- **SHA256 verification:** For large model files (1.8–3.5GB), reading the file back for checksum verification is inefficient. A future PR should add streaming SHA256 hash during download.
- **QML generator compatibility:** If the generator doesn't support the `component` group type for `AIModelDownloadSection`, we need to hand-code the entire `AISettings.qml` page instead of using the JSON generator.
- **LLMEngine thread count:** The `n_threads` parameter in `llama_context_default_params()` was added in llama.cpp. Verify it exists in the pinned version.

## Not In Scope

- QML `Binding` wiring between AISettings Facts and LLMEngine (future PR with AIAgentController)
- Streaming SHA256 hash during download (optimization for large files)
- Model download resume (partial file reuse)
- Multiple model management (slots / profiles)
- HuggingFace API integration (model search, version listing)
- GPU detection / auto-gpu-layers heuristic