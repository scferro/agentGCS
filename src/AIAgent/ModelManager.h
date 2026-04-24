#pragma once

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QQmlEngine>

/// @brief Discovers and validates GGUF model files in configurable directories.
///
/// Scans a directory for .gguf files, validates each has the GGUF magic header
/// (bytes "GGUF" at offset 0), and exposes the valid list to QML for display
/// in the AI settings panel.
class ModelManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /// List of valid GGUF model file paths found in the scan directory.
    Q_PROPERTY(QStringList modelFiles READ modelFiles NOTIFY modelFilesChanged)
    /// Human-readable status message for UI display.
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

    /// Directory to scan for models. Defaults to QStandardPaths::AppDataLocation/models
    Q_PROPERTY(QString scanDirectory READ scanDirectory WRITE setScanDirectory NOTIFY scanDirectoryChanged)

public:
    explicit ModelManager(QObject* parent = nullptr);
    static ModelManager* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
    static ModelManager* singletonInstance();

    QStringList modelFiles() const { return m_modelFiles; }
    QString statusMessage() const { return m_statusMessage; }
    QString scanDirectory() const { return m_scanDirectory; }
    void setScanDirectory(const QString& dir);

    /// Scan the directory for models and update modelFiles.
    Q_INVOKABLE void refresh();

    /// Convenience: set scan directory and refresh.
    Q_INVOKABLE void scanModels(const QString& directory);

    /// Check if a file has a valid GGUF magic header (first 4 bytes == "GGUF").
    static bool validateGgufHeader(const QString& filePath);

    /// Get the default model storage directory.
    static QString defaultModelDirectory();

signals:
    void modelFilesChanged();
    void statusMessageChanged();
    void scanDirectoryChanged();

private:
    QStringList m_modelFiles;
    QString m_statusMessage;
    QString m_scanDirectory;
};