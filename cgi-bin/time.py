#!/usr/bin/python3
import time
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>Current Time</h1>")
print("<p>The current time is:", time.strftime("%Y-%m-%d %H:%M:%S"), "</p>")
print("</body></html>")