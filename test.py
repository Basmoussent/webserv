import requests

large_data = '123'
        
response = requests.post(f"http://127.0.0.1:8002/upload", 
							data=large_data, 
							timeout=10)

print(response.status_code)
print(response.text)