import numpy as np

from DeZero.steps.step01 import Variable
from DeZero.steps.step02 import Square
from DeZero.steps.step03 import Exp
from DeZero.steps.step11 import Add


def square(x):
    return Square()(x)

def exp(x):
    return Exp()(x)

def add(x0, x1):
    return Add()(x0, x1)

if __name__ == '__main__':
    x = Variable(np.array(0.5))
    a = square(x)
    b = exp(a)
    y = square(b)

    y.backward()
    print(x.grad)

    z = Variable(0.5)