import numpy as np

from DeZero.steps.step01 import Variable
from DeZero.steps.step07_1 import square, add

x = Variable(np.array(2.0))
y = Variable(np.array(3.0))

z = add(square(x), square(y))
z.backward()
print(z.data)
print(x.grad)
print(y.grad)
