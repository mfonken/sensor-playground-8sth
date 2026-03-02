from tcp_client import TCPClient
from sample_processor import SampleProcessor
import time
import threading

def run_client(callback):
    listener = TCPClient("127.0.0.1", 12345, on_sample_callback=callback)
    while True:
        try:
            listener.running = False
            listener.thread = None
            thread = listener.start()
            thread.start()
            thread.join()
        except Exception as e:
            print(e)
        time.sleep(0.1)

def main():
    config = {
        'max_len': 10,
        'save_to_file': 'client_app/output/samples.txt',
        'graph': {
            'window_seconds': 5
        }
    }
    processor = SampleProcessor(config)
    processor.start()
    client_thread = threading.Thread(target=run_client, args=(processor.on_sample,), daemon=True)
    client_thread.start()

    if processor.grapher == None:
        client_thread.join()
    else:
        processor.grapher.run()

if __name__ == "__main__":
    main()