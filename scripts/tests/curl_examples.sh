#!/bin/bash

# API Test Commands
SERVER="http://46.37.123.43:8083"

echo -e "\n===== AI Quiz API Test Commands ======\n"
echo -e "Run these commands from your laptop to test the API remotely.\n"

echo -e "1. Health Check:"
echo -e "curl -s $SERVER/"
echo -e "\n"

echo -e "2. Get Personality Traits:"
echo -e "curl -s $SERVER/api/psychology/traits"
echo -e "\n"

echo -e "3. Generate Psychology Questions:"
echo -e "curl -s -X POST $SERVER/api/psychology/generate \\"
echo -e "  -H \"Content-Type: application/json\" \\"
echo -e "  -d '{\"count\": 4}'"
echo -e "\n"

echo -e "4. Analyze Personality:"
echo -e "curl -s -X POST $SERVER/api/psychology/analyze \\"
echo -e "  -H \"Content-Type: application/json\" \\"
echo -e "  -d '{"
echo -e "  \"answers\": ["
echo -e "    {\"questionId\": 1, \"selectedOption\": 0, \"trait\": \"E/I\"},"
echo -e "    {\"questionId\": 2, \"selectedOption\": 1, \"trait\": \"S/N\"},"
echo -e "    {\"questionId\": 3, \"selectedOption\": 0, \"trait\": \"T/F\"},"
echo -e "    {\"questionId\": 4, \"selectedOption\": 1, \"trait\": \"J/P\"}"
echo -e "  ]"
echo -e "}'"
echo -e "\n"

echo -e "5. Get Quiz Categories:"
echo -e "curl -s $SERVER/api/quiz/categories"
echo -e "\n"

echo -e "6. Generate Quiz Question:"
echo -e "curl -s -X POST $SERVER/api/quiz/generate \\"
echo -e "  -H \"Content-Type: application/json\" \\"
echo -e "  -d '{"
echo -e "  \"category\": \"Science\","
echo -e "  \"difficulty\": \"Medium\","
echo -e "  \"playerName\": \"Tester\""
echo -e "}'"
echo -e "\n"

echo -e "===== Test Command Examples End ======\n"
