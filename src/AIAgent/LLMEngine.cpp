#include "LLMEngine.h"

#include <llama.h>
#include "chat.h"
#include "common.h"
#include <nlohmann/json.hpp>

#include <QtCore/QDebug>
#include <QtCore/QFile>

LLMEngine::LLMEngine(QObject* parent)
    : QObject(parent)
{
}

LLMEngine::~LLMEngine()
{
    unloadModel();
}

void LLMEngine::setModelPath(const QString& path)
{
    if (m_modelPath == path) return;
    m_modelPath = path;
    emit modelPathChanged();
}

void LLMEngine::setContextLength(int n)
{
    if (m_contextLength == n) return;
    m_contextLength = n;
    emit contextLengthChanged();
}

void LLMEngine::setGpuLayers(int n)
{
    if (m_gpuLayers == n) return;
    m_gpuLayers = n;
    emit gpuLayersChanged();
}

bool LLMEngine::loadModel()
{
    if (m_isLoaded) return true;

    if (m_modelPath.isEmpty()) {
        emit loadFailed(QStringLiteral("No model path specified"));
        return false;
    }

    if (!QFile::exists(m_modelPath)) {
        const QString msg = QStringLiteral("Model file not found: %1").arg(m_modelPath);
        emit loadFailed(msg);
        return false;
    }

    // --- Load model ---
    const QByteArray pathUtf8 = m_modelPath.toUtf8();

    auto modelParams = llama_model_default_params();
    modelParams.n_gpu_layers = m_gpuLayers;
    modelParams.use_mmap = true;
    modelParams.use_mlock = false;

    m_model = llama_model_load_from_file(pathUtf8.constData(), modelParams);
    if (!m_model) {
        const QString msg = QStringLiteral("Failed to load model: %1").arg(m_modelPath);
        emit loadFailed(msg);
        return false;
    }

    // --- Create inference context ---
    auto ctxParams = llama_context_default_params();
    ctxParams.n_ctx = static_cast<uint32_t>(m_contextLength);
    ctxParams.n_batch = 512;
    ctxParams.n_ubatch = 128;
    ctxParams.n_seq_max = 1;
    ctxParams.offload_kqv = true;

    m_ctx = llama_init_from_model(m_model, ctxParams);
    if (!m_ctx) {
        const QString msg = QStringLiteral("Failed to create context for model: %1").arg(m_modelPath);
        llama_model_free(m_model);
        m_model = nullptr;
        emit loadFailed(msg);
        return false;
    }

    // --- Initialize chat templates (for Gemma 4 tool calling) ---
    // common_chat_templates_init reads the chat template from the GGUF metadata.
    // For Gemma 4 models, this provides native tool calling via COMMON_CHAT_FORMAT_PEG_GEMMA4.
    auto chatTemplatesPtr = common_chat_templates_init(m_model, "");
    // The unique_ptr frees on scope exit — we extract the raw pointer and take ownership.
    m_chatTemplates = chatTemplatesPtr.release();
    if (!m_chatTemplates) {
        // Non-fatal: chat templates are optional for basic generation,
        // but required for tool calling (PR 5).
        qWarning() << "LLMEngine: No chat template found in model metadata."
                    << "Tool calling will not be available.";
    }

    m_isLoaded = true;
    emit isLoadedChanged();
    return true;
}

void LLMEngine::unloadModel()
{
    if (m_chatTemplates) {
        common_chat_templates_free(m_chatTemplates);
        m_chatTemplates = nullptr;
    }

    if (m_ctx) {
        llama_free(m_ctx);
        m_ctx = nullptr;
    }

    if (m_model) {
        llama_model_free(m_model);
        m_model = nullptr;
    }

    if (m_isLoaded) {
        m_isLoaded = false;
        emit isLoadedChanged();
    }
}