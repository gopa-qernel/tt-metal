import numpy as np

class MatrixVectorUnit:
    def __init__(self, shape):
        self.shape = shape

    def matvec(self, matrix, vector):
        return np.dot(matrix, vector)