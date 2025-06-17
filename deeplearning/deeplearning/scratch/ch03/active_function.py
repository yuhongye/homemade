import numpy as np
import matplotlib.pylab as plt

def step_function(x):
    """x: numpy array"""
    return np.array(x > 0, dtype=np.int_)

def sigmoid(x):
    """x: numpy array"""
    return 1 / (1 + np.exp(-x))

def relu(x):
    return np.maximum(0, x)

def identity_function(x):
    """x: numpy array"""
    return x

def softmax(a):
    c = np.max(a)
    exp_a = np.exp(a - c)
    sum_exp_a = np.sum(exp_a)
    y = exp_a / sum_exp_a
    return y


if __name__ == '__main__':
    x = np.arange(-5.0, 5.0, 0.1)
    # y = step_function(x)
    # y = sigmoid(x)
    y = relu(x)
    plt.plot(x, y)
    plt.ylim(-0.1, 1.1)
    plt.show()