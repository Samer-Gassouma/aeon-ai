#!/bin/bash

# Test script for psychology features of the AI Quiz Generator
echo -e "\n\033[1;36m====== AI Psychology Test Script ======\033[0m\n"

SERVER="http://localhost:8082"

# Function to format JSON output
format_json() {
  echo "$1"
}

# Test personality traits endpoint
echo -e "\033[1;33m1. Testing Personality Traits Endpoint\033[0m"
echo -e "\033[0;36m   GET $SERVER/api/psychology/traits\033[0m"
traits_response=$(curl -s "$SERVER/api/psychology/traits")
format_json "$traits_response"
echo -e "\n"

# Generate psychology questions
echo -e "\033[1;33m2. Generating Psychology Questions\033[0m"
echo -e "\033[0;36m   POST $SERVER/api/psychology/generate\033[0m"
questions_payload='{"count": 4}'
echo -e "\033[0;36m   Payload: $questions_payload\033[0m"
questions_response=$(curl -s -X POST "$SERVER/api/psychology/generate" \
  -H "Content-Type: application/json" \
  -d "$questions_payload")
format_json "$questions_response"
echo -e "\n"

# Extract question IDs for clarity
echo -e "\033[1;33m3. Extracted Question IDs and Traits\033[0m"
question_ids=$(echo "$questions_response" | grep -o '"id":[^,]*' | cut -d":" -f2)
question_traits=$(echo "$questions_response" | grep -o '"trait":"[^"]*"' | cut -d":" -f2 | tr -d '"')

# Create an array to store questions for easier reference
declare -a questions
i=0
for id in $question_ids; do
  trait=$(echo "$question_traits" | sed -n "$((i+1))p")
  questions[$i]="ID: $id, Trait: $trait"
  echo -e "\033[0;32m   Question $((i+1)): ${questions[$i]}\033[0m"
  i=$((i+1))
done
echo -e "\n"

# Analyze personality based on mock answers
echo -e "\033[1;33m4. Analyzing Personality\033[0m"
echo -e "\033[0;36m   POST $SERVER/api/psychology/analyze\033[0m"
answers_payload='{
  "answers": [
    {"questionId": 1, "selectedOption": 0, "trait": "E/I"},
    {"questionId": 2, "selectedOption": 1, "trait": "S/N"},
    {"questionId": 3, "selectedOption": 0, "trait": "T/F"},
    {"questionId": 4, "selectedOption": 1, "trait": "J/P"}
  ]
}'
echo -e "\033[0;36m   Payload: $answers_payload\033[0m"
analysis_response=$(curl -s -X POST "$SERVER/api/psychology/analyze" \
  -H "Content-Type: application/json" \
  -d "$answers_payload")
format_json "$analysis_response"
echo -e "\n"

# Extract and display personality type
personality_type=$(echo "$analysis_response" | grep -o '"personalityType":"[^"]*"' | cut -d":" -f2 | tr -d '"')
title=$(echo "$analysis_response" | grep -o '"title":"[^"]*"' | cut -d":" -f2 | tr -d '"')
echo -e "\033[1;32m====== Personality Analysis Result ======\033[0m"
echo -e "\033[1;36m   Type: $personality_type - $title\033[0m"

# Extract strengths and growth areas
strengths=$(echo "$analysis_response" | grep -o '"strengths":\[.*\]' | sed 's/"strengths":\[//g' | sed 's/\]//g' | tr -d '"')
growth_areas=$(echo "$analysis_response" | grep -o '"growthAreas":\[.*\]' | sed 's/"growthAreas":\[//g' | sed 's/\]//g' | tr -d '"')

echo -e "\033[0;32m   Strengths: $strengths\033[0m"
echo -e "\033[0;33m   Growth Areas: $growth_areas\033[0m"
echo -e "\n\033[1;36m====== Test Completed ======\033[0m\n"
