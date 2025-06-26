from numpy.ctypeslib import as_array

from step01 import Variable
import numpy as np

class Function:
    """
    Function 是父类，实现所有函数通用的功能；
    具体函数是在继承了Function类的类中实现的
    """
    def __call__(self, input):
        self.input = input # 保存输入的变量
        x = input.data # input 是 Variable 类型
        y = self.forward(x)
        output = Variable(as_array(y))
        output.set_creator(self) # 让输出变量保存创造者信息
        self.output = output # 保存输出结果
        return output

    def as_array(x):
        if np.isscalar(x):
            return np.array(x)
        return x

    def forward(self, x):
        raise NotImplementedError()

    def backward(self, gy):
        """
        Parameters
        ----------
        gy

        Returns 函数的导数
        -------

        """
        raise NotImplementedError()


class Square(Function):
    def forward(self, x):
        return x ** 2

    def backward(self, gy):
        x = self.input.data
        gx = 2 * x * gy  # 2*x 是导数
        return gx

if __name__ == '__main__':
    x = Variable(np.array(10))
    f = Square()
    y = f(x)
    print(type(y))
    print(y.data)