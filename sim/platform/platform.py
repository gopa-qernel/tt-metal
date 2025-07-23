from .cluster import CoreCluster
from .matrix_vector import MatrixVectorUnit
from .memory import Memory
from .noc import NoC

class VirtualPlatform:
    def __init__(self, num_clusters, cores_per_cluster, mem_size):
        self.memory = Memory(mem_size)
        self.clusters = [CoreCluster(i, cores_per_cluster, self.memory) for i in range(num_clusters)]
        self.mv_unit = MatrixVectorUnit((4096, 4096))  # Example shape
        self.noc = NoC(self.clusters)

    def load_program(self, binary):
        for cluster in self.clusters:
            cluster.load_program(binary)

    def run(self, steps=1000):
        for cluster in self.clusters:
            cluster.run(steps)

    def total_cycles(self):
        return sum(cluster.total_cycles() for cluster in self.clusters)