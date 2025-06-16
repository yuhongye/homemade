from mimetypes import guess_type


def cbrt(x):
    def not_enough():
        # 重点关注：可以直接使用外部函数的局部变量
        return abs((guess ** 3 - x) / x) > 1e-6

    def improve():
        return (2.0 * guess + x / guess / guess) / 3

    if x == 0.0:
        return 0.0

    guess = x
    while not_enough():
        guess = improve()
    return guess


# 把【逼近】这一概念引入编程环境后，就可以抽象出一套框架，被任何【逼近】类计算所复用
def appr_method(x, not_enough, improve):
    if x == 0.0:
        return 0.0

    guess = x
    while not_enough(x, guess):
        guess = improve(x, guess)
    return guess

# 求立方根就可以直接复用了
def cbrt_not_enough(x, guess):
    return abs((guess ** 3 - x) / x) > 1e-6

def cbrt_improve(x, guess):
    return (2.0 * guess + x / guess / guess) / 3

def cbrt2(x):
    return appr_method(x, cbrt_not_enough, cbrt_improve)

print("8的立方根:", cbrt(8))
print("11的立方根:", cbrt(11))
print("8的立方根:", cbrt2(8))
print("11的立方根:", cbrt2(11))