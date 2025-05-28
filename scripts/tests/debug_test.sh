#!/bin/bash

# Debug script for psychology features
SERVER="http://localhost:8082"

echo -e "\n\033[1;36m====== AI Psychology Debug Test Script ======\033[0m\n"

echo -e "1. Getting personality traits..."
echo -e "Command: curl -v $SERVER/api/psychology/traits"
curl -v "$SERVER/api/psychology/traits"
echo -e "\n"

echo -e "2. Generating psychology questions..."
echo -e "Command: curl -v -X POST $SERVER/api/psychology/generate -H \"Content-Type: application/json\" -d '{\"count\": 2}'"
curl -v -X POST "$SERVER/api/psychology/generate" \
  -H "Content-Type: application/json" \
  -d '{"count": 2}'
echo -e "\n"

echo -e "3. Analyzing personality..."
echo -e "Command: curl -v -X POST $SERVER/api/psychology/analyze -H \"Content-Type: application/json\" -d '{\"answers\": [...]}'"
curl -v -X POST "$SERVER/api/psychology/analyze" \
  -H "Content-Type: application/json" \
  -d '{
  "answers": [
    {"questionId": 1, "selectedOption": 0, "trait": "E/I"},
    {"questionId": 2, "selectedOption": 1, "trait": "S/N"},
    {"questionId": 3, "selectedOption": 0, "trait": "T/F"},
    {"questionId": 4, "selectedOption": 1, "trait": "J/P"}
  ]
}'
echo -e "\n\n===== Testing Completed =====\n"
