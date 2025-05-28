# AEON AI

<p align="center">
  <img src="docs/images/aeon_logo.png" alt="AEON AI Logo" width="400"/>
</p>

**Advanced AI-Powered Quiz & Personality Analysis Platform**

AEON AI is a high-performance C++ implementation of an AI-powered quiz generator and personality analysis system. It leverages the DistilGPT-2 language model through llama.cpp to provide dynamic, AI-generated content for quizzes and psychological assessments without any hardcoded questions.

## ✨ Features

- **AI-Generated Quizzes**: Create dynamic quiz questions across multiple categories and difficulty levels
- **Personality Assessment**: Generate and analyze psychology questions based on established personality frameworks
- **High-Performance C++ Core**: Built on optimized C++ with llama.cpp for efficient AI inference
- **RESTful API**: Comprehensive HTTP API for easy integration with any frontend
- **Portable Design**: Runs on Linux, macOS, and Windows platforms
- **Customizable Output**: Tailor questions to specific contexts or user inputs

## 🚀 Getting Started

### Prerequisites

- C++ compiler with C++17 support (GCC 8+, Clang 7+, or MSVC 2019+)
- CMake 3.14+
- Git
- 4+ CPU cores recommended, 8GB+ RAM
- (Optional) Node.js for proxy server if needed

### Installation

1. Clone the repository:
```bash
git clone https://github.com/yourusername/aeon-ai.git
cd aeon-ai
```

2. Download the model:
```bash
mkdir -p models
# Download the DistilGPT-2 model (GGUF format)
wget -O models/distilgpt2.Q4_K_M.gguf https://huggingface.co/TheBloke/distilgpt2-GGUF/resolve/main/distilgpt2.Q4_K_M.gguf
```

3. Build the project:
```bash
mkdir build && cd build
cmake ..
make -j4
```

4. Run the server:
```bash
cd bin
./ai_quiz_server -m ../../models/distilgpt2.Q4_K_M.gguf -p 8082
```

## 📚 API Documentation

### Core Endpoints

- `GET /` - Health check
- `POST /api/quiz/generate` - Generate quiz questions
- `GET /api/quiz/categories` - List available quiz categories
- `POST /api/psychology/questions` - Generate personality assessment questions
- `POST /api/psychology/analyze` - Analyze personality profile responses
- `GET /api/psychology/traits` - List available personality traits
- `GET /api/stats` - Server statistics
- `GET /api/model/info` - AI model information

### Quiz Generation Example

```bash
curl -X POST http://localhost:8082/api/quiz/generate \
  -H "Content-Type: application/json" \
  -d '{
    "category": "Science",
    "difficulty": "Medium",
    "playerName": "User"
  }'
```

### Personality Analysis Example

```bash
curl -X POST http://localhost:8082/api/psychology/analyze \
  -H "Content-Type: application/json" \
  -d '{
    "answers": [
      {"questionId": 1, "selectedOption": 0, "trait": "E/I"},
      {"questionId": 2, "selectedOption": 1, "trait": "S/N"}
    ]
  }'
```

## 📊 Performance

- **Memory Usage**: 1.5-2.5GB (model + runtime)
- **Response Time**: 1-3 seconds per AI operation
- **Concurrency**: Support for 15-25 simultaneous users
- **Throughput**: 1000-3000 operations/hour

## 🛠️ Running as a Service

A systemd service file is available for Linux deployments:

```bash
sudo cp scripts/ai-quiz.service /etc/systemd/system/
sudo systemctl enable ai-quiz.service
sudo systemctl start ai-quiz.service
```

## 🌐 Remote Access

### Using the Node.js Proxy Server

1. Install dependencies:
```bash
npm install express http-proxy-middleware
```

2. Start the proxy server:
```bash
node proxy.js
```

### Using SSH Port Forwarding

```bash
ssh -L 8082:localhost:8082 username@your-server
```

## 📝 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- [llama.cpp](https://github.com/ggerganov/llama.cpp) - Efficient inference of LLM models
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) - HTTP server implementation
- The DistilGPT-2 model from Hugging Face
