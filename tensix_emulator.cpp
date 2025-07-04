#include <iostream>
#include <vector>
#include <memory>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <random>
#include <chrono>
#include <functional>
#include <cstring>
#include <iomanip>

// Forward declarations
class TensixCore;
class NetworkOnChip;
class MemorySystem;
class ComputeEngine;

// Data formats supported by Tensix
enum class DataFormat {
    FP32,
    FP16,
    BFP16,
    FP8,
    INT8,
    INT16,
    INT32
};

// Tensor descriptor
struct Tensor {
    std::vector<uint32_t> shape;
    DataFormat format;
    std::vector<uint8_t> data;
    uint64_t device_address;
    
    Tensor(std::vector<uint32_t> s, DataFormat f) : shape(s), format(f), device_address(0) {
        size_t element_size = getElementSize(f);
        size_t total_elements = 1;
        for (auto dim : shape) total_elements *= dim;
        data.resize(total_elements * element_size);
    }
    
    size_t getElementSize(DataFormat f) {
        switch (f) {
            case DataFormat::FP32: return 4;
            case DataFormat::FP16: return 2;
            case DataFormat::BFP16: return 2;
            case DataFormat::FP8: return 1;
            case DataFormat::INT8: return 1;
            case DataFormat::INT16: return 2;
            case DataFormat::INT32: return 4;
            default: return 4;
        }
    }
    
    size_t getTotalSize() const {
        size_t total_elements = 1;
        for (auto dim : shape) total_elements *= dim;
        return total_elements * getElementSize(format);
    }
};

// Memory hierarchy levels
enum class MemoryLevel {
    L1_CACHE,
    DRAM,
    HOST
};

// Memory system modeling SRAM and DRAM
class MemorySystem {
private:
    struct MemoryBank {
        uint64_t base_address;
        uint64_t size;
        std::vector<uint8_t> data;
        MemoryLevel level;
        uint32_t access_latency;
        uint32_t bandwidth_gbps;
        
        MemoryBank(uint64_t addr, uint64_t sz, MemoryLevel lvl, uint32_t lat, uint32_t bw) 
            : base_address(addr), size(sz), level(lvl), access_latency(lat), bandwidth_gbps(bw) {
            data.resize(size, 0);
        }
    };
    
    std::vector<MemoryBank> memory_banks;
    std::mutex memory_mutex;
    std::atomic<uint64_t> memory_allocator_ptr{0x1000000}; // Start at 16MB
    
public:
    MemorySystem() {
        // L1 Cache (SRAM) - 108MB for Wormhole
        memory_banks.emplace_back(0x0, 108 * 1024 * 1024, MemoryLevel::L1_CACHE, 1, 1000);
        
        // DRAM - 12GB for single Wormhole n150
        memory_banks.emplace_back(0x10000000, 12ULL * 1024 * 1024 * 1024, MemoryLevel::DRAM, 100, 288);
    }
    
    uint64_t allocateMemory(size_t size, MemoryLevel preferred_level = MemoryLevel::DRAM) {
        std::lock_guard<std::mutex> lock(memory_mutex);
        
        for (auto& bank : memory_banks) {
            if (bank.level == preferred_level && 
                memory_allocator_ptr + size < bank.base_address + bank.size) {
                uint64_t allocated_addr = memory_allocator_ptr;
                memory_allocator_ptr += size;
                return allocated_addr;
            }
        }
        
        throw std::runtime_error("Out of memory");
    }
    
    void writeMemory(uint64_t address, const void* data, size_t size) {
        std::lock_guard<std::mutex> lock(memory_mutex);
        
        for (auto& bank : memory_banks) {
            if (address >= bank.base_address && 
                address + size <= bank.base_address + bank.size) {
                std::memcpy(&bank.data[address - bank.base_address], data, size);
                
                // Simulate memory access latency
                std::this_thread::sleep_for(
                    std::chrono::nanoseconds(bank.access_latency * 10));
                return;
            }
        }
        
        throw std::runtime_error("Invalid memory address");
    }
    
    void readMemory(uint64_t address, void* data, size_t size) {
        std::lock_guard<std::mutex> lock(memory_mutex);
        
        for (auto& bank : memory_banks) {
            if (address >= bank.base_address && 
                address + size <= bank.base_address + bank.size) {
                std::memcpy(data, &bank.data[address - bank.base_address], size);
                
                // Simulate memory access latency
                std::this_thread::sleep_for(
                    std::chrono::nanoseconds(bank.access_latency * 10));
                return;
            }
        }
        
        throw std::runtime_error("Invalid memory address");
    }
    
    void dumpMemoryStats() {
        std::cout << "Memory System Statistics:\n";
        for (size_t i = 0; i < memory_banks.size(); ++i) {
            auto& bank = memory_banks[i];
            std::cout << "  Bank " << i << " (" 
                      << (bank.level == MemoryLevel::L1_CACHE ? "L1" : "DRAM") 
                      << "): " << bank.size / (1024*1024) << "MB"
                      << ", Latency: " << bank.access_latency << "ns"
                      << ", Bandwidth: " << bank.bandwidth_gbps << "GB/s\n";
        }
    }
};

// Compute Engine modeling Matrix, Vector, and RISC-V units
class ComputeEngine {
private:
    uint32_t core_id;
    std::atomic<bool> busy{false};
    std::mutex compute_mutex;
    
public:
    ComputeEngine(uint32_t id) : core_id(id) {}
    virtual ~ComputeEngine() = default;
    
    // Matrix multiplication operation
    virtual void matmul(const Tensor& a, const Tensor& b, Tensor& c) {
        std::lock_guard<std::mutex> lock(compute_mutex);
        busy = true;
        
        if (a.shape.size() != 2 || b.shape.size() != 2) {
            throw std::runtime_error("Matrix multiplication requires 2D tensors");
        }
        
        uint32_t m = a.shape[0], k = a.shape[1], n = b.shape[1];
        if (a.shape[1] != b.shape[0]) {
            throw std::runtime_error("Matrix dimension mismatch");
        }
        
        // Simulate compute time based on operations
        auto compute_time = std::chrono::microseconds(m * n * k / 1000);
        std::this_thread::sleep_for(compute_time);
        
        std::cout << "Core " << core_id << ": MatMul [" << m << "x" << k 
                  << "] x [" << k << "x" << n << "] -> [" << m << "x" << n << "]\n";
        
        busy = false;
    }
    
    // Element-wise operations
    virtual void elementwise_add(const Tensor& a, const Tensor& b, Tensor& c) {
        std::lock_guard<std::mutex> lock(compute_mutex);
        busy = true;
        
        size_t elements = a.getTotalSize() / a.getElementSize(a.format);
        auto compute_time = std::chrono::microseconds(elements / 10000);
        std::this_thread::sleep_for(compute_time);
        
        std::cout << "Core " << core_id << ": ElementWise Add (" 
                  << elements << " elements)\n";
        
        busy = false;
    }
    
    // Activation functions
    virtual void relu(const Tensor& input, Tensor& output) {
        std::lock_guard<std::mutex> lock(compute_mutex);
        busy = true;
        
        size_t elements = input.getTotalSize() / input.getElementSize(input.format);
        auto compute_time = std::chrono::microseconds(elements / 20000);
        std::this_thread::sleep_for(compute_time);
        
        std::cout << "Core " << core_id << ": ReLU (" << elements << " elements)\n";
        
        busy = false;
    }
    
    virtual void gelu(const Tensor& input, Tensor& output) {
        std::lock_guard<std::mutex> lock(compute_mutex);
        busy = true;
        
        size_t elements = input.getTotalSize() / input.getElementSize(input.format);
        auto compute_time = std::chrono::microseconds(elements / 15000);
        std::this_thread::sleep_for(compute_time);
        
        std::cout << "Core " << core_id << ": GELU (" << elements << " elements)\n";
        
        busy = false;
    }
    
    bool isBusy() const { return busy; }
};

// Network-on-Chip for inter-core communication
class NetworkOnChip {
private:
    struct Message {
        uint32_t src_core;
        uint32_t dst_core;
        uint64_t src_addr;
        uint64_t dst_addr;
        size_t size;
        std::vector<uint8_t> data;
    };
    
    std::queue<Message> message_queue;
    std::mutex noc_mutex;
    std::atomic<bool> running{true};
    std::thread noc_thread;
    std::atomic<uint64_t> bytes_transferred{0};
    
public:
    NetworkOnChip() {
        noc_thread = std::thread(&NetworkOnChip::processMessages, this);
    }
    
    ~NetworkOnChip() {
        running = false;
        if (noc_thread.joinable()) {
            noc_thread.join();
        }
    }
    
    void sendData(uint32_t src_core, uint32_t dst_core, 
                  uint64_t src_addr, uint64_t dst_addr, 
                  const void* data, size_t size) {
        std::lock_guard<std::mutex> lock(noc_mutex);
        
        Message msg;
        msg.src_core = src_core;
        msg.dst_core = dst_core;
        msg.src_addr = src_addr;
        msg.dst_addr = dst_addr;
        msg.size = size;
        msg.data.resize(size);
        std::memcpy(msg.data.data(), data, size);
        
        message_queue.push(std::move(msg));
        bytes_transferred += size;
    }
    
    void processMessages() {
        while (running) {
            std::unique_lock<std::mutex> lock(noc_mutex);
            
            if (!message_queue.empty()) {
                Message msg = std::move(message_queue.front());
                message_queue.pop();
                lock.unlock();
                
                // Simulate NoC latency based on distance and size
                auto latency = std::chrono::microseconds(
                    std::abs((int)msg.dst_core - (int)msg.src_core) * 10 + msg.size / 1000);
                std::this_thread::sleep_for(latency);
                
                std::cout << "NoC: Core " << msg.src_core << " -> Core " 
                          << msg.dst_core << " (" << msg.size << " bytes)\n";
            } else {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }
    
    uint64_t getBytesTransferred() const { return bytes_transferred; }
};

// Individual Tensix Core
class TensixCore {
private:
    uint32_t core_id;
    std::unique_ptr<ComputeEngine> compute_engine;
    std::shared_ptr<MemorySystem> memory_system;
    std::shared_ptr<NetworkOnChip> noc;
    
public:
    TensixCore(uint32_t id, std::shared_ptr<MemorySystem> mem, 
               std::shared_ptr<NetworkOnChip> network) 
        : core_id(id), memory_system(mem), noc(network) {
        compute_engine = std::make_unique<ComputeEngine>(id);
    }
    
    uint32_t getId() const { return core_id; }
    
    void executeMatMul(const Tensor& a, const Tensor& b, Tensor& c) {
        compute_engine->matmul(a, b, c);
    }
    
    void executeElementwiseAdd(const Tensor& a, const Tensor& b, Tensor& c) {
        compute_engine->elementwise_add(a, b, c);
    }
    
    void executeActivation(const Tensor& input, Tensor& output, const std::string& activation) {
        if (activation == "relu") {
            compute_engine->relu(input, output);
        } else if (activation == "gelu") {
            compute_engine->gelu(input, output);
        }
    }
    
    bool isBusy() const { return compute_engine->isBusy(); }
    
    void allocateTensor(Tensor& tensor, MemoryLevel level = MemoryLevel::DRAM) {
        tensor.device_address = memory_system->allocateMemory(tensor.getTotalSize(), level);
    }
    
    void writeTensor(const Tensor& tensor) {
        memory_system->writeMemory(tensor.device_address, tensor.data.data(), tensor.getTotalSize());
    }
    
    void readTensor(Tensor& tensor) {
        memory_system->readMemory(tensor.device_address, tensor.data.data(), tensor.getTotalSize());
    }
    
    void sendTensorToCore(uint32_t dst_core, const Tensor& tensor, uint64_t dst_addr) {
        noc->sendData(core_id, dst_core, tensor.device_address, dst_addr, 
                      tensor.data.data(), tensor.getTotalSize());
    }
};

// Main Tensix Device Emulator
class TensixDevice {
private:
    std::vector<std::unique_ptr<TensixCore>> cores;
    std::shared_ptr<MemorySystem> memory_system;
    std::shared_ptr<NetworkOnChip> noc;
    uint32_t num_cores;
    
public:
    TensixDevice(uint32_t core_count = 72) : num_cores(core_count) {
        memory_system = std::make_shared<MemorySystem>();
        noc = std::make_shared<NetworkOnChip>();
        
        // Initialize cores
        for (uint32_t i = 0; i < num_cores; ++i) {
            cores.push_back(std::make_unique<TensixCore>(i, memory_system, noc));
        }
        
        std::cout << "Initialized Tensix Device with " << num_cores << " cores\n";
        memory_system->dumpMemoryStats();
    }
    
    TensixCore& getCore(uint32_t core_id) {
        if (core_id >= num_cores) {
            throw std::runtime_error("Invalid core ID");
        }
        return *cores[core_id];
    }
    
    uint32_t getNumCores() const { return num_cores; }
    
    std::shared_ptr<MemorySystem> getMemorySystem() { return memory_system; }
    std::shared_ptr<NetworkOnChip> getNoC() { return noc; }
    
    // Find available core for computation
    uint32_t findAvailableCore() {
        for (uint32_t i = 0; i < num_cores; ++i) {
            if (!cores[i]->isBusy()) {
                return i;
            }
        }
        return 0; // Return first core if all busy
    }
    
    void waitForAllCores() {
        bool all_idle = false;
        while (!all_idle) {
            all_idle = true;
            for (const auto& core : cores) {
                if (core->isBusy()) {
                    all_idle = false;
                    break;
                }
            }
            if (!all_idle) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }
    
    void dumpStats() {
        std::cout << "\n=== Device Statistics ===\n";
        std::cout << "Active cores: " << num_cores << "\n";
        std::cout << "NoC bytes transferred: " << noc->getBytesTransferred() << "\n";
        memory_system->dumpMemoryStats();
    }
};