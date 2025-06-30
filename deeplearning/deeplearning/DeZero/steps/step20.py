import numpy as np

from DeZero.steps.step01 import Variable
from DeZero.steps.step02 import Function
from DeZero.steps.step07_1 import add


class Mul(Function):
    def forward(self, x0, x1):
        y = x0 * x1
        return y

    def backward(self, gy):
        x0, x1 = self.inputs[0].data, self.inputs[1].data
        return gy * x1, gy * x0


def mul(x0, x1):
    return Mul()(x0, x1)

if __name__ == '__main__':
    a = Variable(np.array(3.0))
    b = Variable(np.array(2.0))
    c = Variable(np.array(1.0))

    # y = add(mul(a, b), c)
    # y.backward()
    # print(y)
    # print(a.grad)
    # print(b.grad)

    Variable.__mul__ = mul
    Variable.__add__ = add
    t = a + b
    print(type(t), type(c))
    y = t + c
    y.backward()
    print(y)
    print(a.grad)
    print(b.grad)
