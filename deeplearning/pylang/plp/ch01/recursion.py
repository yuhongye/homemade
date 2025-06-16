from time import time

def fib(n):
    if n < 1:
        return 0
    if n == 1:
        return 1
    return fib(n - 1) + fib(n - 2)

def fib_perf(n):
    start = time() # 返回的是秒级时间戳
    f = fib(n)
    print("Fib[", n, "]=", f, "Timing:", time() - start)

for i in range(32, 40):
    fib_perf(i)