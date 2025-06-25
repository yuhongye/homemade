import numpy as np

from DeZero.steps.step01 import Variable
from DeZero.steps.step02 import Square
from DeZero.steps.step03 import Exp

A = Square()
B = Exp()
C = Square()

x = Variable(np.array(0.5))
a = A(x)
b = B(a)
y = C(b)

y.grad = np.array(1.0) # dy/dy, y对自身求导是1
b.grad = C.backward(y.grad)
a.grad = B.backward(b.grad)
x.grad = A.backward(a.grad)
print(x.grad)