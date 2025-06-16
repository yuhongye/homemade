from math import sin, cos

def newton(f, init):
    epsilon = 0.001
    def diff(f):
        return lambda x: (f(x + epsilon) - f(x)) / epsilon

    df = diff(f)

    def improve(x1):
        return x1 - f(x1)/df(x1)

    x1 = init
    while abs(f(x1)) > 1e-6:
        x1 = improve(x1)

    return x1

print("A root of sin:", newton(sin, 1.0))
print("A root of cos:", newton(cos, 10))

