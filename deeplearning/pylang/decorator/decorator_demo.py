def my_decorator(func):
    def wrapper():
        print("✨ 执行前")
        func()
        print("✅ 执行后")
    return wrapper

@my_decorator
def say_hello():
    print("Hello!")

say_hello()
