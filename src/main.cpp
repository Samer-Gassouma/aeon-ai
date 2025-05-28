#include "http_server.h"
#include <iostream>
#include <signal.h>
#include <memory>
#include <chrono>
#include <thread>
#include <iomanip>

// Global server instance for signal handling
std::unique_ptr<HttpServer> g_server;

void signalHandler(int signal) {
    std::cout << "\n🛑 Received signal " << signal << ". Shutting down gracefully..." << std::endl;
    
    if (g_server) {
        g_server->stop();
    }
    
    exit(0);
}

void printBanner() {
    std::cout << R"(
 █████╗ ███████╗ ██████╗ ███╗   ██╗     █████╗ ██╗
██╔══██╗██╔════╝██╔═══██╗████╗  ██║    ██╔══██╗██║
███████║█████╗  ██║   ██║██╔██╗ ██║    ███████║██║
██╔══██║██╔══╝  ██║   ██║██║╚██╗██║    ██╔══██║██║
██║  ██║███████╗╚██████╔╝██║ ╚████║    ██║  ██║██║
╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═╝  ╚═══╝    ╚═╝  ╚═╝╚═╝
                                                   
✨ AI-Powered Quiz & Psychology Analysis Platform
🧠 Real AI Generation with DistilGPT-2 + llama.cpp
⚡ High-Performance C++ Implementation
🔮 Advanced Personality Analysis & Quiz Generation

)" << std::endl;
}

void printServerInfo(const std::string& host, int port, const std::string& modelPath) {
    std::cout << "┌─────────────────────────────────────────────────┐" << std::endl;
    std::cout << "│               AEON AI SERVER INFO               │" << std::endl;
    std::cout << "├─────────────────────────────────────────────────┤" << std::endl;
    std::cout << "│ Host: " << std::setw(36) << std::left << host << " │" << std::endl;
    std::cout << "│ Port: " << std::setw(36) << std::left << port << " │" << std::endl;
    std::cout << "│ API Base: http://" << host << ":" << port << std::setw(16) << " │" << std::endl;
    std::cout << "│ Model: DistilGPT-2 (GGUF)" << std::setw(17) << " │" << std::endl;
    std::cout << "│ Model Path: " << std::setw(29) << std::left << modelPath.substr(0, 29) << " │" << std::endl;
    std::cout << "│ Backend: llama.cpp (CPU optimized)" << std::setw(9) << " │" << std::endl;
    std::cout << "└─────────────────────────────────────────────────┘" << std::endl;
}

void printEndpoints() {
    std::cout << "\n🔌 Available API Endpoints:" << std::endl;
    std::cout << "├─ GET  /                       → Status & health check" << std::endl;
    std::cout << "│" << std::endl;
    std::cout << "├─ Quiz Generation:" << std::endl;
    std::cout << "│  ├─ POST /api/quiz/generate   → Generate quiz question" << std::endl;
    std::cout << "│  └─ GET  /api/quiz/categories → List available categories" << std::endl;
    std::cout << "│" << std::endl;
    std::cout << "├─ Psychology Analysis:" << std::endl;
    std::cout << "│  ├─ GET  /api/psychology/traits   → Get personality traits" << std::endl;
    std::cout << "│  ├─ POST /api/psychology/generate → Create psychology questions" << std::endl;
    std::cout << "│  └─ POST /api/psychology/analyze  → Analyze personality profile" << std::endl;
    std::cout << "│" << std::endl;
    std::cout << "└─ System Information:" << std::endl;
    std::cout << "   ├─ GET  /api/stats         → Detailed server statistics" << std::endl;
    std::cout << "   └─ GET  /api/model/info    → AI model information\n" << std::endl;
}

void printAIFeatures() {
    std::cout << "✨ AEON AI Core Features:" << std::endl;
    std::cout << "├─ Real-time AI Generation    → Each response is unique" << std::endl;
    std::cout << "├─ Dynamic Content Creation   → No pre-written content" << std::endl;
    std::cout << "├─ Psychology Assessment      → MBTI & personality analysis" << std::endl;
    std::cout << "├─ Smart Context Processing   → Adaptive question generation" << std::endl;
    std::cout << "├─ Robust Error Recovery      → Graceful fallback systems" << std::endl;
    std::cout << "└─ Comprehensive Analytics    → Performance monitoring\n" << std::endl;
}

void printPerformanceInfo() {
    std::cout << "⚡ Technical Architecture:" << std::endl;
    std::cout << "├─ Engine: llama.cpp          → High-performance inference" << std::endl;
    std::cout << "├─ Optimization: AVX2/AVX512  → CPU vectorization" << std::endl;
    std::cout << "├─ Storage: Quantized Models  → Efficient GGUF format (Q4_K_M)" << std::endl;
    std::cout << "├─ Processing: Multi-threaded → Concurrent request handling" << std::endl;
    std::cout << "├─ Memory: Smart Management   → Optimized token context" << std::endl;
    std::cout << "└─ Networking: Async I/O      → High-throughput HTTP server\n" << std::endl;
}

void printUsageExamples(const std::string& host, int port) {
    std::cout << "📘 API Usage Examples:" << std::endl;
    std::cout << "┌─ Server Status:" << std::endl;
    std::cout << "│  curl http://" << host << ":" << port << "/" << std::endl;
    std::cout << "│" << std::endl;
    std::cout << "├─ Quiz Generation API:" << std::endl;
    std::cout << "│  curl -X POST http://" << host << ":" << port << "/api/quiz/generate \\" << std::endl;
    std::cout << "│    -H \"Content-Type: application/json\" \\" << std::endl;
    std::cout << "│    -d '{" << std::endl;
    std::cout << "│      \"category\": \"Science\"," << std::endl;
    std::cout << "│      \"difficulty\": \"Medium\"," << std::endl;
    std::cout << "│      \"playerName\": \"User\"" << std::endl;
    std::cout << "│    }'" << std::endl;
    std::cout << "│" << std::endl;
    std::cout << "├─ Psychology Analysis API:" << std::endl;
    std::cout << "│  curl -X POST http://" << host << ":" << port << "/api/psychology/analyze \\" << std::endl;
    std::cout << "│    -H \"Content-Type: application/json\" \\" << std::endl;
    std::cout << "│    -d '{" << std::endl;
    std::cout << "│      \"answers\": [" << std::endl;
    std::cout << "│        {\"questionId\": 1, \"selectedOption\": 0, \"trait\": \"E/I\"}," << std::endl;
    std::cout << "│        {\"questionId\": 2, \"selectedOption\": 1, \"trait\": \"S/N\"}" << std::endl;
    std::cout << "│      ]" << std::endl;
    std::cout << "│    }'" << std::endl;
    std::cout << "│" << std::endl;
    std::cout << "└─ System Information:" << std::endl;
    std::cout << "   curl http://" << host << ":" << port << "/api/stats" << std::endl;
    std::cout << std::endl;
}

void printExpectedPerformance() {
    std::cout << "🚀 System Requirements & Performance:" << std::endl;
    std::cout << "├─ Recommended:      4+ CPU cores, 8GB+ RAM" << std::endl;
    std::cout << "├─ Memory Usage:     1.5-2.5GB (models + runtime)" << std::endl;
    std::cout << "├─ Response Time:    1-3 seconds per AI operation" << std::endl;
    std::cout << "├─ Concurrency:      15-25 simultaneous users" << std::endl;
    std::cout << "├─ Startup Time:     5-15 seconds (model loading)" << std::endl;
    std::cout << "└─ Throughput:       1000-3000 operations/hour\n" << std::endl;
}

int main(int argc, char* argv[]) {
    // Print banner
    printBanner();
    
    // Parse command line arguments
    std::string host = "0.0.0.0";
    int port = 8080;
    std::string modelPath = "models/distilgpt2.Q4_K_M.gguf";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "--host" || arg == "-h") && i + 1 < argc) {
            host = argv[++i];
        } else if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if ((arg == "--model" || arg == "-m") && i + 1 < argc) {
            modelPath = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --host, -h <host>     Server host (default: 0.0.0.0)" << std::endl;
            std::cout << "  --port, -p <port>     Server port (default: 8080)" << std::endl;
            std::cout << "  --model, -m <path>    Model path (default: models/distilgpt2.Q4_K_M.gguf)" << std::endl;
            std::cout << "  --help                Show this help message" << std::endl;
            return 0;
        }
    }
    
    // Print server information
    printServerInfo(host, port, modelPath);
    printEndpoints();
    printAIFeatures();
    printPerformanceInfo();
    printUsageExamples(host, port);
    printExpectedPerformance();
    
    try {
        std::cout << "🔄 Initializing AEON AI Server..." << std::endl;
        
        // Create server instance with AI model
        g_server = std::make_unique<HttpServer>(host, port, modelPath);
        
        // Set up signal handlers for graceful shutdown
        signal(SIGINT, signalHandler);   // Ctrl+C
        signal(SIGTERM, signalHandler);  // Termination signal
        
        std::cout << "✅ Server core initialized successfully!" << std::endl;
        
        if (g_server->isAIModelLoaded()) {
            std::cout << "🧠 Neural models loaded and ready for inference!" << std::endl;
        } else {
            std::cout << "⚠️ Warning: AI models failed to load - check model path!" << std::endl;
        }
        
        std::cout << "🚀 Starting HTTP API service..." << std::endl;
        
        // Start the server
        if (!g_server->start()) {
            std::cerr << "❌ Failed to start server!" << std::endl;
            return 1;
        }
        
        std::cout << "🎉 AEON AI Platform is online!" << std::endl;
        std::cout << "🌐 Access dashboard at: http://" << host << ":" << port << "/" << std::endl;
        std::cout << "📊 View API documentation at: http://" << host << ":" << port << "/api" << std::endl;
        std::cout << "💡 Press Ctrl+C to shutdown server" << std::endl;
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "AEON AI SERVER LOG:" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        // Keep the main thread alive and show periodic stats
        int counter = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Print periodic stats every 2 minutes
            if (++counter % 120 == 0 && g_server->getTotalRequests() > 0) {
                std::cout << "📊 AEON AI Status | "
                          << "Requests: " << g_server->getTotalRequests() 
                          << " | Success: " << g_server->getSuccessfulGenerations()
                          << " | Errors: " << g_server->getFailedGenerations()
                          << " | Uptime: " << g_server->getUptime() << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Server error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Unknown server error occurred!" << std::endl;
        return 1;
    }
    
    return 0;
}