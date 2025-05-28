# Web Server Configuration Redirects

## Syntax

### Redirect Types and Codes
| Syntax                           | Redirect Type | HTTP Code |
|----------------------------------|---------------|-----------|
| `return 301 /new;`               | Permanent     | 301       |
| `return 302 /new;`               | Temporary     | 302       |
| `rewrite ^/old$ /new;`           | Temporary (default) | 302 |
| `rewrite ^/old$ /new permanent;` | Permanent     | 301 |
| `rewrite ^/old$ /new redirect;`  | Temporary     | 302 |

#### Examples and Explanations

# path redirect
location /old-path {
    return 301 /new-path;
}

# Rewrite with pattern matching
location /old-url {
    rewrite ^/old-url$ /new-url permanent;
}

# HTTP to HTTPS
server {
    listen 80;
    server_name example.com;
    return 301 https://$server_name$request_uri;
}


## Behavior

### Client Request Flow
1. Client sends request to original URL
2. Server responds with:
   - Status code (301/302)
   - Location header with new URL

3. Client follows redirect to new URL with -L flag

### Status Codes
- 301: Permanent redirect 
- 302: Temporary redirect 


Use curl to test redirects:
```bash
# Test without following redirect
curl -v http://example.com/old

# Test with following redirect
curl -L http://example.com/old

# Get final URL
curl -Ls -o /dev/null -w "%{url_effective}" http://example.com/old
```

## Response Headers
When a redirect occurs, the server sends these headers:
```
HTTP/1.1 301 Moved Permanently
Location: /new-path
```
