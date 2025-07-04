#include "llama7b_performance_analysis.cpp"

// Enhanced MatMul Performance Analyzer
class EnhancedMatMulAnalyzer {
private:
    struct PerformanceScenario {
        std::string name;
        double matmul_speedup;
        double vector_speedup;
        double memory_speedup;
        std::string description;
    };
    
    // Baseline performance from previous analysis
    struct BaselineMetrics {
        uint64_t total_cycles = 40322000;
        uint64_t matmul_cycles = 32851200;      // 81.5%
        uint64_t elementwise_cycles = 2419320;  // 6.0%
        uint64_t activation_cycles = 3225760;   // 8.0%
        uint64_t data_movement_cycles = 1825720; // 4.5%
        double time_per_token_ms = 40.3;
        double tokens_per_second = 24.8;
        double memory_bandwidth_utilization = 23.2; // % of peak DRAM
    } baseline;
    
public:
    struct EnhancedMetrics {
        uint64_t new_total_cycles;
        uint64_t new_matmul_cycles;
        double new_time_per_token_ms;
        double new_tokens_per_second;
        double speedup_factor;
        double new_matmul_percentage;
        double new_bottleneck_percentage;
        std::string primary_bottleneck;
        double projected_memory_bandwidth;
        bool memory_bound;
        double hardware_utilization;
        double theoretical_max_speedup;
    };
    
    EnhancedMetrics analyze20xMatMulImprovement() {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "           ENHANCED MATMUL PERFORMANCE ANALYSIS\n";
        std::cout << "              20x MatMul Speedup Scenario\n";
        std::cout << std::string(80, '=') << "\n\n";
        
        EnhancedMetrics metrics;
        
        // Calculate new cycle counts with 20x MatMul improvement
        metrics.new_matmul_cycles = baseline.matmul_cycles / 20;
        
        // Other operations remain the same
        uint64_t other_cycles = baseline.elementwise_cycles + 
                               baseline.activation_cycles + 
                               baseline.data_movement_cycles;
        
        metrics.new_total_cycles = metrics.new_matmul_cycles + other_cycles;
        
        // Calculate performance improvements
        metrics.speedup_factor = static_cast<double>(baseline.total_cycles) / metrics.new_total_cycles;
        metrics.new_time_per_token_ms = baseline.time_per_token_ms / metrics.speedup_factor;
        metrics.new_tokens_per_second = 1000.0 / metrics.new_time_per_token_ms;
        
        // Calculate new percentages
        metrics.new_matmul_percentage = (metrics.new_matmul_cycles / static_cast<double>(metrics.new_total_cycles)) * 100;
        
        // Determine new bottleneck
        uint64_t max_other_cycles = std::max({baseline.elementwise_cycles, 
                                             baseline.activation_cycles, 
                                             baseline.data_movement_cycles});
        
        metrics.new_bottleneck_percentage = (max_other_cycles / static_cast<double>(metrics.new_total_cycles)) * 100;
        
        if (baseline.activation_cycles == max_other_cycles) {
            metrics.primary_bottleneck = "Activation Functions";
        } else if (baseline.elementwise_cycles == max_other_cycles) {
            metrics.primary_bottleneck = "Element-wise Operations";
        } else {
            metrics.primary_bottleneck = "Data Movement";
        }
        
        // Memory bandwidth analysis
        // 20x faster MatMul means 20x higher memory bandwidth requirement for MatMul ops
        double matmul_memory_fraction = 0.70; // ~70% of memory traffic is MatMul-related
        metrics.projected_memory_bandwidth = baseline.memory_bandwidth_utilization + 
                                           (matmul_memory_fraction * baseline.memory_bandwidth_utilization * 19); // 19x increase
        
        metrics.memory_bound = metrics.projected_memory_bandwidth > 100.0;
        
        // Hardware utilization (based on achieved TFLOPS)
        double baseline_tflops = 3.68;
        metrics.hardware_utilization = std::min(100.0, (baseline_tflops * metrics.speedup_factor / 74.0) * 100);
        
        // Theoretical max speedup (if MatMul was 100% of execution)
        metrics.theoretical_max_speedup = 1.0 / (1.0 - 0.815); // Amdahl's law approximation
        
        return metrics;
    }
    
    void printComprehensiveAnalysis() {
        auto enhanced = analyze20xMatMulImprovement();
        
        std::cout << "BASELINE vs ENHANCED PERFORMANCE COMPARISON:\n";
        std::cout << std::string(60, '-') << "\n";
        std::cout << std::fixed << std::setprecision(2);
        
        std::cout << "                              Baseline    Enhanced    Improvement\n";
        std::cout << "Time per Token:              " << std::setw(8) << baseline.time_per_token_ms << "ms  "
                  << std::setw(8) << enhanced.new_time_per_token_ms << "ms     "
                  << enhanced.speedup_factor << "x\n";
        std::cout << "Tokens per Second:           " << std::setw(8) << baseline.tokens_per_second << "     "
                  << std::setw(8) << enhanced.new_tokens_per_second << "       "
                  << enhanced.speedup_factor << "x\n";
        std::cout << "Total Execution Cycles:      " << std::setw(8) << baseline.total_cycles / 1e6 << "M   "
                  << std::setw(8) << enhanced.new_total_cycles / 1e6 << "M      "
                  << enhanced.speedup_factor << "x\n\n";
        
        std::cout << "DETAILED CYCLE BREAKDOWN:\n";
        std::cout << std::string(60, '-') << "\n";
        std::cout << "                              Baseline         Enhanced        Change\n";
        std::cout << "MatMul Cycles:               " << std::setw(10) << baseline.matmul_cycles 
                  << " (" << std::setw(5) << 81.5 << "%) " 
                  << std::setw(10) << enhanced.new_matmul_cycles 
                  << " (" << std::setw(5) << enhanced.new_matmul_percentage << "%) "
                  << "20.0x faster\n";
        
        std::cout << "Element-wise Cycles:         " << std::setw(10) << baseline.elementwise_cycles 
                  << " (" << std::setw(5) << 6.0 << "%) " 
                  << std::setw(10) << baseline.elementwise_cycles 
                  << " (" << std::setw(5) << (baseline.elementwise_cycles / static_cast<double>(enhanced.new_total_cycles)) * 100 << "%) "
                  << "No change\n";
        
        std::cout << "Activation Cycles:           " << std::setw(10) << baseline.activation_cycles 
                  << " (" << std::setw(5) << 8.0 << "%) " 
                  << std::setw(10) << baseline.activation_cycles 
                  << " (" << std::setw(5) << (baseline.activation_cycles / static_cast<double>(enhanced.new_total_cycles)) * 100 << "%) "
                  << "No change\n";
        
        std::cout << "Data Movement Cycles:        " << std::setw(10) << baseline.data_movement_cycles 
                  << " (" << std::setw(5) << 4.5 << "%) " 
                  << std::setw(10) << baseline.data_movement_cycles 
                  << " (" << std::setw(5) << (baseline.data_movement_cycles / static_cast<double>(enhanced.new_total_cycles)) * 100 << "%) "
                  << "No change\n\n";
        
        std::cout << "NEW BOTTLENECK ANALYSIS:\n";
        std::cout << std::string(60, '-') << "\n";
        std::cout << "Primary Bottleneck:          " << enhanced.primary_bottleneck << " ("
                  << enhanced.new_bottleneck_percentage << "% of total cycles)\n";
        std::cout << "MatMul now represents:       " << enhanced.new_matmul_percentage << "% of execution time\n";
        std::cout << "Speedup achieved:            " << enhanced.speedup_factor << "x out of " 
                  << enhanced.theoretical_max_speedup << "x theoretical maximum\n";
        std::cout << "Amdahl's Law efficiency:     " << (enhanced.speedup_factor / enhanced.theoretical_max_speedup) * 100 << "%\n\n";
        
        // Memory bandwidth analysis
        std::cout << "MEMORY BANDWIDTH IMPACT:\n";
        std::cout << std::string(60, '-') << "\n";
        std::cout << "Baseline Memory Usage:       " << baseline.memory_bandwidth_utilization << "% of peak DRAM\n";
        std::cout << "Projected Memory Usage:      " << enhanced.projected_memory_bandwidth << "% of peak DRAM\n";
        
        if (enhanced.memory_bound) {
            std::cout << "⚠️  MEMORY BOUND: System becomes memory bandwidth limited!\n";
            double memory_limited_speedup = 100.0 / baseline.memory_bandwidth_utilization;
            std::cout << "   Maximum achievable speedup: " << memory_limited_speedup << "x (memory limited)\n";
            std::cout << "   Actual achievable tokens/s: " << baseline.tokens_per_second * memory_limited_speedup << " tokens/s\n";
        } else {
            std::cout << "✅ Memory bandwidth sufficient for this improvement\n";
        }
        
        std::cout << "\nHARDWARE UTILIZATION:\n";
        std::cout << std::string(60, '-') << "\n";
        std::cout << "Baseline Utilization:        4.97% of peak TFLOPS\n";
        std::cout << "Enhanced Utilization:        " << enhanced.hardware_utilization << "% of peak TFLOPS\n\n";
        
        // Real-world projections
        analyzeRealWorldScenarios(enhanced);
        
        // Optimization recommendations
        printOptimizationRecommendations(enhanced);
    }
    
    void analyzeRealWorldScenarios(const EnhancedMetrics& enhanced) {
        std::cout << "REAL-WORLD PERFORMANCE SCENARIOS:\n";
        std::cout << std::string(60, '-') << "\n";
        
        struct Scenario {
            std::string name;
            int batch_size;
            double efficiency;
            std::string description;
        };
        
        std::vector<Scenario> scenarios = {
            {"Single User Interactive", 1, 1.0, "Real-time chat/completion"},
            {"Small Batch Serving", 8, 0.95, "API serving with batching"},
            {"Large Batch Inference", 32, 0.85, "Batch processing workloads"},
            {"Server Max Throughput", 128, 0.70, "Maximum concurrent users"}
        };
        
        std::cout << "Scenario                    Batch  Efficiency  Tokens/s   Users@25tok/s\n";
        std::cout << std::string(70, '-') << "\n";
        
        for (const auto& scenario : scenarios) {
            double effective_tokens_per_second = enhanced.new_tokens_per_second * 
                                                scenario.batch_size * scenario.efficiency;
            int max_users = static_cast<int>(effective_tokens_per_second / 25.0);
            
            std::cout << std::setw(25) << scenario.name 
                      << std::setw(8) << scenario.batch_size
                      << std::setw(12) << scenario.efficiency
                      << std::setw(10) << effective_tokens_per_second
                      << std::setw(12) << max_users << "\n";
        }
        std::cout << "\n";
    }
    
    void printOptimizationRecommendations(const EnhancedMetrics& enhanced) {
        std::cout << "OPTIMIZATION RECOMMENDATIONS FOR 20x MatMul SCENARIO:\n";
        std::cout << std::string(60, '-') << "\n";
        
        if (enhanced.primary_bottleneck == "Activation Functions") {
            std::cout << "🎯 PRIMARY TARGET: Activation Function Optimization\n";
            std::cout << "   • Custom SFPU kernels for GELU/SiLU functions\n";
            std::cout << "   • Fused activation operations\n";
            std::cout << "   • Vectorized activation implementations\n";
            std::cout << "   • Potential speedup: 3-5x on activation cycles\n\n";
            
            // Calculate potential further improvement
            double activation_speedup = 4.0; // 4x improvement in activations
            uint64_t improved_activation_cycles = baseline.activation_cycles / activation_speedup;
            uint64_t new_total_with_activation_opt = enhanced.new_matmul_cycles + 
                                                   baseline.elementwise_cycles + 
                                                   improved_activation_cycles + 
                                                   baseline.data_movement_cycles;
            double further_speedup = static_cast<double>(enhanced.new_total_cycles) / new_total_with_activation_opt;
            
            std::cout << "   With 4x activation optimization:\n";
            std::cout << "   • Additional speedup: " << further_speedup << "x\n";
            std::cout << "   • Total speedup: " << enhanced.speedup_factor * further_speedup << "x over baseline\n";
            std::cout << "   • Final tokens/s: " << baseline.tokens_per_second * enhanced.speedup_factor * further_speedup << " tokens/s\n\n";
            
        } else if (enhanced.primary_bottleneck == "Element-wise Operations") {
            std::cout << "🎯 PRIMARY TARGET: Element-wise Operation Optimization\n";
            std::cout << "   • Fused element-wise kernels\n";
            std::cout << "   • Vectorized residual connections\n";
            std::cout << "   • In-place operations where possible\n";
            std::cout << "   • Potential speedup: 2-3x on element-wise cycles\n\n";
        } else {
            std::cout << "🎯 PRIMARY TARGET: Data Movement Optimization\n";
            std::cout << "   • NoC traffic optimization\n";
            std::cout << "   • Better L1 cache utilization\n";
            std::cout << "   • Reduced inter-core communication\n";
            std::cout << "   • Potential speedup: 2-4x on data movement\n\n";
        }
        
        std::cout << "SECONDARY OPTIMIZATIONS:\n";
        std::cout << "• Mixed precision (FP8): Additional 1.5-2x improvement\n";
        std::cout << "• Layer pipelining: 1.3-1.8x improvement\n";
        std::cout << "• KV-cache optimization: Critical for multi-token generation\n";
        std::cout << "• Speculative decoding: 2-3x improvement for generation tasks\n\n";
        
        std::cout << "MEMORY CONSIDERATIONS:\n";
        if (enhanced.memory_bound) {
            std::cout << "• ⚠️  Memory bandwidth becomes limiting factor\n";
            std::cout << "• Consider memory compression techniques\n";
            std::cout << "• Implement weight streaming strategies\n";
            std::cout << "• Use lower precision weights (FP8/INT8)\n";
        } else {
            std::cout << "• ✅ Memory bandwidth sufficient\n";
            std::cout << "• Can support higher batch sizes\n";
            std::cout << "• Room for additional optimizations\n";
        }
        
        std::cout << "\nRECOMMENDED IMPLEMENTATION PRIORITY:\n";
        std::cout << "1. Optimize " << enhanced.primary_bottleneck << " (highest impact)\n";
        std::cout << "2. Implement mixed precision (good ROI)\n";
        std::cout << "3. Add layer pipelining (complexity vs benefit)\n";
        std::cout << "4. Memory optimization (if bandwidth constrained)\n";
    }
    
    void analyzeMultipleScenarios() {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "                    MULTIPLE SPEEDUP SCENARIOS\n";
        std::cout << std::string(80, '=') << "\n\n";
        
        std::vector<PerformanceScenario> scenarios = {
            {"Current Baseline", 1.0, 1.0, 1.0, "Current implementation"},
            {"5x MatMul Speedup", 5.0, 1.0, 1.0, "Moderate MatMul improvement"},
            {"10x MatMul Speedup", 10.0, 1.0, 1.0, "Significant MatMul improvement"},
            {"20x MatMul Speedup", 20.0, 1.0, 1.0, "Revolutionary MatMul improvement"},
            {"20x MatMul + 4x Vector", 20.0, 4.0, 1.0, "MatMul + vector optimization"},
            {"20x MatMul + Memory Opt", 20.0, 1.0, 2.0, "MatMul + memory optimization"}
        };
        
        std::cout << "Scenario                     Speedup   Tokens/s   Time/Token   Bottleneck\n";
        std::cout << std::string(75, '-') << "\n";
        
        for (const auto& scenario : scenarios) {
            auto metrics = calculateScenarioMetrics(scenario);
            std::cout << std::setw(25) << scenario.name
                      << std::setw(10) << metrics.speedup_factor << "x"
                      << std::setw(10) << metrics.new_tokens_per_second
                      << std::setw(12) << metrics.new_time_per_token_ms << "ms"
                      << std::setw(15) << metrics.primary_bottleneck << "\n";
        }
        
        std::cout << "\nKey Insights:\n";
        std::cout << "• Diminishing returns beyond 10x MatMul speedup\n";
        std::cout << "• Vector optimization becomes critical at high MatMul speedups\n";
        std::cout << "• Memory bandwidth limits ultimate performance\n";
        std::cout << "• Balanced optimization yields best results\n";
    }
    
private:
    EnhancedMetrics calculateScenarioMetrics(const PerformanceScenario& scenario) {
        EnhancedMetrics metrics;
        
        // Calculate new cycle counts
        metrics.new_matmul_cycles = baseline.matmul_cycles / scenario.matmul_speedup;
        uint64_t new_vector_cycles = (baseline.elementwise_cycles + baseline.activation_cycles) / scenario.vector_speedup;
        uint64_t new_memory_cycles = baseline.data_movement_cycles / scenario.memory_speedup;
        
        metrics.new_total_cycles = metrics.new_matmul_cycles + new_vector_cycles + new_memory_cycles;
        
        // Calculate performance improvements
        metrics.speedup_factor = static_cast<double>(baseline.total_cycles) / metrics.new_total_cycles;
        metrics.new_time_per_token_ms = baseline.time_per_token_ms / metrics.speedup_factor;
        metrics.new_tokens_per_second = 1000.0 / metrics.new_time_per_token_ms;
        
        // Determine bottleneck
        uint64_t max_cycles = std::max({metrics.new_matmul_cycles, new_vector_cycles, new_memory_cycles});
        if (max_cycles == metrics.new_matmul_cycles) {
            metrics.primary_bottleneck = "MatMul";
        } else if (max_cycles == new_vector_cycles) {
            metrics.primary_bottleneck = "Vector";
        } else {
            metrics.primary_bottleneck = "Memory";
        }
        
        return metrics;
    }
};

// Function to run the enhanced analysis
void runEnhancedMatMulAnalysis() {
    std::cout << "Running Enhanced MatMul Performance Analysis...\n";
    
    EnhancedMatMulAnalyzer analyzer;
    analyzer.printComprehensiveAnalysis();
    analyzer.analyzeMultipleScenarios();
}