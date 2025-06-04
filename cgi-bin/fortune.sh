#!/bin/bash

echo "Content-Type: text/html"
echo ""

quotes=(
    "The only way to do great work is to love what you do. - Steve Jobs"
    "Life is what happens to you while you're busy making other plans. - John Lennon"
    "The future belongs to those who believe in the beauty of their dreams. - Eleanor Roosevelt"
    "It is during our darkest moments that we must focus to see the light. - Aristotle"
    "In the middle of every difficulty lies opportunity. - Albert Einstein"
)

random_quote=${quotes[$RANDOM % ${#quotes[@]}]}

echo "<html>"
echo "<head><title>Random Quote</title></head>"
echo "<body>"
echo "<h1>Quote of the Moment</h1>"
echo "<blockquote>$random_quote</blockquote>"
echo "<p>Generated at: $(date)</p>"
echo "</body>"
echo "</html>"
