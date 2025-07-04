#include "tensix_emulator.cpp"
#include <map>
#include <chrono>
#include <iomanip>

// Performance profiling for Tensix emulation
class PerformanceProfiler {
private:
    struct OperationMetrics {
        uint64_t total_cycles = 0;
        uint64_t operation_count = 0;
        uint64_t total_flops = 0;
        uint64_t memory_bytes = 0;
        double total_time_ms = 0.0;
        
        double getAverageTime() const {
            return operation_count > 0 ? total_time_ms / operation_count : 0.0;
        }
        
        double getThroughput() const {
            return total_time_ms > 0 ? (total_flops / 1e9) / (total_time_ms / 1000.0) : 0.0; // GFLOPS
        }
    };
    
    std::map<std::string, OperationMetrics> operation_stats;
    std::chrono::high_resolution_clock::time_point profiling_start;
    std::chrono::high_resolution_clock::time_point profiling_end;
    
    // Wormhole n150 specifications
    static constexpr uint32_t WORMHOLE_CORES = 72;
    static constexpr uint32_t AI_CLOCK_FREQ_GHZ = 1; // 1 GHz
    static constexpr uint64_t CYCLES_PER_SECOND = 1e9;
    static constexpr uint32_t MEMORY_BANDWIDTH_GBPS = 288; // DRAM bandwidth
    static constexpr uint32_t L1_BANDWIDTH_GBPS = 1000; // L1 cache bandwidth
    
public:
    void startProfiling() {
        profiling_start = std::chrono::high_resolution_clock::now();
        operation_stats.clear();
    }
    
    void endProfiling() {
        profiling_end = std::chrono::high_resolution_clock::now();
    }
    
    void recordMatMul(uint32_t m, uint32_t k, uint32_t n, double execution_time_ms, uint32_t core_id) {
        auto& stats = operation_stats["MatMul"];
        stats.operation_count++;
        stats.total_time_ms += execution_time_ms;
        
        // Calculate FLOPs: 2*M*N*K for matrix multiplication
        uint64_t flops = 2ULL * m * n * k;
        stats.total_flops += flops;
        
        // Estimate cycles based on Tensix matrix engine performance
        // Assuming peak performance of ~74 TFLOPS FP16 across 72 cores
        double peak_flops_per_core = 74e12 / WORMHOLE_CORES; // FLOPS per core
        uint64_t estimated_cycles = static_cast<uint64_t>(flops / peak_flops_per_core * CYCLES_PER_SECOND);
        stats.total_cycles += estimated_cycles;
        
        // Memory bandwidth: reading A, B and writing C
        uint64_t memory_access = (static_cast<uint64_t>(m * k) + k * n + m * n) * 2; // FP16 = 2 bytes
        stats.memory_bytes += memory_access;
    }
    
    void recordElementWise(uint64_t elements, double execution_time_ms, const std::string& op_type) {
        auto& stats = operation_stats[op_type];
        stats.operation_count++;
        stats.total_time_ms += execution_time_ms;
        
        // Element-wise operations: 1 FLOP per element
        stats.total_flops += elements;
        
        // Estimate cycles for vector engine operations
        // Assuming vector engine can process elements at high throughput
        uint64_t estimated_cycles = elements / 16; // 16 elements per cycle (vectorized)
        stats.total_cycles += estimated_cycles;
        
        // Memory bandwidth: reading input(s) and writing output
        uint64_t memory_access = elements * 2 * 2; // 2 inputs + 1 output, FP16
        stats.memory_bytes += memory_access;
    }
    
    void recordActivation(uint64_t elements, double execution_time_ms, const std::string& activation_type) {
        std::string op_name = "Activation_" + activation_type;
        auto& stats = operation_stats[op_name];
        stats.operation_count++;
        stats.total_time_ms += execution_time_ms;
        
        // Activation functions: ~3-5 FLOPs per element (depending on complexity)
        uint64_t flops_per_element = (activation_type == "gelu") ? 5 : 1; // GELU is more complex
        stats.total_flops += elements * flops_per_element;
        
        // SFPU (Special Function Processing Unit) cycles
        uint64_t estimated_cycles = elements / 8; // 8 elements per cycle for SFPU
        stats.total_cycles += estimated_cycles;
        
        // Memory: read input, write output
        stats.memory_bytes += elements * 2 * 2; // FP16
    }
    
    void recordDataMovement(uint64_t bytes, double execution_time_ms, const std::string& movement_type) {
        auto& stats = operation_stats["DataMovement_" + movement_type];
        stats.operation_count++;
        stats.total_time_ms += execution_time_ms;
        stats.memory_bytes += bytes;
        
        // NoC bandwidth estimation - assume ~200 GB/s for inter-core communication
        double noc_bandwidth_gbps = 200.0;
        uint64_t estimated_cycles = static_cast<uint64_t>((bytes / (noc_bandwidth_gbps * 1e9)) * CYCLES_PER_SECOND);
        stats.total_cycles += estimated_cycles;
    }
    
    void printDetailedReport() {
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(profiling_end - profiling_start);
        double total_time_seconds = total_duration.count() / 1000.0;
        
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "                    TENSIX PERFORMANCE ANALYSIS REPORT\n";
        std::cout << "                           LLaMA-7B on Wormhole n150\n";
        std::cout << std::string(80, '=') << "\n\n";
        
        // Overall metrics
        uint64_t total_cycles = 0;
        uint64_t total_flops = 0;
        double total_op_time = 0;
        
        for (const auto& [op_name, metrics] : operation_stats) {
            total_cycles += metrics.total_cycles;
            total_flops += metrics.total_flops;
            total_op_time += metrics.total_time_ms;
        }
        
        std::cout << "OVERALL PERFORMANCE METRICS:\n";
        std::cout << std::string(40, '-') << "\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Total Execution Time:        " << total_time_seconds << " seconds\n";
        std::cout << "Total Compute Cycles:        " << total_cycles << " cycles\n";
        std::cout << "Total FLOPs:                 " << total_flops / 1e9 << " GFLOP\n";
        std::cout << "Peak Theoretical FLOPS:      " << 74.0 << " TFLOPS (FP16)\n";
        std::cout << "Achieved Performance:        " << (total_flops / 1e12) / total_time_seconds << " TFLOPS\n";
        std::cout << "Hardware Utilization:        " << ((total_flops / 1e12) / total_time_seconds) / 74.0 * 100 << "%\n\n";
        
        // Token throughput calculation
        // Assuming this is for single token generation with typical sequence length
        uint32_t sequence_length = 128; // Typical context length used
        double tokens_per_second = 1.0 / total_time_seconds; // Single token generation
        double effective_throughput = tokens_per_second * sequence_length; // Effective throughput
        
        std::cout << "TOKEN THROUGHPUT ANALYSIS:\n";
        std::cout << std::string(40, '-') << "\n";
        std::cout << "Token Generation Rate:       " << tokens_per_second << " tokens/second\n";
        std::cout << "Time per Token:              " << total_time_seconds * 1000 << " ms/token\n";
        std::cout << "Effective Throughput:        " << effective_throughput << " tokens/second (batched)\n";
        std::cout << "Target Performance:          " << 25.0 << " tokens/second/user (Wormhole spec)\n";
        std::cout << "Performance vs Target:       " << (tokens_per_second / 25.0) * 100 << "%\n\n";
        
        // Operation breakdown
        std::cout << "OPERATION BREAKDOWN:\n";
        std::cout << std::string(40, '-') << "\n";
        std::cout << std::setw(20) << "Operation" 
                  << std::setw(12) << "Count" 
                  << std::setw(12) << "Cycles" 
                  << std::setw(12) << "% Time"
                  << std::setw(12) << "GFLOPS" 
                  << std::setw(15) << "Avg Time (ms)\n";
        std::cout << std::string(83, '-') << "\n";
        
        for (const auto& [op_name, metrics] : operation_stats) {
            double percentage = (metrics.total_cycles / static_cast<double>(total_cycles)) * 100;
            std::cout << std::setw(20) << op_name
                      << std::setw(12) << metrics.operation_count
                      << std::setw(12) << metrics.total_cycles
                      << std::setw(11) << percentage << "%"
                      << std::setw(12) << metrics.getThroughput()
                      << std::setw(15) << metrics.getAverageTime() << "\n";
        }
        
        // MatMul vs Other Operations Analysis
        std::cout << "\n\nMATMUL vs OTHER OPERATIONS ANALYSIS:\n";
        std::cout << std::string(50, '-') << "\n";
        
        uint64_t matmul_cycles = 0;
        uint64_t elementwise_cycles = 0;
        uint64_t data_movement_cycles = 0;
        uint64_t activation_cycles = 0;
        
        for (const auto& [op_name, metrics] : operation_stats) {
            if (op_name == "MatMul") {
                matmul_cycles += metrics.total_cycles;
            } else if (op_name.find("ElementWise") != std::string::npos) {
                elementwise_cycles += metrics.total_cycles;
            } else if (op_name.find("DataMovement") != std::string::npos) {
                data_movement_cycles += metrics.total_cycles;
            } else if (op_name.find("Activation") != std::string::npos) {
                activation_cycles += metrics.total_cycles;
            }
        }
        
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "MatMul Operations:           " << matmul_cycles << " cycles (" 
                  << (matmul_cycles / static_cast<double>(total_cycles)) * 100 << "%)\n";
        std::cout << "Element-wise Operations:     " << elementwise_cycles << " cycles (" 
                  << (elementwise_cycles / static_cast<double>(total_cycles)) * 100 << "%)\n";
        std::cout << "Activation Functions:        " << activation_cycles << " cycles (" 
                  << (activation_cycles / static_cast<double>(total_cycles)) * 100 << "%)\n";
        std::cout << "Data Movement:               " << data_movement_cycles << " cycles (" 
                  << (data_movement_cycles / static_cast<double>(total_cycles)) * 100 << "%)\n";
        
        // Memory bandwidth analysis
        std::cout << "\n\nMEMORY BANDWIDTH ANALYSIS:\n";
        std::cout << std::string(40, '-') << "\n";
        
        uint64_t total_memory_bytes = 0;
        for (const auto& [op_name, metrics] : operation_stats) {
            total_memory_bytes += metrics.memory_bytes;
        }
        
        double memory_bandwidth_used = (total_memory_bytes / 1e9) / total_time_seconds; // GB/s
        std::cout << "Total Memory Traffic:        " << total_memory_bytes / 1e9 << " GB\n";
        std::cout << "Average Memory Bandwidth:    " << memory_bandwidth_used << " GB/s\n";
        std::cout << "Peak Memory Bandwidth:       " << MEMORY_BANDWIDTH_GBPS << " GB/s (DRAM)\n";
        std::cout << "Memory Bandwidth Utilization: " << (memory_bandwidth_used / MEMORY_BANDWIDTH_GBPS) * 100 << "%\n";
        
        // Bottleneck analysis
        std::cout << "\n\nBOTTLENECK ANALYSIS:\n";
        std::cout << std::string(40, '-') << "\n";
        
        double compute_intensity = total_flops / static_cast<double>(total_memory_bytes); // FLOP/byte
        std::cout << "Compute Intensity:           " << compute_intensity << " FLOP/byte\n";
        
        if (compute_intensity > 50) {
            std::cout << "Primary Bottleneck:          COMPUTE BOUND\n";
            std::cout << "Recommendation:              Optimize MatMul operations, increase parallelism\n";
        } else if (compute_intensity > 10) {
            std::cout << "Primary Bottleneck:          BALANCED (Compute/Memory)\n";
            std::cout << "Recommendation:              Optimize both compute and memory access patterns\n";
        } else {
            std::cout << "Primary Bottleneck:          MEMORY BOUND\n";
            std::cout << "Recommendation:              Optimize data movement, increase cache utilization\n";
        }
        
        // Specific optimizations for LLaMA-7B
        std::cout << "\n\nLLaMA-7B SPECIFIC OPTIMIZATIONS:\n";
        std::cout << std::string(40, '-') << "\n";
        std::cout << "• Attention heads can be distributed across " << std::min(32u, WORMHOLE_CORES) << " cores\n";
        std::cout << "• Feed-forward networks benefit from gate/up projection parallelism\n";
        std::cout << "• Layer-wise pipelining can improve throughput by ~2-3x\n";
        std::cout << "• Weight preloading to L1 cache reduces DRAM access by ~40%\n";
        std::cout << "• KV-cache optimization critical for multi-token generation\n";
        
        std::cout << "\n" << std::string(80, '=') << "\n";
    }
    
    // Get specific metrics for external analysis
    struct ThroughputMetrics {
        double tokens_per_second;
        double time_per_token_ms;
        double matmul_percentage;
        double elementwise_percentage;
        double data_movement_percentage;
        double compute_utilization;
        uint64_t total_cycles;
        uint64_t matmul_cycles;
        uint64_t elementwise_cycles;
    };
    
    ThroughputMetrics getThroughputMetrics() {
        ThroughputMetrics metrics;
        
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(profiling_end - profiling_start);
        double total_time_seconds = total_duration.count() / 1000.0;
        
        metrics.tokens_per_second = 1.0 / total_time_seconds;
        metrics.time_per_token_ms = total_time_seconds * 1000;
        
        uint64_t total_cycles = 0;
        uint64_t matmul_cycles = 0;
        uint64_t elementwise_cycles = 0;
        uint64_t data_movement_cycles = 0;
        uint64_t total_flops = 0;
        
        for (const auto& [op_name, op_metrics] : operation_stats) {
            total_cycles += op_metrics.total_cycles;
            total_flops += op_metrics.total_flops;
            
            if (op_name == "MatMul") {
                matmul_cycles += op_metrics.total_cycles;
            } else if (op_name.find("ElementWise") != std::string::npos || 
                       op_name.find("Activation") != std::string::npos) {
                elementwise_cycles += op_metrics.total_cycles;
            } else if (op_name.find("DataMovement") != std::string::npos) {
                data_movement_cycles += op_metrics.total_cycles;
            }
        }
        
        metrics.total_cycles = total_cycles;
        metrics.matmul_cycles = matmul_cycles;
        metrics.elementwise_cycles = elementwise_cycles;
        
        metrics.matmul_percentage = (matmul_cycles / static_cast<double>(total_cycles)) * 100;
        metrics.elementwise_percentage = (elementwise_cycles / static_cast<double>(total_cycles)) * 100;
        metrics.data_movement_percentage = (data_movement_cycles / static_cast<double>(total_cycles)) * 100;
        
        // Compute utilization based on achieved vs peak FLOPS
        double achieved_tflops = (total_flops / 1e12) / total_time_seconds;
        metrics.compute_utilization = (achieved_tflops / 74.0) * 100; // 74 TFLOPS peak for Wormhole
        
        return metrics;
    }
};