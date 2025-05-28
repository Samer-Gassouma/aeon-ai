#include "http_server.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>

HttpServer::HttpServer(const std::string& host, int port, const std::string& modelPath) 
    : host(host), port(port), startTime(std::chrono::steady_clock::now()) {
    
    server = std::make_unique<httplib::Server>();
    
    std::cout << "🚀 Initializing AI Quiz Server..." << std::endl;
    std::cout << "📍 Host: " << host << ":" << port << std::endl;
    std::cout << "🤖 Model: " << modelPath << std::endl;
    
    // Initialize AI generator with same model for all three purposes
    // (for now using single model - can be extended to use separate models)
    aiGenerator = std::make_unique<AIQuizGenerator>(modelPath, modelPath, modelPath);
    
    // Configure server with detailed logging
    server->set_logger([](const httplib::Request& req, const httplib::Response& res) {
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
        std::cout << "[" << timestamp << "] " 
                  << req.method << " " << req.path 
                  << " - " << res.status 
                  << " (" << res.body.length() << " bytes)" << std::endl;
    });
    
    // Set server timeouts for AI generation
    server->set_read_timeout(60);  // Increased for psychology analysis
    server->set_write_timeout(60); // Increased for psychology analysis
    
    setupRoutes();
    std::cout << "✅ HttpServer initialized" << std::endl;
}

void HttpServer::setupRoutes() {
    // Enable CORS for all routes
    server->set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
        setCORSHeaders(res);
        return httplib::Server::HandlerResponse::Unhandled;
    });
    
    // Handle OPTIONS requests for CORS preflight
    server->Options(".*", [this](const httplib::Request& req, httplib::Response& res) {
        setCORSHeaders(res);
        res.status = 200;
    });
    
    // Health check endpoint
    server->Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        handleHealthCheck(req, res);
    });
    
    // AI quiz generation endpoint
    server->Post("/api/quiz/generate", [this](const httplib::Request& req, httplib::Response& res) {
        handleGenerateQuiz(req, res);
    });
    
    // Categories endpoint
    server->Get("/api/quiz/categories", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetCategories(req, res);
    });
    
    // Statistics endpoint
    server->Get("/api/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetStats(req, res);
    });
    
    // Model information endpoint
    server->Get("/api/model/info", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetModelInfo(req, res);
    });
    
    // NEW: Psychology endpoints
    server->Post("/api/psychology/generate", [this](const httplib::Request& req, httplib::Response& res) {
        handleGeneratePsychologyQuestions(req, res);
    });
    
    server->Post("/api/psychology/questions", [this](const httplib::Request& req, httplib::Response& res) {
        handleGeneratePsychologyQuestions(req, res);
    });
    
    server->Post("/api/psychology/analyze", [this](const httplib::Request& req, httplib::Response& res) {
        handleAnalyzePersonality(req, res);
    });
    
    server->Get("/api/psychology/traits", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetPersonalityTraits(req, res);
    });
    
    std::cout << "📡 Routes configured successfully" << std::endl;
}

void HttpServer::handleHealthCheck(const httplib::Request& req, httplib::Response& res) {
    totalRequests++;
    
    Json::Value response;
    response["status"] = "healthy";
    response["service"] = "C++ AI Quiz Generator API";
    response["version"] = "2.0.0";
    response["domain"] = "api.aeonglitch.me";
    response["aiModel"] = "DistilGPT-2";
    response["modelLoaded"] = isAIModelLoaded();
    response["uptime"] = getUptime();
    response["totalRequests"] = getTotalRequests();
    response["successfulGenerations"] = getSuccessfulGenerations();
    response["failedGenerations"] = getFailedGenerations();
    response["timestamp"] = getCurrentTimestamp();
    
    // Add performance info
    if (isAIModelLoaded()) {
        int totalGenerated;
        double avgTime;
        long long totalTime;
        double qpm;
        aiGenerator->getStats(totalGenerated, avgTime, totalTime, qpm);
        
        response["aiStats"]["totalGenerated"] = totalGenerated;
        response["aiStats"]["avgGenerationTimeMs"] = avgTime;
        response["aiStats"]["questionsPerMinute"] = qpm;
        
        // Add psychology stats
        int totalPsychQuestions, totalAnalyses;
        aiGenerator->getPsychologyStats(totalPsychQuestions, totalAnalyses);
        response["psychologyStats"]["totalPsychQuestions"] = totalPsychQuestions;
        response["psychologyStats"]["totalAnalyses"] = totalAnalyses;
    }
    
    sendSuccessResponse(res, response);
}

void HttpServer::handleGenerateQuiz(const httplib::Request& req, httplib::Response& res) {
    totalRequests++;
    
    try {
        if (!isAIModelLoaded()) {
            failedGenerations++;
            sendErrorResponse(res, 503, "AI model not loaded");
            return;
        }
        
        Json::Value requestJson;
        if (!parseJsonRequest(req.body, requestJson)) {
            failedGenerations++;
            sendErrorResponse(res, 400, "Invalid JSON in request body");
            return;
        }
        
        // Extract parameters with defaults
        std::string category = requestJson.get("category", "Science").asString();
        std::string difficulty = requestJson.get("difficulty", "Medium").asString();
        std::string playerName = requestJson.get("playerName", "Unknown").asString();
        
        std::cout << "🎯 Generating AI quiz: " << category << "/" << difficulty 
                  << " for " << playerName << std::endl;
        
        // Generate question using AI
        auto startTime = std::chrono::high_resolution_clock::now();
        QuizQuestion question = aiGenerator->generateQuestion(category, difficulty, playerName);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Build response
        Json::Value response;
        response["success"] = true;
        response["question"] = questionToJson(question);
        response["timestamp"] = getCurrentTimestamp();
        response["aiGenerated"] = question.generated;
        response["aiModel"] = question.aiModel;
        response["generationTime"] = duration.count();
        response["generationTimeUnit"] = "milliseconds";
        response["serverProcessingTime"] = duration.count() - question.generationTimeMs;
        
        if (question.generated) {
            successfulGenerations++;
            std::cout << "✅ AI question generated successfully in " << duration.count() << "ms" << std::endl;
        } else {
            failedGenerations++;
            std::cout << "⚠️ Used fallback question due to AI generation failure" << std::endl;
        }
        
        sendSuccessResponse(res, response);
        
    } catch (const std::exception& e) {
        failedGenerations++;
        std::cerr << "❌ Error generating quiz: " << e.what() << std::endl;
        sendErrorResponse(res, 500, e.what());
    }
}

// NEW: Generate psychology questions
void HttpServer::handleGeneratePsychologyQuestions(const httplib::Request& req, httplib::Response& res) {
    totalRequests++;
    
    try {
        if (!isAIModelLoaded()) {
            sendErrorResponse(res, 503, "AI model not loaded");
            return;
        }
        
        Json::Value requestJson;
        if (!parseJsonRequest(req.body, requestJson)) {
            sendErrorResponse(res, 400, "Invalid JSON in request body");
            return;
        }
        
        int count = requestJson.get("count", 8).asInt(); // Default 8 questions
        if (count < 1 || count > 16) {
            sendErrorResponse(res, 400, "Question count must be between 1 and 16");
            return;
        }
        
        std::cout << "🧠 Generating " << count << " psychology questions..." << std::endl;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        auto questions = aiGenerator->generatePsychologyQuestions(count);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Build response
        Json::Value response;
        response["success"] = true;
        response["count"] = static_cast<int>(questions.size());
        response["generationTime"] = duration.count();
        response["timestamp"] = getCurrentTimestamp();
        
        Json::Value questionsArray(Json::arrayValue);
        for (const auto& question : questions) {
            Json::Value questionJson;
            questionJson["id"] = question.id;
            questionJson["question"] = question.question;
            questionJson["trait"] = question.trait;
            questionJson["category"] = question.category;
            questionJson["generated"] = question.generated;
            questionJson["aiModel"] = question.aiModel;
            questionJson["generationTimeMs"] = question.generationTimeMs;
            
            Json::Value optionsArray(Json::arrayValue);
            for (const auto& option : question.options) {
                optionsArray.append(option);
            }
            questionJson["options"] = optionsArray;
            
            questionsArray.append(questionJson);
        }
        response["questions"] = questionsArray;
        
        std::cout << "✅ Generated " << questions.size() << " psychology questions in " 
                  << duration.count() << "ms" << std::endl;
        
        sendSuccessResponse(res, response);
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error generating psychology questions: " << e.what() << std::endl;
        sendErrorResponse(res, 500, e.what());
    }
}

// NEW: Analyze personality from answers
void HttpServer::handleAnalyzePersonality(const httplib::Request& req, httplib::Response& res) {
    totalRequests++;
    
    try {
        if (!isAIModelLoaded()) {
            sendErrorResponse(res, 503, "AI model not loaded");
            return;
        }
        
        Json::Value requestJson;
        if (!parseJsonRequest(req.body, requestJson)) {
            sendErrorResponse(res, 400, "Invalid JSON in request body");
            return;
        }
        
        if (!requestJson.isMember("answers") || !requestJson["answers"].isArray()) {
            sendErrorResponse(res, 400, "Missing or invalid 'answers' array");
            return;
        }
        
        // Parse answers
        std::vector<PersonalityAnswer> answers;
        for (const auto& answerJson : requestJson["answers"]) {
            PersonalityAnswer answer;
            answer.questionId = answerJson.get("questionId", 1).asInt();
            answer.selectedOption = answerJson.get("selectedOption", 0).asInt();
            answer.trait = answerJson.get("trait", "E/I").asString();
            answers.push_back(answer);
        }
        
        if (answers.empty()) {
            sendErrorResponse(res, 400, "No answers provided");
            return;
        }
        
        std::cout << "🔍 Analyzing personality from " << answers.size() << " answers..." << std::endl;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        PersonalityResult result = aiGenerator->analyzePersonality(answers);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Build response
        Json::Value response;
        response["success"] = true;
        response["personalityType"] = result.personalityType;
        response["title"] = result.title;
        response["description"] = result.description;
        response["confidence"] = result.confidence;
        response["aiGenerated"] = result.aiGenerated;
        response["analysisModel"] = result.analysisModel;
        response["analysisTime"] = duration.count();
        response["timestamp"] = getCurrentTimestamp();
        
        // Add trait scores
        Json::Value scores;
        for (const auto& score : result.scores) {
            scores[score.first] = score.second;
        }
        response["scores"] = scores;
        
        // Add strengths
        Json::Value strengthsArray(Json::arrayValue);
        for (const auto& strength : result.strengths) {
            strengthsArray.append(strength);
        }
        response["strengths"] = strengthsArray;
        
        // Add growth areas
        Json::Value growthArray(Json::arrayValue);
        for (const auto& growth : result.growthAreas) {
            growthArray.append(growth);
        }
        response["growthAreas"] = growthArray;
        
        std::cout << "✅ Personality analysis complete: " << result.personalityType 
                  << " in " << duration.count() << "ms" << std::endl;
        
        sendSuccessResponse(res, response);
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error analyzing personality: " << e.what() << std::endl;
        sendErrorResponse(res, 500, e.what());
    }
}

// NEW: Get personality traits information
void HttpServer::handleGetPersonalityTraits(const httplib::Request& req, httplib::Response& res) {
    totalRequests++;
    
    try {
        Json::Value response;
        response["success"] = true;
        
        // Get personality traits
        auto traits = aiGenerator->getPersonalityTraits();
        Json::Value traitsArray(Json::arrayValue);
        for (const auto& trait : traits) {
            traitsArray.append(trait);
        }
        response["traits"] = traitsArray;
        
        // Get personality types
        auto types = aiGenerator->getPersonalityTypes();
        Json::Value typesArray(Json::arrayValue);
        for (const auto& type : types) {
            typesArray.append(type);
        }
        response["types"] = typesArray;
        
        response["timestamp"] = getCurrentTimestamp();
        response["modelLoaded"] = isAIModelLoaded();
        
        sendSuccessResponse(res, response);
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error getting personality traits: " << e.what() << std::endl;
        sendErrorResponse(res, 500, e.what());
    }
}

void HttpServer::handleGetCategories(const httplib::Request& req, httplib::Response& res) {
    totalRequests++;
    
    try {
        Json::Value response;
        response["success"] = true;
        
        // Get categories map
        auto categoriesMap = aiGenerator->getCategoriesMap();
        Json::Value categories;
        for (const auto& pair : categoriesMap) {
            Json::Value subcategories(Json::arrayValue);
            for (const auto& sub : pair.second) {
                subcategories.append(sub);
            }
            categories[pair.first] = subcategories;
        }
        response["categories"] = categories;
        
        // Get difficulties
        auto difficulties = aiGenerator->getDifficulties();
        Json::Value diffArray(Json::arrayValue);
        for (const auto& diff : difficulties) {
            diffArray.append(diff);
        }
        response["difficulties"] = diffArray;
        
        response["timestamp"] = getCurrentTimestamp();
        response["aiModel"] = "DistilGPT-2";
        response["modelLoaded"] = isAIModelLoaded();
        
        sendSuccessResponse(res, response);
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error getting categories: " << e.what() << std::endl;
        sendErrorResponse(res, 500, e.what());
    }
}

void HttpServer::handleGetStats(const httplib::Request& req, httplib::Response& res) {
    totalRequests++;
    
    Json::Value response;
    response["success"] = true;
    response["server"]["uptime"] = getUptime();
    response["server"]["totalRequests"] = getTotalRequests();
    response["server"]["successfulGenerations"] = getSuccessfulGenerations();
    response["server"]["failedGenerations"] = getFailedGenerations();
    
    if (isAIModelLoaded()) {
        int totalGenerated;
        double avgTime;
        long long totalTime;
        double qpm;
        aiGenerator->getStats(totalGenerated, avgTime, totalTime, qpm);
        
        response["ai"]["totalGenerated"] = totalGenerated;
        response["ai"]["avgGenerationTimeMs"] = avgTime;
        response["ai"]["totalGenerationTimeMs"] = static_cast<int>(totalTime);
        response["ai"]["questionsPerMinute"] = qpm;
        response["ai"]["modelMemoryUsage"] = static_cast<int>(aiGenerator->getModelMemoryUsage());
        
        // Add psychology stats
        int totalPsychQuestions, totalAnalyses;
        aiGenerator->getPsychologyStats(totalPsychQuestions, totalAnalyses);
        response["psychology"]["totalPsychQuestions"] = totalPsychQuestions;
        response["psychology"]["totalAnalyses"] = totalAnalyses;
    } else {
        response["ai"]["status"] = "Model not loaded";
    }
    
    response["timestamp"] = getCurrentTimestamp();
    
    sendSuccessResponse(res, response);
}

void HttpServer::handleGetModelInfo(const httplib::Request& req, httplib::Response& res) {
    totalRequests++;
    
    Json::Value response;
    response["success"] = true;
    response["modelLoaded"] = isAIModelLoaded();
    
    if (isAIModelLoaded()) {
        response["modelInfo"] = aiGenerator->getModelInfo();
        response["memoryUsage"] = static_cast<int>(aiGenerator->getModelMemoryUsage());
        
        // Add loaded models info
        auto loadedModels = aiGenerator->getLoadedModels();
        Json::Value modelsArray(Json::arrayValue);
        for (const auto& model : loadedModels) {
            modelsArray.append(model);
        }
        response["loadedModels"] = modelsArray;
    } else {
        response["error"] = "AI model not loaded";
    }
    
    response["timestamp"] = getCurrentTimestamp();
    
    sendSuccessResponse(res, response);
}

Json::Value HttpServer::questionToJson(const QuizQuestion& question) const {
    Json::Value json;
    
    json["question"] = question.question;
    json["category"] = question.category;
    json["difficulty"] = question.difficulty;
    json["correctAnswerIndex"] = question.correctAnswerIndex;
    json["correctAnswerPriceMultiplier"] = question.correctAnswerPriceMultiplier;
    json["wrongAnswerPriceMultiplier"] = question.wrongAnswerPriceMultiplier;
    json["stealChance"] = question.stealChance;
    json["stealPercentage"] = question.stealPercentage;
    json["generated"] = question.generated;
    json["aiModel"] = question.aiModel;
    json["generationTimeMs"] = question.generationTimeMs;
    
    Json::Value answers(Json::arrayValue);
    for (const auto& answer : question.answers) {
        answers.append(answer);
    }
    json["answers"] = answers;
    
    return json;
}

std::string HttpServer::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    
    return ss.str();
}

void HttpServer::setCORSHeaders(httplib::Response& res) const {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    res.set_header("Access-Control-Max-Age", "86400");
}

bool HttpServer::parseJsonRequest(const std::string& body, Json::Value& json) const {
    if (body.empty()) {
        return true; // Empty body is valid
    }
    
    Json::CharReaderBuilder builder;
    std::string errors;
    std::istringstream stream(body);
    
    return Json::parseFromStream(builder, stream, &json, &errors);
}

void HttpServer::sendErrorResponse(httplib::Response& res, int code, 
                                 const std::string& message) const {
    Json::Value error;
    error["success"] = false;
    error["error"] = message;
    error["timestamp"] = getCurrentTimestamp();
    error["aiModelLoaded"] = isAIModelLoaded();
    
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string jsonString = Json::writeString(builder, error);
    
    res.set_content(jsonString, "application/json");
    res.status = code;
    setCORSHeaders(res);
}

void HttpServer::sendSuccessResponse(httplib::Response& res, const Json::Value& data) const {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string jsonString = Json::writeString(builder, data);
    
    res.set_content(jsonString, "application/json");
    res.status = 200;
    setCORSHeaders(res);
}

bool HttpServer::start() {
    std::cout << "🚀 Starting server on " << host << ":" << port << std::endl;
    
    if (!isAIModelLoaded()) {
        std::cout << "⚠️ Warning: AI model not loaded, some features may not work" << std::endl;
    }
    
    // Start server in a separate thread to avoid blocking
    std::thread serverThread([this]() {
        if (server->listen(host.c_str(), port)) {
            std::cout << "✅ Server started successfully!" << std::endl;
        } else {
            std::cerr << "❌ Failed to start server on " << host << ":" << port << std::endl;
        }
    });
    
    // Wait a moment to let the server start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Keep the main thread alive
    serverThread.detach();
    
    return true;
}

void HttpServer::stop() {
    if (server) {
        server->stop();
        std::cout << "🛑 Server stopped" << std::endl;
    }
}

void HttpServer::setHost(const std::string& newHost) {
    host = newHost;
}

void HttpServer::setPort(int newPort) {
    port = newPort;
}

bool HttpServer::isRunning() const {
    return server && server->is_running();
}

int HttpServer::getTotalRequests() const {
    return totalRequests.load();
}

int HttpServer::getSuccessfulGenerations() const {
    return successfulGenerations.load();
}

int HttpServer::getFailedGenerations() const {
    return failedGenerations.load();
}

std::string HttpServer::getUptime() const {
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    
    int hours = uptime.count() / 3600;
    int minutes = (uptime.count() % 3600) / 60;
    int seconds = uptime.count() % 60;
    
    std::stringstream ss;
    ss << hours << "h " << minutes << "m " << seconds << "s";
    return ss.str();
}

bool HttpServer::isAIModelLoaded() const {
    return aiGenerator && aiGenerator->areModelsLoaded();
}

bool HttpServer::isModelLoading() const {
    return false; // There is no async loading in this implementation
}

bool HttpServer::reloadAIModel() {
    if (aiGenerator) {
        return aiGenerator->reloadModels();
    }
    return false;
}