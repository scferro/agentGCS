#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtQmlIntegration/QtQmlIntegration>

class ModelManager;

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
    /// @param url Full HuggingFace download URL
    /// @param expectedSha256 Optional SHA256 hex string for verification
    Q_INVOKABLE void download(const QUrl& url, const QString& expectedSha256 = QString());

    /// Cancel the current download.
    Q_INVOKABLE void cancel();

    /// Build a HuggingFace download URL from repo and filename.
    Q_INVOKABLE static QUrl buildHfUrl(const QString& repo, const QString& filename);

signals:
    void downloadingChanged();
    void progressChanged();
    void statusTextChanged();
    void targetDirectoryChanged();

    /// Emitted when download completes successfully.
    void downloadComplete(const QString& filePath);

    /// Emitted when download fails or is cancelled.
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