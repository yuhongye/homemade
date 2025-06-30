from DeZero.steps.step18 import Config
from step01 import Variable
import numpy as np

import weakref

class Function:
    """
    Function 是父类，实现所有函数通用的功能；
    具体函数是在继承了Function类的类中实现的
    """

    @staticmethod
    def as_array(x):
        if np.isscalar(x):
            return np.array(x)
        return x

    @staticmethod
    def as_variable(obj):
        if isinstance(obj, Variable):
            return obj
        return Variable(obj)

    def __call__(self, *inputs):
        inputs = [Function.as_variable(x) for x in inputs]

        xs = [x.data for x in inputs]
        ys = self.forward(*xs) # 使用 * 解包
        if not isinstance(ys, tuple):
            ys = (ys, )
        outputs = [Variable(Function.as_array(y)) for y in ys]

        if Config.enable_backprop:
            # 有多个变量时，取其中最大的generation
            self.generation = max([x.generation for x in inputs])
            for output in outputs:
                output.set_creator(self) # 让输出变量保存创造者信息

            self.inputs = inputs # 保存输入的变量
            self.outputs = [weakref.ref(output) for output in outputs]# 保存输出结果

        return outputs if len(outputs) > 1 else outputs[0] # 如果列表中只有一个元素，则返回第一个元素



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

    def __str__(self):
        return self.__class__.__name__ + "(" + str(self.inputs) + "): " + str(self.outputs)


class Square(Function):
    def forward(self, x):
        return x ** 2

    def backward(self, gy):
        x = self.inputs[0].data
        gx = 2 * x * gy  # 2*x 是导数
        return gx

if __name__ == '__main__':
    x = Variable(np.array(10))
    f = Square()
    y = f(x)
    print(type(y))
    print(y.data)