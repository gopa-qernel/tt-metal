from .core import RISCVCore

class CoreCluster:
    def __init__(self, cluster_id, num_cores, memory):
        self.cores = [RISCVCore(f"{cluster_id}-{i}", memory) for i in range(num_cores)]

    def load_program(self, binary):
        for core in self.cores:
            core.load_program(binary)

    def run(self, steps=1000):
        for core in self.cores:
            core.run(steps)

    def total_cycles(self):
        return sum(core.cycles for core in self.cores)