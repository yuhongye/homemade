x = 2
y = 3
z = 4

def func1(x):
    y = x ** 2 + z
    return z ** 2 - y

print(func1(x + 2*y + 3*z))

def func2():
    global x
    y = x
    x = 2 # 赋值即定义，会导致上一行的x被解释成局部变量，因为局部变量未定义，所以会报错; 如果把x声明未对global的引用则没问题