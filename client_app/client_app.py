from tcp_client import TCPClient
from sample_processor import SampleProcessor
import time


def main():
    config = {
        'max_len': 10,
        'save_to_file': 'client_app/output/samples.txt',
        'graph': {
        }
    }
    processor = SampleProcessor(config)
    processor.start()
    listener = TCPClient("127.0.0.1", 12345, on_sample_callback=processor.on_sample)
    while True:
        try:
            thread = listener.start()
            thread.start()
            thread.join()
        except Exception as e:
            print(e)
            time.sleep(0.1)

if __name__ == "__main__":
    main()