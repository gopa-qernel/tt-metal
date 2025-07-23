import numpy as np

class Memory:
    def __init__(self, size):
        self.data = np.zeros(size, dtype=np.float32)

    def read(self, addr, size):
        return self.data[addr:addr+size]

    def write(self, addr, values):
        self.data[addr:addr+len(values)] = values