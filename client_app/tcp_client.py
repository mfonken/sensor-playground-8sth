import socket
import threading
import time


class TCPClient:
    def __init__(self, host, port, on_sample_callback=None, sample_delimiter="\n"):
        self.host = host
        self.port = port
        self.on_sample_callback = on_sample_callback
        self.sample_delimiter = sample_delimiter
        self.lock = threading.Lock()
        self.sock = None
        self.running = False
        self.thread = None
        
    def connect(self):
        """Connect to server"""
        with self.lock:
            if self.sock:
                return
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            print(f"Connected to {self.host}:{self.port}")
    
    def disconnect(self):
        """Disconnect from server"""
        with self.lock:
            if self.sock:
                self.sock.close()
                self.sock = None
    
    def send(self, message):
        """Send message (thread-safe)"""
        with self.lock:
            if self.sock:
                try:
                    self.sock.sendall(message.encode('utf-8'))
                except:
                    pass
    
    def receive(self):
        """Receive data from server"""
        with self.lock:
            if self.sock:
                try:
                    data = self.sock.recv(1024)
                    if not data:
                        return
                    data_stripped = data.decode('utf-8').strip()
                    data_split = data_stripped.split(self.sample_delimiter)
                    for sample in data_split:
                        if self.on_sample_callback:
                            self.on_sample_callback(sample)
                except Exception as e:
                    pass
    
    def connection_manager(self):
        """Background thread managing connection and samples"""
        while self.running:
            try:
                self.connect()
                while self.running:
                    self.receive()
            except Exception as e:
                print(f"Connection error: {e}")
                self.disconnect()
                time.sleep(0.5)
    
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
        self.disconnect()

    def restart(self):
        self.stop()          # sets running=False, joins old thread, disconnects
        time.sleep(0.1)
        self.running = True
        self.thread = threading.Thread(target=self.connection_manager, daemon=True)
        self.thread.start()
        return self.thread
