import contextlib
import weakref

import numpy as np


class Config:
    enable_backprop = True

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

def as_array(x):
    if np.isscalar(x):
        return np.array(x)
    return x

def as_variable(obj):
    if isinstance(obj, Variable):
        return obj
    return Variable(obj)

class Variable:
    def __init__(self, data, name=None):
        if data is not None and not isinstance(data, np.ndarray):
            raise TypeError('{} is not supported'.format(type(data))) # 只支持ndarray

        self.data = data
        self.name = name
        self.grad = None # 反向传播到这里的梯度
        self.creator = None # 这个变量是由哪个函数创建的，即经过哪个函数计算后得到的结果
        self.generation = 0

    @property
    def shape(self):
        return self.data.shape

    @property
    def ndim(self):
        return self.data.ndim

    @property
    def size(self):
        return self.data.size

    @property
    def dtype(self):
        return self.data.dtype

    def set_creator(self, func):
        self.creator = func
        self.generation = func.generation + 1

    def backward(self, retain_grad=False):
        if self.grad is None:
            self.grad = np.ones_like(self.data) # 自动生成导数，自己对自己的导数是1

        funcs = [self.creator]
        seen_set = set()
        def add_func(f):
            if f not in seen_set:
                funcs.append(f)
                seen_set.add(f)
                funcs.sort(key=lambda x: x.generation)

        while len(funcs) > 0:
            f = funcs.pop()
            print('[backward] - function:', f)
            gys = [output().grad for output in f.outputs]
            gxs = f.backward(*gys) # 反向传播，计算梯度
            if not isinstance(gxs, tuple):
                gxs= (gxs, )

            for x, gx in zip(f.inputs, gxs):
                if x.grad is None:
                    x.grad = gx
                else:
                    x.grad = x.grad + gx
                if x.creator is not None: # 输入变量， 停止反向传播
                    add_func(x.creator)

            if not retain_grad:
                for y in f.outputs:
                    y().grad = None # y是weakref


    def cleargrad(self):
        self.grad = None

    # def __str__(self):
    #     return '{{value: {}, generation: {}}}'.format(self.data, self.generation)

    def __repr__(self):
        if self.data is None:
            return 'variable(None)'

        p = str(self.data).replace('\n', '\n' + ' ' * 9)
        return 'variable(' + p + ')'

    def __len__(self):
        return len(self.data)

class Function:
    """
    Function 是父类，实现所有函数通用的功能；
    具体函数是在继承了Function类的类中实现的
    """

    def __call__(self, *inputs):
        inputs = [as_variable(x) for x in inputs]

        xs = [x.data for x in inputs]
        ys = self.forward(*xs) # 使用 * 解包
        if not isinstance(ys, tuple):
            ys = (ys, )
        outputs = [Variable(as_array(y)) for y in ys]

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

class Neg(Function):
    """ 负数 """
    def forward(self, x):
        return -x

    def backward(self, gy):
        return -gy

class Add(Function):
    def forward(self, x0, x1):
        y = x0 + x1
        return y

    def backward(self, gy):
        return gy, gy

class Sub(Function):
    def forward(self, x0, x1):
        y = x0 - x1
        return y

    def backward(self, gy):
        return gy, -gy

class Mul(Function):
    def forward(self, x0, x1):
        y = x0 * x1
        return y

    def backward(self, gy):
        x0, x1 = self.inputs[0].data, self.inputs[1].data
        return gy * x1, gy * x0

class Div(Function):
    def forward(self, x0, x1):
        y = x0 / x1
        return y

    def backward(self, gy):
        x0, x1 = self.inputs[0].data, self.inputs[1].data
        gx0 = gy / x1
        gx1 = gy * (-x0 / x1 ** 2)
        return gx0, gx1

class Pow(Function):
    def __init__(self, c):
        self.c = c
    def forward(self, x):
        y = x ** self.c
        return y
    def backward(self, gy):
        x = self.inputs[0].data
        c = self.c
        gx = c * x ** (c -1) * gy
        return gx

class Square(Function):
    def forward(self, x):
        return x ** 2

    def backward(self, gy):
        x = self.inputs[0].data
        gx = 2 * x * gy  # 2*x 是导数
        return gx

def neg(x):
    return Neg()(x)

def add(x0, x1):
    x1 = as_array(x1)
    return Add()(x0, x1)

def sub(x0, x1):
    x1 = as_array(x1)
    return Sub()(x0, x1)
def rsub(x0, x1):
    """ x1 - x0"""
    x1 = as_array(x1)
    return Sub()(x1, x0)

def mul(x0, x1):
    x1 = as_array(x1)
    return Mul()(x0, x1)

def div(x0, x1):
    x1 = as_array(x1)
    return Div()(x0, x1)
def rdiv(x0, x1):
    x1 = as_array(x1)
    return Div()(x1, x0)

def pow(x, c):
    return Pow(c)(x)

def setup_variable():
    Variable.__add__ = add
    Variable.__radd__ = add
    Variable.__mul__ = mul
    Variable.__rmul__ = mul
    Variable.__neg__ = neg
    Variable.__sub__ = sub
    Variable.__rsub__ = rsub
    Variable.__truediv__ = div
    Variable.__rtruediv__ = rdiv
    Variable.__pow__ = pow