#!/usr/bin/env python3
"""
Testeur complet pour le projet webserv de 42
Teste toutes les edge cases possibles basées sur le fichier de configuration
"""

import os
import sys
import socket
import time
import threading
import subprocess
import requests
import tempfile
import shutil
from pathlib import Path
from typing import List, Dict, Any, Optional
import re
import json

# Couleurs ANSI pour l'affichage
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    RESET = '\033[0m'
    
    @staticmethod
    def colorize(text: str, color: str) -> str:
        return f"{color}{text}{Colors.RESET}"

class WebservTester:
    def __init__(self, config_file: str, webserv_binary: str = "./webserv"):
        self.config_file = config_file
        self.webserv_binary = webserv_binary
        self.webserv_process = None
        self.test_results = []
        self.server_config = self.parse_config()
        self.base_url = f"http://{self.server_config.get('host', '127.0.0.1')}:{self.server_config.get('port', 8002)}"
        
    def parse_config(self) -> Dict[str, Any]:
        """Parse le fichier de configuration webserv"""
        config = {
            'port': 8002,
            'host': '127.0.0.1',
            'server_name': 'localhost',
            'root': 'docs/fusion_web/',
            'index': 'index.html',
            'error_pages': {},
            'locations': {}
        }
        
        try:
            with open(self.config_file, 'r') as f:
                content = f.read()
                
            # Parse port
            port_match = re.search(r'listen\s+(\d+);', content)
            if port_match:
                config['port'] = int(port_match.group(1))
                
            # Parse host
            host_match = re.search(r'host\s+([^;]+);', content)
            if host_match:
                config['host'] = host_match.group(1).strip()
                
            # Parse server_name
            server_name_match = re.search(r'server_name\s+([^;]+);', content)
            if server_name_match:
                config['server_name'] = server_name_match.group(1).strip()
                
            # Parse root
            root_match = re.search(r'root\s+([^;]+);', content)
            if root_match:
                config['root'] = root_match.group(1).strip()
                
            # Parse index
            index_match = re.search(r'index\s+([^;]+);', content)
            if index_match:
                config['index'] = index_match.group(1).strip()
                
            # Parse error pages
            error_page_matches = re.findall(r'error_page\s+(\d+)\s+([^;]+);', content)
            for code, page in error_page_matches:
                config['error_pages'][int(code)] = page.strip()
                
            # Parse locations
            location_pattern = r'location\s+([^{]+)\s*\{([^}]*)\}'
            location_matches = re.findall(location_pattern, content, re.DOTALL)
            
            for path, location_content in location_matches:
                path = path.strip()
                location_config = {}
                
                # Parse allow_methods
                methods_match = re.search(r'allow_methods\s+([^;]+);', location_content)
                if methods_match:
                    location_config['allow_methods'] = methods_match.group(1).strip().split()
                
                # Parse autoindex
                autoindex_match = re.search(r'autoindex\s+(on|off);', location_content)
                if autoindex_match:
                    location_config['autoindex'] = autoindex_match.group(1) == 'on'
                    
                # Parse CGI settings
                cgi_path_match = re.search(r'cgi_path\s+([^;]+);', location_content)
                if cgi_path_match:
                    location_config['cgi_path'] = cgi_path_match.group(1).strip().split()
                    
                cgi_ext_match = re.search(r'cgi_ext\s+([^;]+);', location_content)
                if cgi_ext_match:
                    location_config['cgi_ext'] = cgi_ext_match.group(1).strip().split()
                    
                # Parse index for location
                loc_index_match = re.search(r'index\s+([^;]+);', location_content)
                if loc_index_match:
                    location_config['index'] = loc_index_match.group(1).strip()
                
                config['locations'][path] = location_config
                
        except Exception as e:
            print(f"Erreur lors du parsing de la configuration: {e}")
            
        return config
    
    def log_test(self, test_name: str, passed: bool, details: str = ""):
        """Enregistre le résultat d'un test"""
        result = {
            'test': test_name,
            'passed': passed,
            'details': details
        }
        self.test_results.append(result)
        if passed:
            status = Colors.colorize("✓ PASS", Colors.GREEN + Colors.BOLD)
            print(f"{status}: {Colors.colorize(test_name, Colors.WHITE)}")
        else:
            status = Colors.colorize("✗ FAIL", Colors.RED + Colors.BOLD)
            print(f"{status}: {Colors.colorize(test_name, Colors.WHITE)}")
            if details:
                print(f"   {Colors.colorize('Details:', Colors.YELLOW)} {Colors.colorize(details, Colors.RED)}")
    
    def start_webserv(self) -> bool:
        """Démarre le serveur webserv"""
        try:
            self.webserv_process = subprocess.Popen(
                [self.webserv_binary, self.config_file],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            
            time.sleep(2)
            
            if self.webserv_process.poll() is None:
                return True
            else:
                stdout, stderr = self.webserv_process.communicate()
                print(f"{Colors.colorize('Webserv failed to start:', Colors.RED)} {Colors.colorize(stderr.decode(), Colors.YELLOW)}")
                return False
                
        except Exception as e:
            print(f"{Colors.colorize('Erreur lors du démarrage de webserv:', Colors.RED)} {Colors.colorize(str(e), Colors.YELLOW)}")
            return False
    
    def stop_webserv(self):
        """Arrête le serveur webserv"""
        if self.webserv_process:
            self.webserv_process.terminate()
            self.webserv_process.wait()
    
    def is_server_running(self) -> bool:
        """Vérifie si le serveur répond"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1)
            result = sock.connect_ex((self.server_config['host'], self.server_config['port']))
            sock.close()
            return result == 0
        except:
            return False
    
    def test_server_startup(self):
        """Test de démarrage du serveur"""
        server_started = self.start_webserv()
        self.log_test("Server Startup", server_started, 
                     "Server failed to start" if not server_started else "")
        
        if server_started:
            server_responsive = self.is_server_running()
            self.log_test("Server Responsive", server_responsive,
                         "Server not responding on configured port" if not server_responsive else "")
    
    def test_basic_http_methods(self):
        """Test des méthodes HTTP de base"""
        methods_to_test = ['GET', 'POST', 'DELETE']
        
        for method in methods_to_test:
            try:
                response = requests.request(method, self.base_url, timeout=5)
                # Un serveur web doit au moins répondre (même avec une erreur)
                self.log_test(f"HTTP {method} Response", 
                            response.status_code is not None,
                            f"Status: {response.status_code}")
            except Exception as e:
                self.log_test(f"HTTP {method} Response", False, str(e))
    
    def test_location_methods(self):
        """Test des méthodes autorisées par location"""
        for location_path, location_config in self.server_config['locations'].items():
            allowed_methods = location_config.get('allow_methods', [])
            
            if not allowed_methods:
                continue
                
            url = f"{self.base_url}{location_path}"
            
            # Test des méthodes autorisées
            for method in allowed_methods:
                try:
                    print(method, url)
                    response = requests.request(method, url, timeout=5)
                    # Méthode autorisée ne devrait pas retourner 405
                    passed = response.status_code != 405
                    self.log_test(f"Location {location_path} - {method} allowed",
                                passed, f"Status: {response.status_code}")
                except Exception as e:
                    self.log_test(f"Location {location_path} - {method} allowed", 
                                False, str(e))
            
            # Test des méthodes non autorisées
            forbidden_methods = ['GET', 'POST', 'DELETE']
            for method in forbidden_methods:
                if method not in allowed_methods:
                    try:
                        
                        response = requests.request(method, url, timeout=5)
                        # Méthode non autorisée devrait retourner 405
                        passed = response.status_code == 405 or response.status_code == 501
                        self.log_test(f"Location {location_path} - {method} forbidden",
                                    passed, f"Status: {response.status_code}")
                    except Exception as e:
                        self.log_test(f"Location {location_path} - {method} forbidden", 
                                    False, str(e))
    
    def test_error_pages(self):
        """Test des pages d'erreur personnalisées"""
        # Test 404
        try:
            response = requests.get(f"{self.base_url}/nonexistent", timeout=5)
            if 404 in self.server_config['error_pages']:
                # Vérifier si la page d'erreur personnalisée est servie
                expected_error_page = self.server_config['error_pages'][404]
                self.log_test("Custom 404 Error Page", 
                            response.status_code == 404,
                            f"Status: {response.status_code}")
            else:
                self.log_test("Default 404 Error", 
                            response.status_code == 404,
                            f"Status: {response.status_code}")
        except Exception as e:
            self.log_test("404 Error Handling", False, str(e))
    
    def test_large_request_body(self):
        """Test des requêtes avec corps volumineux"""
        large_data = 'x' * (5 * 1024 * 1024)  # 5MB
        
        try:
            response = requests.post(f"{self.base_url}/upload", 
                                   data=large_data, 
                                   timeout=10)
            # Le serveur devrait gérer ou rejeter proprement
            self.log_test("Large Request Body", 
                        response.status_code in [200, 201, 413, 500],
                        f"Status: {response.status_code}")
        except Exception as e:
            self.log_test("Large Request Body", False, str(e))
    
    def test_malformed_requests(self):
        """Test des requêtes malformées"""
        malformed_requests = [
            "GET / HTTP/1.1\r\n\r\n",  # Sans Host header
            "INVALID_METHOD / HTTP/1.1\r\nHost: localhost\r\n\r\n",
            "GET / HTTP/9.9\r\nHost: localhost\r\n\r\n",  # Version HTTP invalide
            "GET /../../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n",  # Path traversal
        ]
        
        for i, req in enumerate(malformed_requests):
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                sock.connect((self.server_config['host'], self.server_config['port']))
                sock.send(req.encode())
                response = sock.recv(1024)
                sock.close()
                
                self.log_test(f"Malformed Request {i+1}", 
                            b'400' in response or b'501' in response or b'404' in response,
                            f"Response: {response[:100]}")
            except Exception as e:
                self.log_test(f"Malformed Request {i+1}", False, str(e))
    
    def test_concurrent_requests(self):
        """Test des requêtes concurrentes"""
        def make_request():
            try:
                response = requests.get(self.base_url, timeout=5)
                return response.status_code
            except:
                return None
        
        threads = []
        results = []
        
        # Lancer 20 requêtes simultanées
        for _ in range(20):
            thread = threading.Thread(target=lambda: results.append(make_request()))
            threads.append(thread)
            thread.start()
        
        for thread in threads:
            thread.join()
        
        successful_requests = sum(1 for r in results if r is not None)
        self.log_test("Concurrent Requests", 
                    successful_requests >= 15,  # Au moins 75% de succès
                    f"Successful: {successful_requests}/20")
    
    def test_cgi_execution(self):
        """Test de l'exécution CGI"""
        cgi_location = None
        for path, config in self.server_config['locations'].items():
            if 'cgi_path' in config:
                cgi_location = path
                break
        
        if not cgi_location:
            self.log_test("CGI Execution", True, "No CGI configured - skipped")
            return
        
        # Test d'une requête CGI
        try:
            cgi_url = f"{self.base_url}{cgi_location}"
            headers = {
                "Content-Type": "text/plain"
            }
            response = requests.get(cgi_url, headers=headers ,data="time.py" , timeout=10)
            
            self.log_test("CGI Execution", 
                        response.status_code in [200, 500, 502],
                        f"Status: {response.status_code}")
        except Exception as e:
            self.log_test("CGI Execution", False, str(e))
    
    def test_autoindex(self):
        """Test de l'autoindex"""
        for path, config in self.server_config['locations'].items():
            if config.get('autoindex'):
                try:
                    response = requests.get(f"{self.base_url}{path}", timeout=5)
                    # Autoindex devrait lister les fichiers
                    self.log_test(f"Autoindex {path}", 
                                response.status_code == 200,
                                f"Status: {response.status_code}")
                except Exception as e:
                    self.log_test(f"Autoindex {path}", False, str(e))
    
    def test_edge_cases(self):
        """Test des cas limites spécifiques"""
        edge_cases = [
            # URLs avec caractères spéciaux
            ("/test%20file", "URL with space encoding"),
            ("/test?param=value", "URL with query string"),
            ("/test#fragment", "URL with fragment"),
            ("//double//slash", "Double slash in path"),
            ("/./current/dir", "Current directory in path"),
            ("/../parent/dir", "Parent directory in path"),
            ("/very/long/path/that/might/cause/buffer/overflow/issues/with/some/implementations", "Very long path"),
        ]
        
        for path, description in edge_cases:
            try:
                response = requests.get(f"{self.base_url}{path}", timeout=5)
                # Le serveur devrait gérer sans crasher
                self.log_test(f"Edge Case: {description}", 
                            response.status_code is not None,
                            f"Status: {response.status_code}")
            except Exception as e:
                self.log_test(f"Edge Case: {description}", False, str(e))
    
    def test_http_headers(self):
        """Test des headers HTTP"""
        headers_tests = [
            ({}, "No additional headers"),
            ({"Connection": "close"}, "Connection close"),
            ({"Connection": "keep-alive"}, "Connection keep-alive"),
            ({"Accept": "*/*"}, "Accept all"),
            ({"User-Agent": "TestBot/1.0"}, "Custom User-Agent"),
            ({"Content-Type": "application/json"}, "JSON Content-Type"),
            ({"Authorization": "Bearer token123"}, "Authorization header"),
        ]
        
        for headers, description in headers_tests:
            try:
                response = requests.get(self.base_url, headers=headers, timeout=5)
                self.log_test(f"HTTP Headers: {description}", 
                            response.status_code is not None,
                            f"Status: {response.status_code}")
            except Exception as e:
                self.log_test(f"HTTP Headers: {description}", False, str(e))
    
    def test_stress_conditions(self):
        """Test des conditions de stress"""
        # Test avec beaucoup de headers
        many_headers = {f"X-Test-Header-{i}": f"value{i}" for i in range(100)}
        try:
            response = requests.get(self.base_url, headers=many_headers, timeout=5)
            self.log_test("Many Headers", 
                        response.status_code is not None,
                        f"Status: {response.status_code}")
        except Exception as e:
            self.log_test("Many Headers", False, str(e))
        
        # Test avec URL très longue
        long_url = self.base_url + "/" + "a" * 2000
        try:
            response = requests.get(long_url, timeout=5)
            self.log_test("Very Long URL", 
                        response.status_code in [200, 404, 414, 500],
                        f"Status: {response.status_code}")
        except Exception as e:
            self.log_test("Very Long URL", False, str(e))
    
    def run_all_tests(self):
        """Lance tous les tests"""
        print(f"{Colors.colorize('=== WEBSERV TESTER - Démarrage des tests ===', Colors.MAGENTA + Colors.BOLD)}\n")
        print(f"{Colors.colorize('Configuration:', Colors.BLUE)} {Colors.colorize(self.config_file, Colors.WHITE)}")
        print(f"{Colors.colorize('Serveur:', Colors.BLUE)} {Colors.colorize(self.base_url, Colors.WHITE)}")
        print(f"{Colors.colorize('Binaire:', Colors.BLUE)} {Colors.colorize(self.webserv_binary, Colors.WHITE)}\n")
        
        # Tests de base
        self.test_server_startup()
        
        if not self.is_server_running():
            print(f"{Colors.colorize('❌ Le serveur ne répond pas. Arrêt des tests.', Colors.RED + Colors.BOLD)}")
            return
        
        # Tests fonctionnels
        print(f"\n{Colors.colorize('--- Tests des méthodes HTTP ---', Colors.CYAN + Colors.BOLD)}")
        self.test_basic_http_methods()
        self.test_location_methods()
        
        msg = "--- Tests des pages d'erreur ---"
        print(f'\n{Colors.colorize(msg, Colors.CYAN + Colors.BOLD)}')
        self.test_error_pages()
        
        print(f"\n{Colors.colorize('--- Tests des fonctionnalités ---', Colors.CYAN + Colors.BOLD)}")
        self.test_cgi_execution()
        self.test_autoindex()
        
        print(f"\n{Colors.colorize('--- Tests des cas limites ---', Colors.CYAN + Colors.BOLD)}")
        self.test_edge_cases()
        self.test_malformed_requests()
        
        print(f"\n{Colors.colorize('--- Tests de performance ---', Colors.CYAN + Colors.BOLD)}")
        self.test_concurrent_requests()
        self.test_large_request_body()
        
        print(f"\n{Colors.colorize('--- Tests de stress ---', Colors.CYAN + Colors.BOLD)}")
        self.test_http_headers()
        self.test_stress_conditions()
        
        # Arrêt du serveur
        self.stop_webserv()
        
        # Résumé
        self.print_summary()
    
    def print_summary(self):
        """Affiche le résumé des tests"""
        print(f"\n{Colors.colorize('='*60, Colors.MAGENTA)}")
        print(f"{Colors.colorize('RÉSUMÉ DES TESTS', Colors.MAGENTA + Colors.BOLD)}")
        print(f"{Colors.colorize('='*60, Colors.MAGENTA)}")
        
        total_tests = len(self.test_results)
        passed_tests = sum(1 for r in self.test_results if r['passed'])
        failed_tests = total_tests - passed_tests
        
        print(f"{Colors.colorize('Total des tests:', Colors.BLUE)} {Colors.colorize(str(total_tests), Colors.WHITE + Colors.BOLD)}")
        print(f"{Colors.colorize('Tests réussis:', Colors.GREEN)} {Colors.colorize(str(passed_tests), Colors.GREEN + Colors.BOLD)}")
        print(f"{Colors.colorize('Tests échoués:', Colors.RED)} {Colors.colorize(str(failed_tests), Colors.RED + Colors.BOLD)}")
        
        success_rate = (passed_tests/total_tests)*100
        rate_color = Colors.GREEN if success_rate >= 80 else Colors.YELLOW if success_rate >= 60 else Colors.RED
        print(f"{Colors.colorize('Taux de réussite:', Colors.BLUE)} {Colors.colorize(f'{success_rate:.1f}%', rate_color + Colors.BOLD)}")
        
        if failed_tests > 0:
            print(f"\n{Colors.colorize(f'❌ TESTS ÉCHOUÉS ({failed_tests}):', Colors.RED + Colors.BOLD)}")
            for result in self.test_results:
                if not result['passed']:
                    print(f"  {Colors.colorize('- ' + result['test'] + ':', Colors.WHITE)} {Colors.colorize(result['details'], Colors.YELLOW)}")
        else:
            print(f"\n{Colors.colorize('🎉 TOUS LES TESTS ONT RÉUSSI !', Colors.GREEN + Colors.BOLD)}")
        
        print(f"\n{Colors.colorize('='*60, Colors.MAGENTA)}")
        
        # Analyse des erreurs potentielles
        self.analyze_potential_errors()
    
    def analyze_potential_errors(self):
        """Analyse les erreurs potentielles dans l'implémentation"""
        print(f"{Colors.colorize('ANALYSE DES ERREURS POTENTIELLES:', Colors.CYAN + Colors.BOLD)}")
        print(f"{Colors.colorize('-' * 40, Colors.CYAN)}")
        
        error_categories = {
            'HTTP Methods': [r for r in self.test_results if 'HTTP' in r['test'] and not r['passed']],
            'Location Handling': [r for r in self.test_results if 'Location' in r['test'] and not r['passed']],
            'Error Pages': [r for r in self.test_results if 'Error' in r['test'] and not r['passed']],
            'CGI': [r for r in self.test_results if 'CGI' in r['test'] and not r['passed']],
            'Edge Cases': [r for r in self.test_results if 'Edge Case' in r['test'] and not r['passed']],
            'Performance': [r for r in self.test_results if any(x in r['test'] for x in ['Concurrent', 'Large', 'Stress']) and not r['passed']],
        }
        
        has_errors = False
        for category, failures in error_categories.items():
            if failures:
                has_errors = True
                print(f"\n{Colors.colorize(f'🔍 {category}:', Colors.YELLOW + Colors.BOLD)}")
                for failure in failures:
                    print(f"   {Colors.colorize('- ' + failure['test'], Colors.WHITE)}")
                    if failure['details']:
                        print(f"     {Colors.colorize('Détails:', Colors.BLUE)} {Colors.colorize(failure['details'], Colors.YELLOW)}")
        
        if not has_errors:
            print(f"\n{Colors.colorize('✨ Aucune erreur majeure détectée !', Colors.GREEN + Colors.BOLD)}")
            print(f"{Colors.colorize('Votre implémentation webserv semble robuste.', Colors.GREEN)}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 webserv_tester.py <config_file> [webserv_binary]")
        sys.exit(1)
    
    config_file = sys.argv[1]
    webserv_binary = sys.argv[2] if len(sys.argv) > 2 else "./webserv"
    
    if not os.path.exists(config_file):
        print(f"❌ Fichier de configuration introuvable: {config_file}")
        sys.exit(1)
    
    if not os.path.exists(webserv_binary):
        print(f"❌ Binaire webserv introuvable: {webserv_binary}")
        sys.exit(1)
    
    tester = WebservTester(config_file, webserv_binary)
    tester.run_all_tests()

if __name__ == "__main__":
    main()