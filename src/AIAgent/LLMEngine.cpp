#include "LLMEngine.h"

#include <llama.h>
#include "chat.h"
#include "common.h"
#include <nlohmann/json.hpp>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QMetaObject>

LLMEngine::LLMEngine(QObject* parent)
    : QObject(parent)
{
    // Register ChatMessage for queuedsignal connections across threads
    static const int chatMessageTypeId = qRegisterMetaType<ChatMessage>("ChatMessage");
    Q_UNUSED(chatMessageTypeId);
}

LLMEngine::~LLMEngine()
{
    cancelCompletion();
    unloadModel();
}

// ---------------------------------------------------------------------------
// Property setters
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Model loading / unloading
// ---------------------------------------------------------------------------

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
    auto chatTemplatesPtr = common_chat_templates_init(m_model, "");
    m_chatTemplates = chatTemplatesPtr.release();
    if (!m_chatTemplates) {
        qWarning() << "LLMEngine: No chat template found in model metadata."
                    << "Tool calling will not be available.";
    }

    // --- Create default sampler chain ---
    resetSampler();

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

    if (m_sampler) {
        llama_sampler_free(m_sampler);
        m_sampler = nullptr;
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

// ---------------------------------------------------------------------------
// Sampler management
// ---------------------------------------------------------------------------

void LLMEngine::resetSampler()
{
    if (m_sampler) {
        llama_sampler_free(m_sampler);
        m_sampler = nullptr;
    }

    auto sparams = llama_sampler_chain_default_params();
    sparams.no_perf = true;
    m_sampler = llama_sampler_chain_init(sparams);

    // Default sampling chain for chat: top-k → top-p → temp → dist
    llama_sampler_chain_add(m_sampler, llama_sampler_init_top_k(50));
    llama_sampler_chain_add(m_sampler, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(m_sampler, llama_sampler_init_temp(0.6f));
    llama_sampler_chain_add(m_sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
}

// ---------------------------------------------------------------------------
// Completion
// ---------------------------------------------------------------------------

void LLMEngine::startCompletion(const QList<ChatMessage>& messages)
{
    if (!m_isLoaded) {
        emit loadFailed(QStringLiteral("Cannot start completion: no model loaded"));
        return;
    }

    if (m_isGenerating) {
        qWarning() << "LLMEngine: Completion already in progress. Cancel first.";
        return;
    }

    m_cancelled.store(false);

    // Always invoke on the engine's own thread (handles QML thread and moveToThread)
    QMetaObject::invokeMethod(this, [this, messages]() {
        runCompletion(messages);
    }, Qt::QueuedConnection);
}

void LLMEngine::cancelCompletion()
{
    m_cancelled.store(true);
}

void LLMEngine::setIsGenerating(bool generating)
{
    if (m_isGenerating == generating) return;
    m_isGenerating = generating;
    emit isGeneratingChanged();
}

void LLMEngine::runCompletion(const QList<ChatMessage>& messages)
{
    setIsGenerating(true);

    // --- 1. Build common_chat_msg list from ChatMessage input ---
    std::vector<common_chat_msg> chatMsgs;
    chatMsgs.reserve(messages.size());

    for (const auto& msg : messages) {
        common_chat_msg cm;
        cm.role = msg.role.toStdString();
        cm.content = msg.content.toStdString();
        if (!msg.toolName.isEmpty()) {
            cm.tool_name = msg.toolName.toStdString();
        }
        if (!msg.toolCallId.isEmpty()) {
            cm.tool_call_id = msg.toolCallId.toStdString();
        }
        chatMsgs.push_back(std::move(cm));
    }

    // --- 2. Apply chat template to get the formatted prompt ---
    std::string formattedPrompt;

    if (m_chatTemplates) {
        common_chat_templates_inputs inputs;
        inputs.messages = chatMsgs;
        inputs.add_generation_prompt = true;

        auto chatParams = common_chat_templates_apply(m_chatTemplates, inputs);
        formattedPrompt = chatParams.prompt;
    } else {
        // Fallback: concatenate messages as plain text (no template)
        for (const auto& cm : chatMsgs) {
            formattedPrompt += cm.role + ": " + cm.content + "\n";
        }
        formattedPrompt += "assistant: ";
    }

    // --- 3. Tokenize the prompt ---
    const auto* vocab = llama_model_get_vocab(m_model);
    const bool addBos = llama_vocab_get_add_bos(vocab);

    auto promptTokens = common_tokenize(vocab, formattedPrompt, addBos, true);

    // Check context capacity
    const int32_t nCtx = static_cast<int32_t>(llama_n_ctx(m_ctx));
    if (static_cast<int32_t>(promptTokens.size()) >= nCtx) {
        emit loadFailed(QStringLiteral("Prompt too long (%1 tokens) for context (%2)")
                            .arg(promptTokens.size()).arg(nCtx));
        setIsGenerating(false);
        return;
    }

    // --- 4. Create batch for the full prompt ---
    auto batch = llama_batch_get_one(promptTokens.data(), static_cast<int32_t>(promptTokens.size()));

    // --- 5. Decode the prompt (prefill) ---
    if (llama_decode(m_ctx, batch) != 0) {
        emit loadFailed(QStringLiteral("Failed to decode prompt batch"));
        setIsGenerating(false);
        return;
    }

    // --- 6. Generate tokens in a loop ---
    QString generatedText;
    int nGenerated = 0;
    const int maxTokens = nCtx - static_cast<int32_t>(promptTokens.size());

    while (nGenerated < maxTokens) {
        // Check cancellation
        if (m_cancelled.load()) {
            qDebug() << "LLMEngine: Completion cancelled after" << nGenerated << "tokens";
            break;
        }

        // Sample the next token
        llama_token newToken = llama_sampler_sample(m_sampler, m_ctx, -1);

        // Check for end-of-generation
        if (llama_vocab_is_eog(vocab, newToken)) {
            break;
        }

        // Convert token to text
        const std::string tokenStr = common_token_to_piece(m_ctx, newToken, false);
        const QString tokenQStr = QString::fromStdString(tokenStr);

        generatedText += tokenQStr;
        emit tokenGenerated(tokenQStr);

        // Accept the token into the sampler state
        llama_sampler_accept(m_sampler, newToken);

        // Prepare the next batch (single token)
        auto nextBatch = llama_batch_get_one(&newToken, 1);

        if (llama_decode(m_ctx, nextBatch) != 0) {
            qWarning() << "LLMEngine: Failed to decode token at position" << nGenerated;
            break;
        }

        nGenerated++;
    }

    // --- 7. Parse the generated text for tool calls (if chat template supports it) ---
    if (m_chatTemplates && !generatedText.isEmpty()) {
        common_chat_params chatParams;
        if (m_chatTemplates) {
            common_chat_templates_inputs dummyInputs;
            dummyInputs.add_generation_prompt = false;
            chatParams = common_chat_templates_apply(m_chatTemplates, dummyInputs);
        }

        common_chat_parser_params parserParams(chatParams);
        parserParams.parse_tool_calls = true;

        // Parse the full generated text for tool calls
        const common_chat_msg parsed = common_chat_parse(generatedText.toStdString(), false, parserParams);

        if (!parsed.tool_calls.empty()) {
            // Emit tool call signals
            for (const auto& tc : parsed.tool_calls) {
                emit toolCallDetected(
                    QString::fromStdString(tc.name),
                    QString::fromStdString(tc.arguments)
                );
            }
            // Reset sampler for next turn
            llama_sampler_reset(m_sampler);
            setIsGenerating(false);
            emit generationComplete(generatedText);
            return;
        }
    }

    // --- 8. Done ---
    llama_sampler_reset(m_sampler);
    setIsGenerating(false);
    emit generationComplete(generatedText);
}