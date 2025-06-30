import numpy as np

from DeZero.steps.step01 import Variable
from DeZero.steps.step07_1 import add

if __name__ == '__main__':
    x = Variable(np.array(3.0))
    y = add(x, x)
    y.backward()
    print(x.grad)

    # 一次全新的计算
    x.cleargrad()
    y = add(add(x, x), x)
    y.backward()
    print(x.grad)