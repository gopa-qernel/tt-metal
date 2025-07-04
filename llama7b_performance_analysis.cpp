#include "performance_profiler.cpp"
#include "llm_model.cpp"

// Enhanced Compute Engine with profiling
class ProfiledComputeEngine : public ComputeEngine {
private:
    PerformanceProfiler* profiler;
    uint32_t core_id;
    
public:
    ProfiledComputeEngine(uint32_t id, PerformanceProfiler* prof) : ComputeEngine(id), core_id(id), profiler(prof) {}
    
    void matmul(const Tensor& a, const Tensor& b, Tensor& c) override {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Call parent implementation
        ComputeEngine::matmul(a, b, c);
        
        auto end = std::chrono::high_resolution_clock::now();
        double execution_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Record profiling data
        if (profiler && a.shape.size() >= 2 && b.shape.size() >= 2) {
            uint32_t m = a.shape[0], k = a.shape[1], n = b.shape[1];
            profiler->recordMatMul(m, k, n, execution_time, core_id);
        }
    }
    
    void elementwise_add(const Tensor& a, const Tensor& b, Tensor& c) override {
        auto start = std::chrono::high_resolution_clock::now();
        
        ComputeEngine::elementwise_add(a, b, c);
        
        auto end = std::chrono::high_resolution_clock::now();
        double execution_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        if (profiler) {
            uint64_t elements = a.getTotalSize() / a.getElementSize(a.format);
            profiler->recordElementWise(elements, execution_time, "ElementWise_Add");
        }
    }
    
    void relu(const Tensor& input, Tensor& output) override {
        auto start = std::chrono::high_resolution_clock::now();
        
        ComputeEngine::relu(input, output);
        
        auto end = std::chrono::high_resolution_clock::now();
        double execution_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        if (profiler) {
            uint64_t elements = input.getTotalSize() / input.getElementSize(input.format);
            profiler->recordActivation(elements, execution_time, "relu");
        }
    }
    
    void gelu(const Tensor& input, Tensor& output) override {
        auto start = std::chrono::high_resolution_clock::now();
        
        ComputeEngine::gelu(input, output);
        
        auto end = std::chrono::high_resolution_clock::now();
        double execution_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        if (profiler) {
            uint64_t elements = input.getTotalSize() / input.getElementSize(input.format);
            profiler->recordActivation(elements, execution_time, "gelu");
        }
    }
};

// LLaMA-7B Performance Analyzer
class LLaMA7BPerformanceAnalyzer {
private:
    TensixDevice& device;
    PerformanceProfiler profiler;
    
    // LLaMA-7B specific configuration
    ModelConfig llama7b_config = {
        .vocab_size = 32000,
        .hidden_dim = 4096,
        .intermediate_dim = 11008,
        .num_layers = 32,
        .num_heads = 32,
        .head_dim = 128,
        .max_seq_len = 2048,
        .weight_format = DataFormat::FP16,
        .activation_format = DataFormat::FP16
    };
    
public:
    LLaMA7BPerformanceAnalyzer(TensixDevice& dev) : device(dev) {}
    
    struct LLaMA7BMetrics {
        // Token throughput
        double tokens_per_second;
        double time_per_token_ms;
        double ttft_ms; // Time to first token
        
        // Cycle breakdown
        uint64_t total_cycles;
        uint64_t matmul_cycles;
        uint64_t elementwise_cycles;
        uint64_t activation_cycles;
        uint64_t data_movement_cycles;
        
        // Percentages
        double matmul_percentage;
        double elementwise_percentage;
        double activation_percentage;
        double data_movement_percentage;
        
        // Performance metrics
        double achieved_tflops;
        double hardware_utilization;
        double memory_bandwidth_utilization;
        
        // Model-specific metrics
        uint64_t model_parameters;
        double flops_per_token;
        double memory_bandwidth_per_token;
    };
    
    LLaMA7BMetrics analyzeSingleTokenGeneration() {
        std::cout << "Starting LLaMA-7B Performance Analysis on Wormhole n150...\n\n";
        
        profiler.startProfiling();
        
        // Create LLaMA model with profiling
        LlamaModel model(llama7b_config, device);
        
        // Single token generation with realistic prompt
        std::vector<uint32_t> prompt = {1, 15234, 567, 89, 123, 456, 789, 321}; // 8 tokens
        
        auto start_generation = std::chrono::high_resolution_clock::now();
        
        // Generate single token (most realistic for measuring per-token latency)
        Tensor logits({1, (uint32_t)prompt.size(), llama7b_config.vocab_size}, llama7b_config.activation_format);
        auto& core0 = device.getCore(0);
        core0.allocateTensor(logits);
        
        model.forward(prompt, logits);
        
        auto end_generation = std::chrono::high_resolution_clock::now();
        device.waitForAllCores();
        
        profiler.endProfiling();
        
        auto generation_time = std::chrono::duration<double, std::milli>(end_generation - start_generation).count();
        
        // Extract metrics
        auto throughput_metrics = profiler.getThroughputMetrics();
        LLaMA7BMetrics metrics;
        
        // Token throughput
        metrics.tokens_per_second = throughput_metrics.tokens_per_second;
        metrics.time_per_token_ms = generation_time; // Real measured time
        metrics.ttft_ms = generation_time; // Same for single token
        
        // Cycle breakdown
        metrics.total_cycles = throughput_metrics.total_cycles;
        metrics.matmul_cycles = throughput_metrics.matmul_cycles;
        metrics.elementwise_cycles = throughput_metrics.elementwise_cycles;
        
        // Calculate activation and data movement cycles
        metrics.activation_cycles = calculateActivationCycles();
        metrics.data_movement_cycles = calculateDataMovementCycles();
        
        // Percentages
        metrics.matmul_percentage = throughput_metrics.matmul_percentage;
        metrics.elementwise_percentage = throughput_metrics.elementwise_percentage;
        metrics.activation_percentage = (metrics.activation_cycles / static_cast<double>(metrics.total_cycles)) * 100;
        metrics.data_movement_percentage = (metrics.data_movement_cycles / static_cast<double>(metrics.total_cycles)) * 100;
        
        // Performance metrics
        metrics.hardware_utilization = throughput_metrics.compute_utilization;
        metrics.achieved_tflops = calculateAchievedTFLOPS(generation_time);
        metrics.memory_bandwidth_utilization = calculateMemoryBandwidthUtilization();
        
        // Model-specific metrics
        metrics.model_parameters = calculateModelParameters();
        metrics.flops_per_token = calculateFLOPsPerToken();
        metrics.memory_bandwidth_per_token = calculateMemoryBandwidthPerToken();
        
        return metrics;
    }
    
    void printComprehensiveReport() {
        auto metrics = analyzeSingleTokenGeneration();
        
        std::cout << "\n" << std::string(100, '=') << "\n";
        std::cout << "                           LLaMA-7B PERFORMANCE ANALYSIS\n";
        std::cout << "                              Wormhole n150 (72 Cores)\n";
        std::cout << std::string(100, '=') << "\n\n";
        
        // Model specifications
        std::cout << "MODEL SPECIFICATIONS:\n";
        std::cout << std::string(50, '-') << "\n";
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "Parameters:                  " << metrics.model_parameters / 1e9 << "B\n";
        std::cout << "Hidden Dimension:            " << llama7b_config.hidden_dim << "\n";
        std::cout << "Number of Layers:            " << llama7b_config.num_layers << "\n";
        std::cout << "Attention Heads:             " << llama7b_config.num_heads << "\n";
        std::cout << "Vocabulary Size:             " << llama7b_config.vocab_size << "\n";
        std::cout << "Precision:                   FP16\n\n";
        
        // TOKEN THROUGHPUT RESULTS
        std::cout << "TOKEN THROUGHPUT ANALYSIS:\n";
        std::cout << std::string(50, '-') << "\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Tokens per Second:           " << metrics.tokens_per_second << " tokens/s\n";
        std::cout << "Time per Token:              " << metrics.time_per_token_ms << " ms\n";
        std::cout << "Time to First Token (TTFT): " << metrics.ttft_ms << " ms\n";
        std::cout << "Target Performance:          25.0 tokens/s/user (Wormhole spec)\n";
        std::cout << "Performance vs Target:       " << (metrics.tokens_per_second / 25.0) * 100 << "%\n\n";
        
        // CYCLE BREAKDOWN - CORE ANSWER TO THE QUESTION
        std::cout << "CYCLE BREAKDOWN ANALYSIS:\n";
        std::cout << std::string(50, '-') << "\n";
        std::cout << "Total Execution Cycles:      " << metrics.total_cycles << " cycles\n";
        std::cout << "MatMul Operations:           " << metrics.matmul_cycles << " cycles (" << metrics.matmul_percentage << "%)\n";
        std::cout << "Element-wise Operations:     " << metrics.elementwise_cycles << " cycles (" << metrics.elementwise_percentage << "%)\n";
        std::cout << "Activation Functions:        " << metrics.activation_cycles << " cycles (" << metrics.activation_percentage << "%)\n";
        std::cout << "Data Movement (NoC):         " << metrics.data_movement_cycles << " cycles (" << metrics.data_movement_percentage << "%)\n\n";
        
        // OPERATION TYPE BREAKDOWN
        std::cout << "MATMUL vs OTHER OPERATIONS BREAKDOWN:\n";
        std::cout << std::string(50, '-') << "\n";
        double non_matmul_percentage = 100.0 - metrics.matmul_percentage;
        std::cout << "MatMul Operations:           " << metrics.matmul_percentage << "% of total cycles\n";
        std::cout << "All Other Operations:        " << non_matmul_percentage << "% of total cycles\n";
        std::cout << "  - Element-wise Ops:        " << metrics.elementwise_percentage << "%\n";
        std::cout << "  - Activation Functions:    " << metrics.activation_percentage << "%\n";
        std::cout << "  - Data Movement:           " << metrics.data_movement_percentage << "%\n";
        std::cout << "  - Other (overhead):        " << non_matmul_percentage - metrics.elementwise_percentage - 
                     metrics.activation_percentage - metrics.data_movement_percentage << "%\n\n";
        
        // PERFORMANCE METRICS
        std::cout << "HARDWARE UTILIZATION:\n";
        std::cout << std::string(50, '-') << "\n";
        std::cout << "Achieved Performance:        " << metrics.achieved_tflops << " TFLOPS\n";
        std::cout << "Peak Theoretical:            74.0 TFLOPS (FP16)\n";
        std::cout << "Hardware Utilization:        " << metrics.hardware_utilization << "%\n";
        std::cout << "Memory Bandwidth Used:       " << metrics.memory_bandwidth_utilization << "%\n\n";
        
        // PER-TOKEN ANALYSIS
        std::cout << "PER-TOKEN COMPUTATIONAL REQUIREMENTS:\n";
        std::cout << std::string(50, '-') << "\n";
        std::cout << "FLOPs per Token:             " << metrics.flops_per_token / 1e9 << " GFLOP\n";
        std::cout << "Memory Bandwidth per Token:  " << metrics.memory_bandwidth_per_token << " GB\n";
        std::cout << "Arithmetic Intensity:        " << (metrics.flops_per_token / (metrics.memory_bandwidth_per_token * 1e9)) << " FLOP/byte\n\n";
        
        // DETAILED MATMUL ANALYSIS
        analyzeMatMulOperations();
        
        // OPTIMIZATION RECOMMENDATIONS
        std::cout << "OPTIMIZATION RECOMMENDATIONS:\n";
        std::cout << std::string(50, '-') << "\n";
        if (metrics.matmul_percentage > 80) {
            std::cout << "• PRIMARY BOTTLENECK: MatMul operations (compute-bound)\n";
            std::cout << "• Optimize matrix multiplication parallelization\n";
            std::cout << "• Increase tensor parallel degree\n";
            std::cout << "• Optimize attention head distribution across cores\n";
        } else if (metrics.data_movement_percentage > 20) {
            std::cout << "• PRIMARY BOTTLENECK: Data movement (memory-bound)\n";
            std::cout << "• Optimize NoC communication patterns\n";
            std::cout << "• Increase L1 cache utilization\n";
            std::cout << "• Implement weight streaming\n";
        } else {
            std::cout << "• BALANCED workload across operation types\n";
            std::cout << "• Focus on overall pipeline optimization\n";
        }
        
        std::cout << "• Enable KV-cache for multi-token generation\n";
        std::cout << "• Implement layer-wise pipelining\n";
        std::cout << "• Use mixed precision (FP8) where possible\n\n";
        
        std::cout << std::string(100, '=') << "\n";
        
        // Print detailed profiling report
        profiler.printDetailedReport();
    }
    
private:
    uint64_t calculateModelParameters() {
        // LLaMA-7B parameter count calculation
        uint64_t vocab_embedding = static_cast<uint64_t>(llama7b_config.vocab_size) * llama7b_config.hidden_dim;
        uint64_t layer_params = 0;
        
        for (uint32_t i = 0; i < llama7b_config.num_layers; ++i) {
            // Attention weights: q_proj, k_proj, v_proj, o_proj
            layer_params += 4 * static_cast<uint64_t>(llama7b_config.hidden_dim) * llama7b_config.hidden_dim;
            
            // Feed-forward weights: gate, up, down
            layer_params += static_cast<uint64_t>(llama7b_config.hidden_dim) * llama7b_config.intermediate_dim * 2; // gate + up
            layer_params += static_cast<uint64_t>(llama7b_config.intermediate_dim) * llama7b_config.hidden_dim; // down
            
            // Layer norms (simplified)
            layer_params += llama7b_config.hidden_dim * 2; // pre-attention and pre-ffn
        }
        
        uint64_t output_projection = static_cast<uint64_t>(llama7b_config.hidden_dim) * llama7b_config.vocab_size;
        
        return vocab_embedding + layer_params + output_projection;
    }
    
    double calculateFLOPsPerToken() {
        // For transformer models, FLOPs per token ≈ 2 * num_parameters
        // This accounts for the forward pass computation
        return 2.0 * calculateModelParameters();
    }
    
    double calculateMemoryBandwidthPerToken() {
        // Memory bandwidth = reading all parameters + activations
        uint64_t parameters = calculateModelParameters();
        uint64_t activation_memory = static_cast<uint64_t>(llama7b_config.hidden_dim) * 128 * llama7b_config.num_layers; // Rough estimate
        
        return (parameters + activation_memory) * 2 / 1e9; // FP16 = 2 bytes, convert to GB
    }
    
    double calculateAchievedTFLOPS(double execution_time_ms) {
        double flops_per_token = calculateFLOPsPerToken();
        double execution_time_s = execution_time_ms / 1000.0;
        return (flops_per_token / 1e12) / execution_time_s;
    }
    
    double calculateMemoryBandwidthUtilization() {
        // Simplified calculation based on theoretical requirements
        double required_bandwidth = calculateMemoryBandwidthPerToken();
        double peak_bandwidth = 288.0; // GB/s for Wormhole DRAM
        return std::min(100.0, (required_bandwidth / peak_bandwidth) * 100);
    }
    
    uint64_t calculateActivationCycles() {
        // Estimate activation function cycles based on model architecture
        uint64_t elements_per_layer = static_cast<uint64_t>(llama7b_config.hidden_dim) * 128; // sequence length
        uint64_t total_activations = elements_per_layer * llama7b_config.num_layers * 2; // GELU + others
        return total_activations / 8; // SFPU throughput
    }
    
    uint64_t calculateDataMovementCycles() {
        // Estimate NoC data movement cycles
        uint64_t weight_transfers = calculateModelParameters() * 2 / (1024 * 1024); // MB transfers
        double noc_bandwidth_gbps = 200.0;
        return static_cast<uint64_t>(weight_transfers / noc_bandwidth_gbps * 1e9);
    }
    
    void analyzeMatMulOperations() {
        std::cout << "DETAILED MATMUL ANALYSIS:\n";
        std::cout << std::string(50, '-') << "\n";
        
        // Calculate major MatMul operations in LLaMA-7B
        uint32_t seq_len = 128;
        uint32_t hidden_dim = llama7b_config.hidden_dim;
        uint32_t intermediate_dim = llama7b_config.intermediate_dim;
        uint32_t num_layers = llama7b_config.num_layers;
        
        // Attention MatMuls per layer
        uint64_t attention_flops_per_layer = 
            4 * (2ULL * seq_len * hidden_dim * hidden_dim) + // Q, K, V, O projections
            2 * (2ULL * seq_len * seq_len * hidden_dim);     // QK^T and attention*V
        
        // Feed-forward MatMuls per layer  
        uint64_t ffn_flops_per_layer = 
            2 * (2ULL * seq_len * hidden_dim * intermediate_dim) + // gate + up projections
            2ULL * seq_len * intermediate_dim * hidden_dim;       // down projection
        
        uint64_t total_attention_flops = attention_flops_per_layer * num_layers;
        uint64_t total_ffn_flops = ffn_flops_per_layer * num_layers;
        uint64_t total_matmul_flops = total_attention_flops + total_ffn_flops;
        
        std::cout << "Attention MatMuls:           " << total_attention_flops / 1e9 << " GFLOP (" 
                  << (total_attention_flops / static_cast<double>(total_matmul_flops)) * 100 << "% of MatMul)\n";
        std::cout << "Feed-Forward MatMuls:        " << total_ffn_flops / 1e9 << " GFLOP (" 
                  << (total_ffn_flops / static_cast<double>(total_matmul_flops)) * 100 << "% of MatMul)\n";
        std::cout << "Total MatMul FLOPs:          " << total_matmul_flops / 1e9 << " GFLOP\n\n";
    }
};

// Function to run comprehensive analysis
void runLLaMA7BAnalysis() {
    std::cout << "Initializing Wormhole n150 for LLaMA-7B Analysis...\n";
    TensixDevice device(72); // Wormhole n150 configuration
    
    LLaMA7BPerformanceAnalyzer analyzer(device);
    analyzer.printComprehensiveReport();
}