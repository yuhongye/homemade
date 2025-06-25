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

assert y.creator == C
assert C.output == y
assert y.creator.input == b
assert y.creator.input.creator == B
assert y.creator.input.creator.input == a
assert y.creator.input.creator.input.creator == A
assert y.creator.input.creator.input.creator.input == x

# 反向传播
y.grad = np.array(1.0) # y对自身求导是1
y.backward()
print(x.grad)