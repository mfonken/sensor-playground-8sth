import socket
import threading
import time


class TCPClient:
    def __init__(self, host, port, on_sample_callback=None):
        self.host = host
        self.port = port
        self.on_sample_callback = on_sample_callback
        self.sock = None
        self.running = False
        self.thread = None
        
    def connection_manager(self):
        """Background thread managing connection and samples"""
        while self.running:
            try:
                if self.sock is None or not self.sock:
                    self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    self.sock.connect((self.host, self.port))
                    print(f"Connected to {self.host}:{self.port}")
                
                # Receive loop
                self.sock.settimeout(1.0)
                while self.running:
                    data = self.sock.recv(1024)
                    if not data:
                        break
                    if self.on_sample_callback:
                        data_stripped = data.decode('utf-8').strip()
                        print("Received sample:", data_stripped)
                        self.on_sample_callback(data_stripped)
                        
            except Exception as e:
                print(f"Connection error: {e}")
                if self.sock:
                    self.sock.close()
                    self.sock = None
                time.sleep(0.5)
    
    def send(self, message):
        """Send message (thread-safe)"""
        if self.sock:
            try:
                self.sock.sendall(message.encode('utf-8'))
            except:
                pass
    
    def start(self):
        """Start threaded client"""
        self.running = True
        self.thread = threading.Thread(target=self.connection_manager, daemon=True)
        return self.thread
    
    def stop(self):
        """Stop client"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)
        if self.sock:
            self.sock.close()