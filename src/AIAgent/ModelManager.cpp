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

ModelManager* ModelManager::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(jsEngine)
    static ModelManager* instance = nullptr;
    if (!instance) {
        instance = new ModelManager(qmlEngine);
    }
    return instance;
}

ModelManager* ModelManager::singletonInstance()
{
    return create(nullptr, nullptr);
}

QString ModelManager::defaultModelDirectory()
{
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

    m_statusMessage = QStringLiteral("Found %1 model(s)").arg(m_modelFiles.size());
    emit statusMessageChanged();
}

void ModelManager::scanModels(const QString& directory)
{
    if (!directory.isEmpty()) {
        setScanDirectory(directory);
    }
    refresh();
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