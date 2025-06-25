import numpy as np

from DeZero.steps.step01 import Variable
from DeZero.steps.step02 import Square
from DeZero.steps.step03 import Exp


def numerial_diff(f, x, eps=1e-4):
    """
    数值微分
    Parameters
    ----------
    f: Function
    x: Variable
    eps: 极小值

    Returns f 在 x 处的导数
    -------
    """
    x0 = Variable(x.data - eps)
    x1 = Variable(x.data + eps)
    y0 = f(x0)
    y1 = f(x1)
    return (y1.data - y0.data) / (2 * eps)

if __name__ == '__main__':
    f = Square() # x^2
    x = Variable(np.array(2.0))
    dy = numerial_diff(f, x)
    print(dy)

    def cf(x): # 复合函数
        A = Square()
        B = Exp()
        C = Square()
        return C(B(A(x)))

    x = Variable(np.array(0.5))
    dy = numerial_diff(cf, x)
    print(dy)
