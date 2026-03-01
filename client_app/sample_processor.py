import queue
import time
import json
import threading
import os
from sensor_sample import SensorSample


class SampleProcessor:
    def __init__(self, config):
        self.config = config    
        self.max_len = self.config.get('max_len', 100)
        self.msg_queue = queue.Queue(maxsize=self.max_len)
        self.config = config if config is not None else {}
        if 'save_to_file' in self.config:
            self.save_file = self.config['save_to_file']
            with open(self.save_file, 'w') as f:
                f.write('')
        if 'graph' in self.config:
            from accelerometer_grapher import AccelerometerGrapher
            self.grapher = AccelerometerGrapher(max_samples=300, title="IMU Demo — Random Walk")
            self.grapher.start()

    def on_sample(self, sample_data):
        self.msg_queue.put(sample_data)

    def start(self):
        self.thread = threading.Thread(target=self.process, daemon=True)
        self.thread.start()

    def sink(self, sample):
        if 'save_to_file' in self.config and self.save_file:
            with open(self.save_file, 'a') as f:
                f.write(sample.to_json() + '\n')

        if 'graph' in self.config and self.grapher:
            self.grapher.add_sample(sample.x, sample.y, sample.z, sample.mag)
            # self.grapher.update(sample.index, sample.x, sample.y, sample.z, sample.mag)

    def process(self):
        sample = SensorSample()
        while True:
            if self.msg_queue.empty():
                time.sleep(0.01)

            sample_data = self.msg_queue.get()
            sample.from_json(sample_data)
            self.sink(sample)

