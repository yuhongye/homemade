import numpy as np

from DeZero.steps.step01 import Variable

if __name__ == '__main__':
    x = Variable(np.array([[1,2,3], [4,5,6]]))
    print(x.shape)
    print(x.ndim)
    print(x.size)
    print(x.dtype)
    print(len(x))
    print(x)