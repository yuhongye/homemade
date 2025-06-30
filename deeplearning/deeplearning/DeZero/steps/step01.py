import numpy as np

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

if __name__ == '__main__':
    data = np.array(1.0)
    x = Variable(data)
    tuple = (x, )
    print(tuple)
    print(x)
    print(x.data)

    x.data = np.array(2.0)
    print(x.data)