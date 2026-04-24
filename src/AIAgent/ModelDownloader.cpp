#include "ModelDownloader.h"
#include "ModelManager.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QCryptographicHash>
#include <QtNetwork/QNetworkRequest>
#include <QDebug>

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

    const QString fileName = url.fileName();
    if (fileName.isEmpty()) {
        emit downloadFailed(QStringLiteral("Cannot determine filename from URL"));
        return;
    }

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
    if (m_currentReply && m_outputFile.isOpen()) {
        m_outputFile.write(m_currentReply->readAll());
    }
}

void ModelDownloader::_onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        double pct = static_cast<double>(bytesReceived) / static_cast<double>(bytesTotal);
        setProgress(pct);
        setStatusText(QStringLiteral("Downloading %1... %2%")
                          .arg(m_outputFile.fileName())
                          .arg(static_cast<int>(pct * 100)));
    }
}

void ModelDownloader::_onDownloadFinished()
{
    m_outputFile.close();

    QNetworkReply::NetworkError error = m_currentReply->error();
    QString errorString = m_currentReply->errorString();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    setDownloading(false);

    if (error != QNetworkReply::NoError) {
        // Clean up partial file
        QFile::remove(m_outputFile.fileName());
        setProgress(0.0);
        setStatusText(QString());
        emit downloadFailed(errorString);
        return;
    }

    // SHA256 verification if expected hash provided
    if (!m_expectedSha256.isEmpty()) {
        setStatusText(QStringLiteral("Verifying checksum..."));
        QFile file(m_outputFile.fileName());
        if (!file.open(QIODevice::ReadOnly)) {
            emit downloadFailed(QStringLiteral("Cannot open downloaded file for checksum verification"));
            return;
        }

        QCryptographicHash hash(QCryptographicHash::Sha256);
        while (!file.atEnd()) {
            hash.addData(file.read(1024 * 1024)); // 1MB chunks
        }
        file.close();

        QString computed = QString(hash.result().toHex());
        if (computed != m_expectedSha256.toLower()) {
            QFile::remove(m_outputFile.fileName());
            setStatusText(QString());
            emit downloadFailed(QStringLiteral("SHA256 mismatch: expected %1, got %2")
                                   .arg(m_expectedSha256, computed));
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