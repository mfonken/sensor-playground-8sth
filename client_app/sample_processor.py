import queue
import time
import json
import threading
import datetime 
import os


class SampleProcessor:
    def __init__(self, config):
        self.config = config    
        self.max_len = self.config.get('max_len', 100)
        self.msg_queue = queue.Queue(maxsize=self.max_len)
        self.config = config if config is not None else {}
        if 'save_to_file' in self.config:
            current_time = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
            self.save_file = self.config['save_to_file']
            self.save_file_basename, self.save_file_extension = os.path.splitext(os.path.basename(self.save_file))
            self.save_file = os.path.join(os.path.dirname(self.save_file), f"{self.save_file_basename}_{current_time}{self.save_file_extension}")
            with open(self.save_file, 'w') as f:
                f.write('')
        if 'graph' in self.config:
            from accel_plotter import AccelPlotter
            self.grapher = AccelPlotter(window_seconds=self.config['graph']['window_seconds'])
        else:
            self.grapher = None

    def on_sample(self, sample_data):
        self.msg_queue.put((time.time(), sample_data))

    def start(self):
        self.thread = threading.Thread(target=self.process, daemon=True)
        self.thread.start()

    def sink(self, sample):
        if 'save_to_file' in self.config and self.save_file:
            self.sink_file(sample)
        if 'graph' in self.config and self.grapher:
            self.sink_graph(sample)

    def sink_file(self, sample):
        try:
            with open(self.save_file, 'a') as f:
                sample_json = json.dumps(sample)
                f.write(sample_json + '\n')
        except Exception as e:
            print(f"Error writing to file: {e}")

    def sink_graph(self, sample):
        try:
            self.grapher.add_sample(sample)
        except Exception as e:
            print(f"Error adding sample to graph: {e}")

    def process(self):
        while True:
            try:
                queue_item = self.msg_queue.get(timeout=0.01) 
            except queue.Empty:
                continue
            timestamp, sample_string = queue_item
            try:
                if 'sample_count' in sample_string or '~' in sample_string:
                    print("STATUS:")
                    print(sample_string)
                    continue
                sample = json.loads(sample_string)
                sample['timestamp'] = timestamp
                self.sink(sample)
            except Exception as e:
                print(f"Error processing sample: {e}")
                