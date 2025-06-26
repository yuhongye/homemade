import numpy as np

class Variable:
    def __init__(self, data):
        if data is not None and not isinstance(data, np.ndarray):
            raise TypeError('{} is not supported'.format(type(data))) # 只支持ndarray

        self.data = data
        self.grad = None # 反向传播到这里的梯度
        self.creator = None # 这个变量是由哪个函数创建的，即经过哪个函数计算后得到的结果

    def set_creator(self, func):
        self.creator = func

    def backward(self):
        if self.grad is None:
            self.grad = np.ones_like(self.data) # 自动生成导数，自己对自己的导数是1

        f = self.creator # 1. 获取函数
        while f is not None:
            x, y = f.input, f.output
            x.grad = f.backward(y.grad)
            f = x.creator

if __name__ == '__main__':
    data = np.array(1.0)
    x = Variable(data)
    print(x.data)

    x.data = np.array(2.0)
    print(x.data)