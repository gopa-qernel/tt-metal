import sys
sys.path.append('tinyrv')

import tinyrv
from platform.platform import VirtualPlatform
from llama7b.baremetal_kernel import llama7b_attention
import numpy as np


def main():
    # Instantiate platform
    platform = VirtualPlatform(num_clusters=4, cores_per_cluster=8, mem_size=2**20)

    # Simulate loading a Llama 7B kernel (stub)
    matrix = np.random.randn(4096, 4096).astype(np.float32)
    vector = np.random.randn(4096).astype(np.float32)
    result = platform.mv_unit.matvec(matrix, vector)
    print("Matrix-vector result (simulating Llama 7B attention):", result[:10])

    # Optionally, run the RISC-V clusters (if you have a compiled binary)
    # platform.load_program(open('llama7b.elf', 'rb').read())
    # platform.run(steps=10000)
    # print(f"Total cycles for all clusters: {platform.total_cycles()}")

    # For demonstration, run the clusters for a small number of steps
    platform.run(steps=100)
    print(f"Total cycles for all clusters: {platform.total_cycles()}")

if __name__ == "__main__":
    main()