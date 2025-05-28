#ifndef AI_QUIZ_GENERATOR_H
#define AI_QUIZ_GENERATOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>

// Forward declaration for llama.cpp types
struct llama_model;
struct llama_context;

struct QuizQuestion {
    std::string question;
    std::vector<std::string> answers;
    int correctAnswerIndex;
    std::string category;
    std::string difficulty;
    double correctAnswerPriceMultiplier;
    double wrongAnswerPriceMultiplier;
    double stealChance;
    double stealPercentage;
    bool generated;
    std::string aiModel;
    int generationTimeMs;
};

// New structures for psychological assessment
struct PsychologicalQuestion {
    int id;
    std::string question;
    std::vector<std::string> options;
    std::string trait; // E/I, S/N, T/F, J/P for MBTI
    std::string category; // "Extroversion", "Thinking", etc.
    bool generated;
    std::string aiModel;
    int generationTimeMs;
};

struct PersonalityAnswer {
    int questionId;
    int selectedOption; // 0-based index
    std::string value; // The actual answer text
    std::string trait; // E/I, S/N, T/F, J/P for MBTI
};

struct PersonalityResult {
    std::string personalityType; // e.g., "INTJ", "ENFP"
    std::string title; // e.g., "The Architect", "The Campaigner"
    std::string description;
    std::unordered_map<std::string, double> scores; // trait scores
    std::vector<std::string> strengths;
    std::vector<std::string> growthAreas;
    double confidence; // 0.0 - 1.0
    bool aiGenerated;
    std::string analysisModel;
    int analysisTimeMs;
};

// Model management structure
struct ModelInstance {
    llama_model* model;
    llama_context* context;
    std::string modelPath;
    std::string modelName;
    bool isLoaded;
    std::mutex modelMutex;
    std::atomic<int> usageCount{0};
    std::chrono::steady_clock::time_point lastUsed;
    
    ModelInstance() : model(nullptr), context(nullptr), isLoaded(false) {}
};

class AIQuizGenerator {
private:
    // Multiple small models for different tasks
    std::unique_ptr<ModelInstance> quizModel;        // For quiz questions
    std::unique_ptr<ModelInstance> psychologyModel;  // For psychology questions
    std::unique_ptr<ModelInstance> analysisModel;    // For personality analysis
    
    // Model configuration
    int contextSize;
    int maxTokens;
    float temperature;
    
    // Thread safety for model management
    std::mutex managerMutex;
    
    // Performance tracking
    std::atomic<int> totalQuestionsGenerated{0};
    std::atomic<int> totalPsychQuestionsGenerated{0};
    std::atomic<int> totalPersonalityAnalyses{0};
    std::atomic<long long> totalGenerationTimeMs{0};
    std::chrono::steady_clock::time_point startTime;
    
    // Difficulty modifiers
    std::unordered_map<std::string, std::unordered_map<std::string, double>> difficultyModifiers;
    
    // AI prompt templates for different categories and difficulties
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> promptTemplates;
    
    // Psychological assessment templates and data
    std::unordered_map<std::string, std::string> psychologyPromptTemplates;
    std::unordered_map<std::string, std::vector<std::string>> personalityTraits;
    std::unordered_map<std::string, std::string> personalityDescriptions;
    
    // Private methods for model management
    bool initializeModel(ModelInstance* instance, const std::string& modelPath, const std::string& modelName);
    void cleanupModel(ModelInstance* instance);
    bool isModelLoaded(ModelInstance* instance) const;
    std::string generateText(ModelInstance* instance, const std::string& prompt);
    
    // Initialization methods
    void initializeDifficultyModifiers();
    void initializePromptTemplates();
    void initializePsychologyTemplates();
    void initializePersonalityData();
    
    // Generation methods
    std::string buildPrompt(const std::string& category, const std::string& difficulty) const;
    std::string buildPsychologyPrompt(const std::string& trait, const std::string& category) const;
    
    QuizQuestion parseAIResponse(const std::string& response, 
                                const std::string& category, 
                                const std::string& difficulty);
    
    PsychologicalQuestion parsePsychologyResponse(const std::string& response,
                                                 int questionId,
                                                 const std::string& trait,
                                                 const std::string& category);
    
    // Response parsing helpers
    std::string extractQuestion(const std::string& text) const;
    std::vector<std::string> extractAnswers(const std::string& text) const;
    std::vector<std::string> extractPsychologyOptions(const std::string& text) const;
    int extractCorrectAnswer(const std::string& text) const;
    
    // Personality analysis helpers
    std::unordered_map<std::string, double> calculateTraitScores(const std::vector<PersonalityAnswer>& answers) const;
    std::string determinePersonalityType(const std::unordered_map<std::string, double>& scores) const;
    std::string generatePersonalityDescription(const std::string& personalityType, 
                                              const std::unordered_map<std::string, double>& scores);
    std::vector<std::string> generateStrengthsAndGrowthAreas(const std::string& personalityType, bool isStrengths);

public:
    AIQuizGenerator(const std::string& quizModelPath = "models/distilgpt2-quiz.Q2_K.gguf",
                   const std::string& psychologyModelPath = "models/distilgpt2-psychology.Q2_K.gguf",
                   const std::string& analysisModelPath = "models/distilgpt2-analysis.Q2_K.gguf");
    ~AIQuizGenerator();
    
    // Main generation functions
    QuizQuestion generateQuestion(const std::string& category = "Science",
                                const std::string& difficulty = "Medium",
                                const std::string& playerName = "Unknown");
    
    // Psychology assessment functions
    std::vector<PsychologicalQuestion> generatePsychologyQuestions(int count = 8);
    PersonalityResult analyzePersonality(const std::vector<PersonalityAnswer>& answers);
    
    // Model management
    bool areModelsLoaded() const;
    bool reloadModels();
    std::vector<std::string> getLoadedModels() const;
    
    // Configuration
    void setTemperature(float temp);
    void setMaxTokens(int tokens);
    void setContextSize(int size);
    
    // Get available categories and difficulties
    std::vector<std::string> getCategories() const;
    std::vector<std::string> getDifficulties() const;
    std::unordered_map<std::string, std::vector<std::string>> getCategoriesMap() const;
    
    // Get psychology-related info
    std::vector<std::string> getPersonalityTraits() const;
    std::vector<std::string> getPersonalityTypes() const;
    
    // Performance monitoring
    void getStats(int& totalGenerated, double& avgGenerationTime, 
                 long long& totalTime, double& questionsPerMinute) const;
    void getPsychologyStats(int& totalPsychQuestions, int& totalAnalyses) const;
    
    // Model information
    std::string getModelInfo() const;
    size_t getModelMemoryUsage() const;
};

#endif // AI_QUIZ_GENERATOR_H