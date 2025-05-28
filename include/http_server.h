#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "httplib.h"
#include "ai_quiz_generator.h"
#include <jsoncpp/json/json.h>
#include <memory>
#include <string>
#include <chrono>
#include <atomic>

class HttpServer {
private:
    std::unique_ptr<httplib::Server> server;
    std::unique_ptr<AIQuizGenerator> aiGenerator;
    
    // Server configuration
    std::string host;
    int port;
    
    // Statistics
    std::atomic<int> totalRequests{0};
    std::atomic<int> successfulGenerations{0};
    std::atomic<int> failedGenerations{0};
    std::chrono::steady_clock::time_point startTime;
    
    // Request handlers
    void setupRoutes();
    void handleHealthCheck(const httplib::Request& req, httplib::Response& res);
    void handleGenerateQuiz(const httplib::Request& req, httplib::Response& res);
    void handleGetCategories(const httplib::Request& req, httplib::Response& res);
    void handleGetStats(const httplib::Request& req, httplib::Response& res);
    void handleGetModelInfo(const httplib::Request& req, httplib::Response& res);
    
    // Psychology handlers
    void handleGeneratePsychologyQuestions(const httplib::Request& req, httplib::Response& res);
    void handleAnalyzePersonality(const httplib::Request& req, httplib::Response& res);
    void handleGetPersonalityTraits(const httplib::Request& req, httplib::Response& res);
    
    // Utility functions
    Json::Value questionToJson(const QuizQuestion& question) const;
    std::string getCurrentTimestamp() const;
    void setCORSHeaders(httplib::Response& res) const;
    bool parseJsonRequest(const std::string& body, Json::Value& json) const;
    
    // Error handling
    void sendErrorResponse(httplib::Response& res, int code, 
                          const std::string& message) const;
    void sendSuccessResponse(httplib::Response& res, 
                           const Json::Value& data) const;

public:
    HttpServer(const std::string& host = "0.0.0.0", int port = 8080,
               const std::string& modelPath = "models/distilgpt2.Q4_K_M.gguf");
    ~HttpServer() = default;
    
    // Server control
    bool start();
    void stop();
    
    // Configuration
    void setHost(const std::string& newHost);
    void setPort(int newPort);
    
    // Status
    bool isRunning() const;
    int getTotalRequests() const;
    int getSuccessfulGenerations() const;
    int getFailedGenerations() const;
    std::string getUptime() const;
    
    // AI model management
    bool isAIModelLoaded() const;
    bool reloadAIModel();
    bool isModelLoading() const;
};

#endif // HTTP_SERVER_H