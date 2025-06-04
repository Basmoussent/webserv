import requests

large_data = 'x' * (5 * 1024 * 1024)
        
response = requests.post(f"http://127.0.0.1:8002/upload", 
							data=large_data, 
							timeout=10)

print(response.status_code)
print(response.text)