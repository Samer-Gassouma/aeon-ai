#include "ai_quiz_generator.h"
#include "llama.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <random>
#include <thread>

AIQuizGenerator::AIQuizGenerator(const std::string &quizModelPath,
                                 const std::string &psychologyModelPath,
                                 const std::string &analysisModelPath)
    : contextSize(1024), maxTokens(128), temperature(0.7), // Reduced for small models
      startTime(std::chrono::steady_clock::now())
{

    // Initialize model instances
    quizModel = std::make_unique<ModelInstance>();
    psychologyModel = std::make_unique<ModelInstance>();
    analysisModel = std::make_unique<ModelInstance>();

    initializeDifficultyModifiers();
    initializePromptTemplates();
    initializePsychologyTemplates();
    initializePersonalityData();

    std::cout << "🤖 AIQuizGenerator initializing with multiple small models..." << std::endl;
    std::cout << "📁 Quiz Model: " << quizModelPath << std::endl;
    std::cout << "📁 Psychology Model: " << psychologyModelPath << std::endl;
    std::cout << "📁 Analysis Model: " << analysisModelPath << std::endl;

    // Initialize models in parallel for faster startup
    std::vector<std::thread> initThreads;

    initThreads.emplace_back([this, quizModelPath]()
                             {
        if (initializeModel(quizModel.get(), quizModelPath, "Quiz-Model")) {
            std::cout << "✅ Quiz model loaded successfully!" << std::endl;
        } else {
            std::cerr << "❌ Failed to load quiz model!" << std::endl;
        } });

    initThreads.emplace_back([this, psychologyModelPath]()
                             {
        if (initializeModel(psychologyModel.get(), psychologyModelPath, "Psychology-Model")) {
            std::cout << "✅ Psychology model loaded successfully!" << std::endl;
        } else {
            std::cerr << "❌ Failed to load psychology model!" << std::endl;
        } });

    initThreads.emplace_back([this, analysisModelPath]()
                             {
        if (initializeModel(analysisModel.get(), analysisModelPath, "Analysis-Model")) {
            std::cout << "✅ Analysis model loaded successfully!" << std::endl;
        } else {
            std::cerr << "❌ Failed to load analysis model!" << std::endl;
        } });

    // Wait for all models to load
    for (auto &thread : initThreads)
    {
        thread.join();
    }

    std::cout << "🧠 Multi-model psychology assessment ready!" << std::endl;
    std::cout << "💾 Total memory usage optimized with small models!" << std::endl;
}

AIQuizGenerator::~AIQuizGenerator()
{
    cleanupModel(quizModel.get());
    cleanupModel(psychologyModel.get());
    cleanupModel(analysisModel.get());
}

bool AIQuizGenerator::initializeModel(ModelInstance *instance, const std::string &modelPath, const std::string &modelName)
{
    std::lock_guard<std::mutex> lock(instance->modelMutex);

    std::cout << "🔄 Loading " << modelName << " from " << modelPath << "..." << std::endl;

    // Initialize llama backend (only once)
    static std::once_flag llamaInitFlag;
    std::call_once(llamaInitFlag, []()
                   { llama_backend_init(); });

    // Model parameters optimized for small models
    auto model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0; // CPU only

    // Load model
    instance->model = llama_model_load_from_file(modelPath.c_str(), model_params);
    if (!instance->model)
    {
        std::cerr << "❌ Failed to load " << modelName << " from: " << modelPath << std::endl;
        return false;
    }

    // Context parameters optimized for small models
    auto ctx_params = llama_context_default_params();
    ctx_params.n_ctx = contextSize;
    ctx_params.n_threads = std::max(1, (int)std::thread::hardware_concurrency() / 3); // Distribute threads
    ctx_params.n_threads_batch = ctx_params.n_threads;

    // Create context
    instance->context = llama_init_from_model(instance->model, ctx_params);
    if (!instance->context)
    {
        std::cerr << "❌ Failed to create context for " << modelName << std::endl;
        llama_model_free(instance->model);
        instance->model = nullptr;
        return false;
    }

    instance->modelPath = modelPath;
    instance->modelName = modelName;
    instance->isLoaded = true;
    instance->lastUsed = std::chrono::steady_clock::now();

    char buf[128];
    llama_model_desc(instance->model, buf, sizeof(buf));
    std::cout << "✅ " << modelName << " loaded: " << buf << std::endl;
    std::cout << "🧠 Context size: " << contextSize << " tokens, Threads: " << ctx_params.n_threads << std::endl;

    return true;
}

void AIQuizGenerator::cleanupModel(ModelInstance *instance)
{
    if (!instance)
        return;

    std::lock_guard<std::mutex> lock(instance->modelMutex);

    if (instance->context)
    {
        llama_free(instance->context);
        instance->context = nullptr;
    }

    if (instance->model)
    {
        llama_model_free(instance->model);
        instance->model = nullptr;
    }

    instance->isLoaded = false;
}

bool AIQuizGenerator::isModelLoaded(ModelInstance *instance) const
{
    if (!instance)
        return false;
    std::lock_guard<std::mutex> lock(instance->modelMutex);
    return instance->isLoaded && instance->model && instance->context;
}

std::string AIQuizGenerator::generateText(ModelInstance *instance, const std::string &prompt)
{
    if (!instance || !isModelLoaded(instance))
    {
        return "";
    }

    std::lock_guard<std::mutex> lock(instance->modelMutex);

    // Update usage stats
    instance->usageCount++;
    instance->lastUsed = std::chrono::steady_clock::now();

    // Tokenize prompt
    std::vector<llama_token> tokens_list;
    tokens_list.resize(prompt.length() + 1);

    int n_tokens = llama_tokenize(llama_model_get_vocab(instance->model), prompt.c_str(), prompt.length(),
                                  tokens_list.data(), tokens_list.size(), false, true);

    if (n_tokens < 0)
    {
        std::cerr << "❌ Failed to tokenize prompt for " << instance->modelName << std::endl;
        return "";
    }

    tokens_list.resize(n_tokens);

    // Reset context
    llama_kv_self_clear(instance->context);

    // Process prompt
    if (llama_decode(instance->context, llama_batch_get_one(tokens_list.data(), n_tokens)) != 0)
    {
        std::cerr << "❌ Failed to decode prompt for " << instance->modelName << std::endl;
        return "";
    }

    // Generate response with reduced token count for small models
    std::string response;
    std::vector<llama_token> response_tokens;

    for (int i = 0; i < maxTokens; ++i)
    {
        // Sample next token
        auto logits = llama_get_logits_ith(instance->context, -1);
        auto n_vocab = llama_vocab_n_tokens(llama_model_get_vocab(instance->model));

        std::vector<llama_token_data> candidates;
        candidates.reserve(n_vocab);

        for (llama_token token_id = 0; token_id < n_vocab; token_id++)
        {
            candidates.emplace_back(llama_token_data{token_id, logits[token_id], 0.0f});
        }

        llama_token_data_array candidates_p = {candidates.data(), candidates.size(), false};

        // Apply temperature (softmax) - optimized for small models
        if (temperature <= 0)
        {
            // Greedy sampling
            for (size_t i = 1; i < candidates_p.size; ++i)
            {
                if (candidates_p.data[i].logit > candidates_p.data[0].logit)
                {
                    candidates_p.data[0] = candidates_p.data[i];
                }
            }
        }
        else
        {
            // Temperature sampling
            float max_l = candidates_p.data[0].logit;
            for (size_t i = 1; i < candidates_p.size; ++i)
            {
                max_l = std::max(max_l, candidates_p.data[i].logit);
            }

            // Compute softmax
            float sum = 0.0f;
            for (size_t i = 0; i < candidates_p.size; ++i)
            {
                float p = expf((candidates_p.data[i].logit - max_l) / temperature);
                candidates_p.data[i].p = p;
                sum += p;
            }

            // Normalize
            for (size_t i = 0; i < candidates_p.size; ++i)
            {
                candidates_p.data[i].p /= sum;
            }

            // Sort by probability
            std::sort(candidates_p.data, candidates_p.data + candidates_p.size,
                      [](const llama_token_data &a, const llama_token_data &b)
                      {
                          return a.p > b.p;
                      });
        }

        // Sample token
        llama_token new_token = candidates_p.data[0].id;

        // Check for end of sequence
        if (new_token == llama_vocab_eos(llama_model_get_vocab(instance->model)))
        {
            break;
        }

        // Convert token to text
        char token_str[256];
        int token_len = llama_token_to_piece(llama_model_get_vocab(instance->model), new_token, token_str, sizeof(token_str), 0, false);

        if (token_len > 0)
        {
            response.append(token_str, token_len);
        }

        response_tokens.push_back(new_token);

        // Decode single token for next iteration
        if (llama_decode(instance->context, llama_batch_get_one(&new_token, 1)) != 0)
        {
            break;
        }

        // Stop if we have a complete question (basic heuristic)
        if (response.find("Answer:") != std::string::npos && response.length() > 50)
        {
            break;
        }
    }

    return response;
}

void AIQuizGenerator::initializeDifficultyModifiers()
{
    difficultyModifiers["Easy"] = {
        {"correct", 0.9}, {"wrong", 1.1}, {"steal", 5.0}, {"amount", 2.0}};
    difficultyModifiers["Medium"] = {
        {"correct", 0.8}, {"wrong", 1.3}, {"steal", 15.0}, {"amount", 5.0}};
    difficultyModifiers["Hard"] = {
        {"correct", 0.6}, {"wrong", 1.5}, {"steal", 25.0}, {"amount", 10.0}};
}

void AIQuizGenerator::initializePromptTemplates()
{
    // Simplified prompts for small models
    promptTemplates["Science"]["Easy"] =
        "Create a basic science question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    promptTemplates["Science"]["Medium"] =
        "Create a science question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    promptTemplates["Science"]["Hard"] =
        "Create an advanced science question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    // Similar simplified patterns for other categories
    promptTemplates["Technology"]["Easy"] =
        "Create a basic tech question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    promptTemplates["Technology"]["Medium"] =
        "Create a tech question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    promptTemplates["Technology"]["Hard"] =
        "Create an advanced tech question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    promptTemplates["Mathematics"]["Easy"] =
        "Create a basic math question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    promptTemplates["Mathematics"]["Medium"] =
        "Create a math question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    promptTemplates["Mathematics"]["Hard"] =
        "Create an advanced math question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    promptTemplates["Engineering"]["Easy"] =
        "Create a basic engineering question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    promptTemplates["Engineering"]["Medium"] =
        "Create an engineering question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";

    promptTemplates["Engineering"]["Hard"] =
        "Create an advanced engineering question with 3 options. "
        "Question: [question]? A) [option1] B) [option2] C) [option3] Answer: [A/B/C]\n"
        "Question:";
}

void AIQuizGenerator::initializePsychologyTemplates()
{
    // Simplified psychology prompts for small models
    psychologyPromptTemplates["E/I_Social"] =
        "Create a personality question about social preferences. "
        "Question: [question]? A) [extroverted] B) [neutral] C) [introverted]\n"
        "Question:";

    psychologyPromptTemplates["E/I_Energy"] =
        "Create a personality question about energy and social recharging. "
        "Question: [question]? A) [extroverted] B) [neutral] C) [introverted]\n"
        "Question:";

    psychologyPromptTemplates["S/N_Information"] =
        "Create a personality question about information processing. "
        "Question: [question]? A) [sensing] B) [neutral] C) [intuition]\n"
        "Question:";

    psychologyPromptTemplates["S/N_Future"] =
        "Create a personality question about future planning. "
        "Question: [question]? A) [sensing] B) [neutral] C) [intuition]\n"
        "Question:";

    psychologyPromptTemplates["T/F_Decisions"] =
        "Create a personality question about decision making. "
        "Question: [question]? A) [thinking] B) [neutral] C) [feeling]\n"
        "Question:";

    psychologyPromptTemplates["T/F_Conflict"] =
        "Create a personality question about handling conflict. "
        "Question: [question]? A) [thinking] B) [neutral] C) [feeling]\n"
        "Question:";

    psychologyPromptTemplates["J/P_Structure"] =
        "Create a personality question about structure and organization. "
        "Question: [question]? A) [judging] B) [neutral] C) [perceiving]\n"
        "Question:";

    psychologyPromptTemplates["J/P_Deadlines"] =
        "Create a personality question about deadlines and time management. "
        "Question: [question]? A) [judging] B) [neutral] C) [perceiving]\n"
        "Question:";
}

void AIQuizGenerator::initializePersonalityData()
{
    // MBTI personality types and their descriptions
    personalityDescriptions["INTJ"] = "The Architect - Strategic, independent, and driven by their vision.";
    personalityDescriptions["INTP"] = "The Thinker - Analytical, innovative, and fascinated by concepts.";
    personalityDescriptions["ENTJ"] = "The Commander - Bold, strategic leaders who organize resources.";
    personalityDescriptions["ENTP"] = "The Debater - Curious, innovative, and excellent at generating ideas.";
    personalityDescriptions["INFJ"] = "The Advocate - Idealistic, principled, and driven to help others.";
    personalityDescriptions["INFP"] = "The Mediator - Creative, caring, and guided by values.";
    personalityDescriptions["ENFJ"] = "The Protagonist - Charismatic, inspiring leaders who care about others.";
    personalityDescriptions["ENFP"] = "The Campaigner - Enthusiastic, creative, and socially free-spirited.";
    personalityDescriptions["ISTJ"] = "The Logistician - Practical, reliable, and committed to duties.";
    personalityDescriptions["ISFJ"] = "The Protector - Caring, loyal, and ready to defend loved ones.";
    personalityDescriptions["ESTJ"] = "The Executive - Organized, practical leaders who get things done.";
    personalityDescriptions["ESFJ"] = "The Consul - Caring, social, and eager to help others succeed.";
    personalityDescriptions["ISTP"] = "The Virtuoso - Practical, observant, skilled at understanding things.";
    personalityDescriptions["ISFP"] = "The Adventurer - Gentle, caring, eager to explore possibilities.";
    personalityDescriptions["ESTP"] = "The Entrepreneur - Energetic, perceptive, skilled at adapting.";
    personalityDescriptions["ESFP"] = "The Entertainer - Enthusiastic, spontaneous, eager to help others have fun.";

    // Personality traits for scoring
    personalityTraits["E/I"] = {"Extroversion", "Introversion"};
    personalityTraits["S/N"] = {"Sensing", "Intuition"};
    personalityTraits["T/F"] = {"Thinking", "Feeling"};
    personalityTraits["J/P"] = {"Judging", "Perceiving"};
}

std::string AIQuizGenerator::buildPrompt(const std::string &category, const std::string &difficulty) const
{
    auto catIt = promptTemplates.find(category);
    if (catIt == promptTemplates.end())
    {
        catIt = promptTemplates.find("Science"); // Default fallback
    }

    auto diffIt = catIt->second.find(difficulty);
    if (diffIt == catIt->second.end())
    {
        diffIt = catIt->second.find("Medium"); // Default fallback
    }

    return diffIt->second;
}

QuizQuestion AIQuizGenerator::generateQuestion(const std::string &category,
                                               const std::string &difficulty,
                                               const std::string &playerName)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    std::cout << "🤖 Generating quiz question using dedicated Quiz Model for " << playerName
              << " (" << category << "/" << difficulty << ")" << std::endl;

    if (!isModelLoaded(quizModel.get()))
    {
        std::cerr << "❌ Quiz model not loaded" << std::endl;
        // Return a basic fallback question
        QuizQuestion fallback;
        fallback.question = "What is an important concept in " + category + "?";
        fallback.answers = {"Concept A", "Concept B", "Concept C"};
        fallback.correctAnswerIndex = 0;
        fallback.category = category;
        fallback.difficulty = difficulty;
        fallback.generated = false;
        fallback.aiModel = "Fallback";
        return fallback;
    }

    // Build AI prompt
    std::string prompt = buildPrompt(category, difficulty);

    // Generate AI response using dedicated quiz model
    std::string aiResponse = generateText(quizModel.get(), prompt);

    std::cout << "🔍 Quiz AI Response: " << aiResponse.substr(0, 100) << "..." << std::endl;

    // Parse response into structured question
    QuizQuestion question = parseAIResponse(aiResponse, category, difficulty);
    question.aiModel = "DistilGPT-2-Quiz-Q2_K";

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    question.generationTimeMs = duration.count();
    totalQuestionsGenerated++;
    totalGenerationTimeMs += duration.count();

    std::cout << "✅ Quiz question generated in " << duration.count() << "ms: "
              << question.question.substr(0, 50) << "..." << std::endl;

    return question;
}

// NEW: Psychology question generation using dedicated psychology model
std::vector<PsychologicalQuestion> AIQuizGenerator::generatePsychologyQuestions(int count)
{
    std::vector<PsychologicalQuestion> questions;

    if (!isModelLoaded(psychologyModel.get()))
    {
        std::cerr << "❌ Psychology model not loaded" << std::endl;
        return questions;
    }

    std::cout << "🧠 Generating " << count << " psychology questions using dedicated Psychology Model..." << std::endl;

    // Define question categories to ensure balanced assessment
    std::vector<std::string> categories = {
        "E/I_Social", "E/I_Energy",
        "S/N_Information", "S/N_Future",
        "T/F_Decisions", "T/F_Conflict",
        "J/P_Structure", "J/P_Deadlines"};

    for (int i = 0; i < count && i < categories.size(); ++i)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::string category = categories[i];
        std::string trait = category.substr(0, 3); // Extract "E/I", "S/N", etc.

        std::cout << "🔄 Generating psychology question " << (i + 1) << "/" << count
                  << " (Category: " << category << ")" << std::endl;

        // Build AI prompt for psychology question
        std::string prompt = buildPsychologyPrompt(trait, category);

        // Generate AI response using dedicated psychology model
        std::string aiResponse = generateText(psychologyModel.get(), prompt);

        // Parse response into psychological question
        PsychologicalQuestion question = parsePsychologyResponse(aiResponse, i + 1, trait, category);
        question.aiModel = "DistilGPT-2-Psychology-Q2_K";

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        question.generationTimeMs = duration.count();
        totalPsychQuestionsGenerated++;

        questions.push_back(question);

        std::cout << "✅ Psychology question " << (i + 1) << " generated in "
                  << duration.count() << "ms" << std::endl;
    }

    std::cout << "🧠 Psychology questionnaire generation complete using dedicated model!" << std::endl;
    return questions;
}

// NEW: Personality analysis using dedicated analysis model
PersonalityResult AIQuizGenerator::analyzePersonality(const std::vector<PersonalityAnswer> &answers)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    std::cout << "🔍 Analyzing personality using dedicated Analysis Model from " << answers.size() << " responses..." << std::endl;

    PersonalityResult result;
    result.aiGenerated = true;
    result.analysisModel = "MBTI + DistilGPT-2-Analysis-Q2_K";

    // Calculate trait scores based on answers
    result.scores = calculateTraitScores(answers);

    // Determine personality type
    result.personalityType = determinePersonalityType(result.scores);

    // Get base description
    auto descIt = personalityDescriptions.find(result.personalityType);
    if (descIt != personalityDescriptions.end())
    {
        result.title = descIt->second.substr(0, descIt->second.find(" - "));

        // Generate enhanced description using analysis model
        if (isModelLoaded(analysisModel.get()))
        {
            result.description = generatePersonalityDescription(result.personalityType, result.scores);
        }
        else
        {
            result.description = descIt->second;
        }
    }
    else
    {
        result.title = "Unique Personality";
        result.description = "A distinctive personality pattern with unique traits.";
    }

    // Generate strengths and growth areas
    result.strengths = generateStrengthsAndGrowthAreas(result.personalityType, true);
    result.growthAreas = generateStrengthsAndGrowthAreas(result.personalityType, false);

    // Calculate confidence based on how clear the preferences are
    double totalConfidence = 0.0;
    for (const auto &score : result.scores)
    {
        double preference = std::abs(score.second - 0.5); // Distance from neutral
        totalConfidence += preference;
    }
    result.confidence = std::min(1.0, totalConfidence / result.scores.size() * 2.0);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    result.analysisTimeMs = duration.count();
    totalPersonalityAnalyses++;

    std::cout << "✅ Personality analysis complete using dedicated model: " << result.personalityType
              << " (" << result.title << ") in " << duration.count() << "ms" << std::endl;

    return result;
}

// Helper methods implementation (continued in next part due to length)
std::string AIQuizGenerator::buildPsychologyPrompt(const std::string &trait, const std::string &category) const
{
    auto it = psychologyPromptTemplates.find(category);
    if (it != psychologyPromptTemplates.end())
    {
        return it->second;
    }

    // Fallback prompt
    return "Create a personality question with 3 options. "
           "Question: [question]? A) [option1] B) [option2] C) [option3]\n"
           "Question:";
}

QuizQuestion AIQuizGenerator::parseAIResponse(const std::string &response,
                                              const std::string &category,
                                              const std::string &difficulty)
{
    QuizQuestion question;
    question.category = category;
    question.difficulty = difficulty;
    question.generated = true;

    // Extract question
    question.question = extractQuestion(response);
    if (question.question.empty())
    {
        question.question = "What is a fundamental concept in " + category + "?";
    }

    // Extract answers
    question.answers = extractAnswers(response);
    if (question.answers.size() != 3)
    {
        question.answers = {"Option A", "Option B", "Option C"};
    }

    // Extract correct answer
    question.correctAnswerIndex = extractCorrectAnswer(response);
    if (question.correctAnswerIndex < 0 || question.correctAnswerIndex > 2)
    {
        question.correctAnswerIndex = 0; // Default to first option
    }

    // Set difficulty modifiers
    auto diffMod = difficultyModifiers.find(difficulty);
    if (diffMod == difficultyModifiers.end())
    {
        diffMod = difficultyModifiers.find("Medium");
    }

    question.correctAnswerPriceMultiplier = diffMod->second.at("correct");
    question.wrongAnswerPriceMultiplier = diffMod->second.at("wrong");
    question.stealChance = diffMod->second.at("steal");
    question.stealPercentage = diffMod->second.at("amount");

    return question;
}

PsychologicalQuestion AIQuizGenerator::parsePsychologyResponse(const std::string &response,
                                                               int questionId,
                                                               const std::string &trait,
                                                               const std::string &category)
{
    PsychologicalQuestion question;
    question.id = questionId;
    question.trait = trait;
    question.category = category;
    question.generated = true;

    // Extract question
    question.question = extractQuestion(response);
    if (question.question.empty())
    {
        question.question = "How would you describe yourself in most situations?";
    }

    // Extract psychology options (3 options: trait1, neutral, trait2)
    question.options = extractPsychologyOptions(response);
    if (question.options.size() != 3)
    {
        question.options = {"Very much like me", "Somewhat like me", "Not like me"};
    }

    return question;
}

// Additional helper methods remain the same...
std::string AIQuizGenerator::extractQuestion(const std::string &text) const
{
    std::regex question_regex(R"(Question:\s*([^?]+\?))");
    std::smatch match;

    if (std::regex_search(text, match, question_regex))
    {
        std::string question = match[1].str();
        question = std::regex_replace(question, std::regex(R"(\s+)"), " ");
        question = std::regex_replace(question, std::regex(R"(^\s+|\s+$)"), "");
        return question;
    }

    std::regex fallback_regex(R"(([^.!?]*\?))");
    if (std::regex_search(text, match, fallback_regex))
    {
        std::string question = match[1].str();
        question = std::regex_replace(question, std::regex(R"(\s+)"), " ");
        question = std::regex_replace(question, std::regex(R"(^\s+|\s+$)"), "");
        return question;
    }

    return "";
}

std::vector<std::string> AIQuizGenerator::extractAnswers(const std::string &text) const
{
    std::vector<std::string> answers;

    std::regex answer_regex(R"([ABC]\)\s*([^AB\n]+))");
    std::sregex_iterator iter(text.begin(), text.end(), answer_regex);
    std::sregex_iterator end;

    for (; iter != end; ++iter)
    {
        std::string answer = (*iter)[1].str();
        answer = std::regex_replace(answer, std::regex(R"(\s+)"), " ");
        answer = std::regex_replace(answer, std::regex(R"(^\s+|\s+$)"), "");
        answer = std::regex_replace(answer, std::regex(R"([,;]\s*$)"), "");

        if (!answer.empty() && answers.size() < 3)
        {
            answers.push_back(answer);
        }
    }

    while (answers.size() < 3)
    {
        answers.push_back("Option " + std::to_string(answers.size() + 1));
    }

    return std::vector<std::string>(answers.begin(), answers.begin() + 3);
}

std::vector<std::string> AIQuizGenerator::extractPsychologyOptions(const std::string &text) const
{
    std::vector<std::string> options;

    std::regex option_regex(R"([ABC]\)\s*([^AB\n]+))");
    std::sregex_iterator iter(text.begin(), text.end(), option_regex);
    std::sregex_iterator end;

    for (; iter != end; ++iter)
    {
        std::string option = (*iter)[1].str();
        option = std::regex_replace(option, std::regex(R"(\s+)"), " ");
        option = std::regex_replace(option, std::regex(R"(^\s+|\s+$)"), "");
        option = std::regex_replace(option, std::regex(R"([,;]\s*$)"), "");

        if (!option.empty() && options.size() < 3)
        {
            options.push_back(option);
        }
    }

    while (options.size() < 3)
    {
        if (options.size() == 0)
            options.push_back("Strongly agree");
        else if (options.size() == 1)
            options.push_back("Neutral");
        else
            options.push_back("Strongly disagree");
    }

    return std::vector<std::string>(options.begin(), options.begin() + 3);
}

int AIQuizGenerator::extractCorrectAnswer(const std::string &text) const
{
    std::regex answer_regex(R"(Answer:\s*([ABC]))");
    std::smatch match;

    if (std::regex_search(text, match, answer_regex))
    {
        char answer_char = match[1].str()[0];
        return answer_char - 'A';
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<> dist({50, 30, 20});
    return dist(gen);
}

std::unordered_map<std::string, double> AIQuizGenerator::calculateTraitScores(const std::vector<PersonalityAnswer> &answers) const
{
    std::unordered_map<std::string, double> scores;

    scores["E"] = 0.5;
    scores["I"] = 0.5;
    scores["S"] = 0.5;
    scores["N"] = 0.5;
    scores["T"] = 0.5;
    scores["F"] = 0.5;
    scores["J"] = 0.5;
    scores["P"] = 0.5;

    for (const auto &answer : answers)
    {
        std::string trait;
        if (answer.questionId <= 2)
            trait = "E/I";
        else if (answer.questionId <= 4)
            trait = "S/N";
        else if (answer.questionId <= 6)
            trait = "T/F";
        else
            trait = "J/P";

        double score = 0.5;
        if (answer.selectedOption == 0)
            score = 0.8;
        else if (answer.selectedOption == 2)
            score = 0.2;

        if (trait == "E/I")
        {
            scores["E"] = (scores["E"] + score) / 2.0;
            scores["I"] = 1.0 - scores["E"];
        }
        else if (trait == "S/N")
        {
            scores["S"] = (scores["S"] + score) / 2.0;
            scores["N"] = 1.0 - scores["S"];
        }
        else if (trait == "T/F")
        {
            scores["T"] = (scores["T"] + score) / 2.0;
            scores["F"] = 1.0 - scores["T"];
        }
        else if (trait == "J/P")
        {
            scores["J"] = (scores["J"] + score) / 2.0;
            scores["P"] = 1.0 - scores["J"];
        }
    }

    return scores;
}

std::string AIQuizGenerator::determinePersonalityType(const std::unordered_map<std::string, double> &scores) const
{
    std::string type;

    type += (scores.at("E") > scores.at("I")) ? "E" : "I";
    type += (scores.at("S") > scores.at("N")) ? "S" : "N";
    type += (scores.at("T") > scores.at("F")) ? "T" : "F";
    type += (scores.at("J") > scores.at("P")) ? "J" : "P";

    return type;
}

std::string AIQuizGenerator::generatePersonalityDescription(const std::string &personalityType,
                                                            const std::unordered_map<std::string, double> &scores)
{
    if (!isModelLoaded(analysisModel.get()))
    {
        auto it = personalityDescriptions.find(personalityType);
        if (it != personalityDescriptions.end())
        {
            return it->second;
        }
        return "A unique personality type with distinctive characteristics.";
    }

    // Generate AI-powered description using dedicated analysis model
    std::string prompt = "Describe " + personalityType + " personality type. Key traits and characteristics:";

    std::string aiDescription = generateText(analysisModel.get(), prompt);

    // Clean and format the description
    if (!aiDescription.empty() && aiDescription.length() > 50)
    {
        return aiDescription.substr(0, 200) + (aiDescription.length() > 200 ? "..." : "");
    }

    // Fallback to static description
    auto it = personalityDescriptions.find(personalityType);
    return (it != personalityDescriptions.end()) ? it->second : "A distinctive personality type.";
}

std::vector<std::string> AIQuizGenerator::generateStrengthsAndGrowthAreas(const std::string &personalityType, bool isStrengths)
{
    std::vector<std::string> items;

    // Static data based on MBTI research (keeping this for reliability)
    std::unordered_map<std::string, std::vector<std::string>> strengthsMap = {
        {"INTJ", {"Strategic thinking", "Independent problem-solving", "Long-term vision"}},
        {"INTP", {"Logical analysis", "Creative problem-solving", "Intellectual curiosity"}},
        {"ENTJ", {"Leadership", "Strategic planning", "Decision-making"}},
        {"ENTP", {"Innovation", "Enthusiasm", "Communication"}},
        {"INFJ", {"Empathy", "Insight", "Idealism"}},
        {"INFP", {"Authenticity", "Creativity", "Compassion"}},
        {"ENFJ", {"Inspiring others", "Communication", "Empathy"}},
        {"ENFP", {"Enthusiasm", "Creativity", "People skills"}},
        {"ISTJ", {"Reliability", "Organization", "Attention to detail"}},
        {"ISFJ", {"Caring nature", "Loyalty", "Supportiveness"}},
        {"ESTJ", {"Leadership", "Organization", "Efficiency"}},
        {"ESFJ", {"People skills", "Organization", "Loyalty"}},
        {"ISTP", {"Problem-solving", "Practical skills", "Adaptability"}},
        {"ISFP", {"Creativity", "Empathy", "Authenticity"}},
        {"ESTP", {"Adaptability", "People skills", "Problem-solving"}},
        {"ESFP", {"Enthusiasm", "People skills", "Creativity"}}};

    std::unordered_map<std::string, std::vector<std::string>> growthMap = {
        {"INTJ", {"Interpersonal communication", "Flexibility", "Patience"}},
        {"INTP", {"Follow-through", "Practical application", "Time management"}},
        {"ENTJ", {"Patience", "Active listening", "Work-life balance"}},
        {"ENTP", {"Focus and follow-through", "Attention to detail", "Routine tasks"}},
        {"INFJ", {"Assertiveness", "Practical decisions", "Self-care"}},
        {"INFP", {"Structure", "Deadlines", "Conflict handling"}},
        {"ENFJ", {"Personal boundaries", "Self-focus", "Saying no"}},
        {"ENFP", {"Organization", "Follow-through", "Detail attention"}},
        {"ISTJ", {"Flexibility", "Innovation", "Emotional expression"}},
        {"ISFJ", {"Assertiveness", "Personal needs", "Change adaptation"}},
        {"ESTJ", {"Emotional awareness", "Flexibility", "Patience"}},
        {"ESFJ", {"Personal boundaries", "Criticism handling", "Self-advocacy"}},
        {"ISTP", {"Long-term planning", "Emotional expression", "Teamwork"}},
        {"ISFP", {"Assertiveness", "Structure", "Conflict engagement"}},
        {"ESTP", {"Long-term planning", "Detail attention", "Reflection"}},
        {"ESFP", {"Organization", "Long-term focus", "Criticism handling"}}};

    if (isStrengths)
    {
        auto it = strengthsMap.find(personalityType);
        if (it != strengthsMap.end())
        {
            return it->second;
        }
    }
    else
    {
        auto it = growthMap.find(personalityType);
        if (it != growthMap.end())
        {
            return it->second;
        }
    }

    // Generic fallback
    if (isStrengths)
    {
        return {"Unique perspective", "Personal authenticity", "Individual strengths"};
    }
    else
    {
        return {"Continued learning", "Skill development", "Personal growth"};
    }
}

// Model management methods
bool AIQuizGenerator::areModelsLoaded() const
{
    return isModelLoaded(quizModel.get()) &&
           isModelLoaded(psychologyModel.get()) &&
           isModelLoaded(analysisModel.get());
}

bool AIQuizGenerator::reloadModels()
{
    std::lock_guard<std::mutex> lock(managerMutex);

    std::cout << "🔄 Reloading all models..." << std::endl;

    // Clean up existing models
    cleanupModel(quizModel.get());
    cleanupModel(psychologyModel.get());
    cleanupModel(analysisModel.get());

    // Reinitialize models
    bool success = true;
    success &= initializeModel(quizModel.get(), quizModel->modelPath, "Quiz-Model");
    success &= initializeModel(psychologyModel.get(), psychologyModel->modelPath, "Psychology-Model");
    success &= initializeModel(analysisModel.get(), analysisModel->modelPath, "Analysis-Model");

    return success;
}

std::vector<std::string> AIQuizGenerator::getLoadedModels() const
{
    std::vector<std::string> loadedModels;

    if (isModelLoaded(quizModel.get()))
    {
        loadedModels.push_back("Quiz-Model (" + std::to_string(quizModel->usageCount.load()) + " uses)");
    }

    if (isModelLoaded(psychologyModel.get()))
    {
        loadedModels.push_back("Psychology-Model (" + std::to_string(psychologyModel->usageCount.load()) + " uses)");
    }

    if (isModelLoaded(analysisModel.get()))
    {
        loadedModels.push_back("Analysis-Model (" + std::to_string(analysisModel->usageCount.load()) + " uses)");
    }

    return loadedModels;
}

void AIQuizGenerator::setTemperature(float temp)
{
    temperature = std::max(0.1f, std::min(1.5f, temp)); // Reduced range for small models
}

void AIQuizGenerator::setMaxTokens(int tokens)
{
    maxTokens = std::max(32, std::min(256, tokens)); // Reduced for small models
}

void AIQuizGenerator::setContextSize(int size)
{
    contextSize = std::max(512, std::min(2048, size)); // Reduced for small models
}

std::vector<std::string> AIQuizGenerator::getCategories() const
{
    return {"Science", "Technology", "Mathematics", "Engineering"};
}

std::vector<std::string> AIQuizGenerator::getDifficulties() const
{
    return {"Easy", "Medium", "Hard"};
}

std::unordered_map<std::string, std::vector<std::string>> AIQuizGenerator::getCategoriesMap() const
{
    std::unordered_map<std::string, std::vector<std::string>> result;

    result["Science"] = {"Physics", "Chemistry", "Biology", "Earth Science"};
    result["Technology"] = {"Programming", "Computer Science", "AI", "Networking"};
    result["Mathematics"] = {"Algebra", "Geometry", "Calculus", "Statistics"};
    result["Engineering"] = {"Civil", "Mechanical", "Electrical", "Software"};

    return result;
}

std::vector<std::string> AIQuizGenerator::getPersonalityTraits() const
{
    return {"Extroversion/Introversion", "Sensing/Intuition", "Thinking/Feeling", "Judging/Perceiving"};
}

std::vector<std::string> AIQuizGenerator::getPersonalityTypes() const
{
    std::vector<std::string> types;
    for (const auto &pair : personalityDescriptions)
    {
        types.push_back(pair.first);
    }
    return types;
}

void AIQuizGenerator::getStats(int &totalGenerated, double &avgGenerationTime,
                               long long &totalTime, double &questionsPerMinute) const
{
    totalGenerated = totalQuestionsGenerated.load();
    totalTime = totalGenerationTimeMs.load();

    if (totalGenerated > 0)
    {
        avgGenerationTime = static_cast<double>(totalTime) / totalGenerated;
    }
    else
    {
        avgGenerationTime = 0.0;
    }

    auto currentTime = std::chrono::steady_clock::now();
    auto uptimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

    if (uptimeMs > 0)
    {
        questionsPerMinute = (totalGenerated * 60000.0) / uptimeMs;
    }
    else
    {
        questionsPerMinute = 0.0;
    }
}

void AIQuizGenerator::getPsychologyStats(int &totalPsychQuestions, int &totalAnalyses) const
{
    totalPsychQuestions = totalPsychQuestionsGenerated.load();
    totalAnalyses = totalPersonalityAnalyses.load();
}

std::string AIQuizGenerator::getModelInfo() const
{
    std::ostringstream info;

    info << "Multi-Model Architecture:\n";

    if (isModelLoaded(quizModel.get()))
    {
        char buf[128];
        llama_model_desc(quizModel->model, buf, sizeof(buf));
        info << "Quiz Model: " << buf << " (Uses: " << quizModel->usageCount.load() << ")\n";
    }

    if (isModelLoaded(psychologyModel.get()))
    {
        char buf[128];
        llama_model_desc(psychologyModel->model, buf, sizeof(buf));
        info << "Psychology Model: " << buf << " (Uses: " << psychologyModel->usageCount.load() << ")\n";
    }

    if (isModelLoaded(analysisModel.get()))
    {
        char buf[128];
        llama_model_desc(analysisModel->model, buf, sizeof(buf));
        info << "Analysis Model: " << buf << " (Uses: " << analysisModel->usageCount.load() << ")\n";
    }

    info << "Context size: " << contextSize << "\n";
    info << "Max tokens: " << maxTokens << "\n";
    info << "Temperature: " << temperature;

    return info.str();
}

size_t AIQuizGenerator::getModelMemoryUsage() const
{
    size_t totalUsage = 0;

    if (isModelLoaded(quizModel.get()))
    {
        totalUsage += llama_model_size(quizModel->model);
    }

    if (isModelLoaded(psychologyModel.get()))
    {
        totalUsage += llama_model_size(psychologyModel->model);
    }

    if (isModelLoaded(analysisModel.get()))
    {
        totalUsage += llama_model_size(analysisModel->model);
    }

    return totalUsage;
}
// End of AIQuizGenerator implementation
