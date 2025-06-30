import numpy as np

from DeZero.steps.step01 import Variable
from DeZero.steps.step07_1 import square
from DeZero.steps.step07_1 import add
from DeZero.steps.step18 import Config

import contextlib

# 添加了装饰器@contextlib.contextmanager后，就可以创建一个判断上下文的函数了
# 在函数中yield之前是预处理的代码， yield之后是后处理
# 配置 with 使用时，当处理进入with块作用域时，预处理被调用；当离开with块作用域时，后处理会被调用
@contextlib.contextmanager
def using_config(name, value):
    old_value = getattr(Config, name)
    setattr(Config, name, value)
    try:
        yield
    finally:
        setattr(Config, name, old_value)

def no_grad():
    return using_config("enable_backprop", False)

if __name__ == '__main__':
    x = Variable(np.array(2.0))
    a = square(x)
    y = add(square(a), square(a))

    y.backward()
    print(y.data)
    print(x.grad)

    with no_grad():
        x = Variable(np.ones((100, 100, 100)))
        y = square(x)