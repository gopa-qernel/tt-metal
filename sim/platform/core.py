import sys
sys.path.append('../tinyrv')
import tinyrv

class RISCVCore:
    def __init__(self, core_id, memory):
        self.core_id = core_id
        self.memory = memory
        self.iss = tinyrv.sim(xlen=32)
        self.pc = 0
        self.cycles = 0

    def load_program(self, binary, addr=0):
        self.iss.copy_in(addr, binary)
        self.pc = addr

    def step(self):
        self.iss.step()
        self.pc = self.iss.pc
        self.cycles += 1

    def run(self, steps=1000):
        for _ in range(steps):
            self.step()