import functools

def log(func):
    @functools.wraps(func)  # 保留原函数签名
    def wrapper(*args, **kwargs):
        print(f"[LOG] 调用 {func.__name__}，参数是 {args}, {kwargs}")
        result = func(*args, **kwargs)
        print(f"[LOG] 返回值是 {result}")
        return result
    return wrapper

@log
def add(a, b):
    return a + b

add(2, 3)
