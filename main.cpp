#include "llm_model.cpp"
#include <iostream>
#include <chrono>
#include <iomanip>

void printBanner() {
    std::cout << "========================================================\n";
    std::cout << "    Tenstorrent Tensix Architecture Emulator v1.0     \n";
    std::cout << "         C++ LLM Model Execution Demo                  \n";
    std::cout << "========================================================\n\n";
}

void printSystemSpecs(const ModelConfig& config, const TensixDevice& device) {
    std::cout << "=== System Configuration ===\n";
    std::cout << "Tensix Cores: " << device.getNumCores() << "\n";
    std::cout << "Memory Configuration:\n";
    std::cout << "  - L1 Cache (SRAM): 108 MB\n";
    std::cout << "  - DRAM: 12 GB\n";
    std::cout << "  - Memory Bandwidth: 288 GB/s\n\n";
    
    std::cout << "=== Model Configuration ===\n";
    std::cout << "Model Type: Transformer (LLaMA-style)\n";
    std::cout << "Parameters:\n";
    std::cout << "  - Vocabulary Size: " << config.vocab_size << "\n";
    std::cout << "  - Hidden Dimension: " << config.hidden_dim << "\n";
    std::cout << "  - Intermediate Dimension: " << config.intermediate_dim << "\n";
    std::cout << "  - Number of Layers: " << config.num_layers << "\n";
    std::cout << "  - Number of Attention Heads: " << config.num_heads << "\n";
    std::cout << "  - Head Dimension: " << config.head_dim << "\n";
    std::cout << "  - Max Sequence Length: " << config.max_seq_len << "\n";
    std::cout << "  - Weight Format: " << (config.weight_format == DataFormat::FP16 ? "FP16" : "FP32") << "\n";
    std::cout << "  - Activation Format: " << (config.activation_format == DataFormat::FP16 ? "FP16" : "FP32") << "\n\n";
}

void demonstrateBasicOperations(TensixDevice& device) {
    std::cout << "=== Basic Operations Demo ===\n";
    
    // Create test tensors
    Tensor a({1024, 1024}, DataFormat::FP16);
    Tensor b({1024, 1024}, DataFormat::FP16);
    Tensor c({1024, 1024}, DataFormat::FP16);
    
    // Allocate tensors on device
    auto& core0 = device.getCore(0);
    core0.allocateTensor(a);
    core0.allocateTensor(b);
    core0.allocateTensor(c);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Demonstrate matrix multiplication
    std::cout << "Performing 1024x1024 matrix multiplication...\n";
    core0.executeMatMul(a, b, c);
    
    // Demonstrate element-wise operations
    std::cout << "Performing element-wise addition...\n";
    core0.executeElementwiseAdd(a, b, c);
    
    // Demonstrate activation functions
    std::cout << "Applying GELU activation...\n";
    core0.executeActivation(a, c, "gelu");
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Basic operations completed in " << duration.count() << " microseconds\n\n";
}

void demonstrateMultiCoreOperations(TensixDevice& device) {
    std::cout << "=== Multi-Core Operations Demo ===\n";
    
    const uint32_t num_cores = std::min(8u, device.getNumCores());
    std::vector<std::thread> compute_threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Launch parallel computations across multiple cores
    for (uint32_t i = 0; i < num_cores; ++i) {
        compute_threads.emplace_back([&device, i]() {
            auto& core = device.getCore(i);
            
            // Create tensors for this core
            Tensor a({512, 512}, DataFormat::FP16);
            Tensor b({512, 512}, DataFormat::FP16);
            Tensor c({512, 512}, DataFormat::FP16);
            
            core.allocateTensor(a);
            core.allocateTensor(b);
            core.allocateTensor(c);
            
            // Perform computation
            core.executeMatMul(a, b, c);
            core.executeActivation(c, c, "relu");
        });
    }
    
    // Wait for all cores to complete
    for (auto& thread : compute_threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Multi-core operations on " << num_cores << " cores completed in " 
              << duration.count() << " microseconds\n\n";
}

void demonstrateNetworkOnChip(TensixDevice& device) {
    std::cout << "=== Network-on-Chip Demo ===\n";
    
    if (device.getNumCores() < 4) {
        std::cout << "Skipping NoC demo - insufficient cores\n\n";
        return;
    }
    
    // Create test data
    Tensor data({256, 256}, DataFormat::FP16);
    auto& src_core = device.getCore(0);
    auto& dst_core = device.getCore(3);
    
    src_core.allocateTensor(data);
    
    // Simulate data transfer between cores
    std::cout << "Transferring data from Core 0 to Core 3...\n";
    uint64_t dst_addr = 0x20000000; // Some destination address
    src_core.sendTensorToCore(3, data, dst_addr);
    
    // Wait a bit for the transfer to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "NoC data transfer completed\n";
    std::cout << "Total bytes transferred: " << device.getNoC()->getBytesTransferred() << "\n\n";
}

void runSmallModelDemo(TensixDevice& device) {
    std::cout << "=== Small Model Demo ===\n";
    
    // Create a smaller model configuration for demonstration
    ModelConfig small_config;
    small_config.vocab_size = 1000;
    small_config.hidden_dim = 512;
    small_config.intermediate_dim = 1024;
    small_config.num_layers = 4;
    small_config.num_heads = 8;
    small_config.head_dim = 64;
    small_config.max_seq_len = 128;
    
    std::cout << "Creating small LLM model for demonstration...\n";
    std::cout << "Hidden dim: " << small_config.hidden_dim 
              << ", Layers: " << small_config.num_layers << "\n\n";
    
    LlamaModel model(small_config, device);
    
    // Create a simple prompt
    std::vector<uint32_t> prompt = {1, 2, 3, 4, 5}; // Simple token sequence
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Generate some tokens
    model.generate(prompt, 3);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "\nSmall model demo completed in " << duration.count() << " milliseconds\n\n";
}

void runFullModelDemo(TensixDevice& device) {
    std::cout << "=== Full Scale Model Demo ===\n";
    
    // Use default configuration (similar to LLaMA 7B)
    ModelConfig config;
    
    std::cout << "Creating full-scale LLM model...\n";
    std::cout << "This simulates a " << config.num_layers << "-layer transformer model\n";
    std::cout << "Similar to LLaMA 7B architecture\n\n";
    
    auto start_init = std::chrono::high_resolution_clock::now();
    LlamaModel model(config, device);
    auto end_init = std::chrono::high_resolution_clock::now();
    
    auto init_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_init - start_init);
    std::cout << "Model initialization took " << init_duration.count() << " milliseconds\n\n";
    
    // Create a longer prompt for more realistic demonstration
    std::vector<uint32_t> prompt = {1, 15, 234, 567, 89, 123, 456, 789}; // Longer token sequence
    
    auto start_gen = std::chrono::high_resolution_clock::now();
    
    // Generate tokens
    model.generate(prompt, 5);
    
    auto end_gen = std::chrono::high_resolution_clock::now();
    auto gen_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_gen - start_gen);
    
    std::cout << "\nFull model demo completed in " << gen_duration.count() << " milliseconds\n\n";
}

void printPerformanceAnalysis(TensixDevice& device) {
    std::cout << "=== Performance Analysis ===\n";
    
    device.dumpStats();
    
    std::cout << "\nTensix Architecture Benefits:\n";
    std::cout << "  ✓ Parallel execution across " << device.getNumCores() << " cores\n";
    std::cout << "  ✓ Specialized compute engines for matrix operations\n";
    std::cout << "  ✓ High-bandwidth memory access (288 GB/s DRAM)\n";
    std::cout << "  ✓ Efficient Network-on-Chip for inter-core communication\n";
    std::cout << "  ✓ Support for multiple data formats (FP16, BFP16, FP8)\n";
    std::cout << "  ✓ Hardware-optimized activation functions (GELU, ReLU)\n\n";
    
    std::cout << "Model Mapping Optimizations:\n";
    std::cout << "  ✓ Attention heads distributed across cores\n";
    std::cout << "  ✓ Feed-forward projections parallelized\n";
    std::cout << "  ✓ Layer-wise pipelining for throughput\n";
    std::cout << "  ✓ Memory hierarchy optimization (L1 cache for weights)\n";
    std::cout << "  ✓ NoC utilized for gradient/activation passing\n\n";
}

int main() {
    try {
        printBanner();
        
        // Initialize Tensix device with Wormhole n150 configuration (72 cores)
        std::cout << "Initializing Tenstorrent Tensix Device...\n\n";
        TensixDevice device(72);
        
        // Default model configuration
        ModelConfig config;
        printSystemSpecs(config, device);
        
        // Run comprehensive demos
        std::cout << "Starting comprehensive demonstration...\n\n";
        
        // 1. Basic operations demo
        demonstrateBasicOperations(device);
        
        // 2. Multi-core operations demo
        demonstrateMultiCoreOperations(device);
        
        // 3. Network-on-Chip demo
        demonstrateNetworkOnChip(device);
        
        // 4. Small model demo (faster execution)
        runSmallModelDemo(device);
        
        // 5. Ask user if they want to run full model demo
        std::cout << "Do you want to run the full-scale model demo? (y/N): ";
        char response;
        std::cin >> response;
        
        if (response == 'y' || response == 'Y') {
            runFullModelDemo(device);
        } else {
            std::cout << "Skipping full-scale model demo.\n\n";
        }
        
        // 6. Performance analysis
        printPerformanceAnalysis(device);
        
        std::cout << "========================================================\n";
        std::cout << "           Demo completed successfully!                 \n";
        std::cout << "========================================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}