#!/bin/bash

echo "Content-Type: text/html"
echo ""
echo "<html>"
echo "<head><title>Infinite Loop</title></head>"
echo "<body>"
echo "<h1>Infinite Loop Started</h1>"
echo "<p>This script will run indefinitely...</p>"
echo "<pre>"

counter=0
while true; do
    echo "Loop iteration: $counter - $(date)"
    counter=$((counter + 1))
    sleep 1
done

echo "</pre>"
echo "</body>"
echo "</html>"
