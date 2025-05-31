import requests
import sys

# Test with a smaller payload first (1MB)
test_size = 1024 * 1024 * 2  # 1MB
large_data = 'x' * test_size

try:
    print(f"Sending {test_size/1024/1024:.1f}MB of data...")
    response = requests.post(
        "http://127.0.0.1:8002/",
        data=large_data,
        timeout=30  # Increased timeout
    )
    print(f"Status code: {response.status_code}")
    print(f"Response: {response.text[:200]}...")  # Print first 200 chars of response
except requests.exceptions.RequestException as e:
    print(f"Error occurred: {e}", file=sys.stderr)
    sys.exit(1)