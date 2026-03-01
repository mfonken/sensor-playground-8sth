import json 
class SensorSample:
    def __init__(self, index=None, x=None, y=None, z=None, mag=None):
        self.index = index
        self.x = x
        self.y = y
        self.z = z
        self.mag = mag

    def from_json(self, json_str):
        json_data = json.loads(json_str)
        self.index = json_data['index']
        self.x = json_data['x']
        self.y = json_data['y']
        self.z = json_data['z']
        self.mag = json_data['mag']

    def to_json(self):
        return json.dumps({
            'index': self.index,
            'x': self.x,
            'y': self.y,
            'z': self.z,
            'mag': self.mag
        })
