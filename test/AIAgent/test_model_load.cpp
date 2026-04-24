// Minimal smoke test: load a GGUF model and generate a few tokens.
// Build inside Docker, mount the models dir, run with a model path.
//
// Usage: ./test_model_load /path/to/model.gguf

#include <cstdio>
#include <string>
#include <vector>

#include <llama.h>
#include "common.h"
#include "chat.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <model.gguf>\n", argv[0]);
        return 1;
    }

    const char* model_path = argv[1];

    // --- Load model ---
    printf("Loading model: %s ...\n", model_path);
    fflush(stdout);

    auto model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0;
    model_params.use_mmap = true;

    auto* model = llama_model_load_from_file(model_path, model_params);
    if (!model) {
        fprintf(stderr, "FAIL: Could not load model\n");
        return 1;
    }
    printf("Model loaded successfully!\n");
    fflush(stdout);

    // --- Create context ---
    auto ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 2048;
    ctx_params.n_batch = 512;
    ctx_params.n_ubatch = 128;
    ctx_params.n_seq_max = 1;

    auto* ctx = llama_init_from_model(model, ctx_params);
    if (!ctx) {
        fprintf(stderr, "FAIL: Could not create context\n");
        llama_model_free(model);
        return 1;
    }
    printf("Context created (n_ctx=%u)\n", llama_n_ctx(ctx));
    fflush(stdout);

    // --- Init chat templates ---
    auto chat_tpl = common_chat_templates_init(model, "");
    auto* tpl = chat_tpl.get();
    if (tpl) {
        printf("Chat template: %s\n", common_chat_templates_source(tpl).c_str());
    } else {
        printf("Warning: No chat template in model metadata\n");
    }
    fflush(stdout);

    // --- Build a simple prompt ---
    std::vector<common_chat_msg> messages;
    common_chat_msg user_msg;
    user_msg.role = "user";
    user_msg.content = "Say hello in one word.";
    messages.push_back(user_msg);

    common_chat_templates_inputs inputs;
    inputs.messages = messages;
    inputs.add_generation_prompt = true;

    auto chat_params = common_chat_templates_apply(tpl, inputs);
    const std::string prompt = chat_params.prompt;
    printf("Formatted prompt (%zu chars)\n", prompt.size());

    // --- Tokenize ---
    const auto* vocab = llama_model_get_vocab(model);
    const bool add_bos = llama_vocab_get_add_bos(vocab);
    auto tokens = common_tokenize(vocab, prompt, add_bos, true);
    printf("Prompt tokens: %zu\n", tokens.size());
    fflush(stdout);

    // --- Prefill ---
    auto batch = llama_batch_get_one(tokens.data(), static_cast<int32_t>(tokens.size()));
    if (llama_decode(ctx, batch) != 0) {
        fprintf(stderr, "FAIL: Prefill decode failed\n");
        llama_free(ctx);
        llama_model_free(model);
        return 1;
    }
    printf("Prefill complete!\n");
    fflush(stdout);

    // --- Create sampler ---
    auto sparams = llama_sampler_chain_default_params();
    sparams.no_perf = true;
    auto* sampler = llama_sampler_chain_init(sparams);
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(50));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.6f));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    // --- Generate tokens ---
    printf("Generating: ");
    fflush(stdout);

    std::string generated;
    for (int i = 0; i < 64; ++i) {  // Max 64 tokens
        llama_token new_token = llama_sampler_sample(sampler, ctx, -1);
        if (llama_vocab_is_eog(vocab, new_token)) {
            break;
        }

        std::string piece = common_token_to_piece(ctx, new_token, false);
        generated += piece;
        printf("%s", piece.c_str());
        fflush(stdout);

        llama_sampler_accept(sampler, new_token);

        auto next_batch = llama_batch_get_one(&new_token, 1);
        if (llama_decode(ctx, next_batch) != 0) {
            fprintf(stderr, "\nFAIL: Token decode failed at pos %d\n", i);
            break;
        }
    }

    printf("\n---\nSUCCESS! Model loads and generates text.\n");

    // Cleanup
    llama_sampler_free(sampler);
    llama_free(ctx);
    llama_model_free(model);

    return 0;
}