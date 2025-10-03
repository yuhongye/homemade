import os

trace_enable = False

def set_up():
    global trace_enable
    print('trace_enable', trace_enable)
    trace_enable = os.getenv("TRACES_ENABLE", "False").lower() == 'true'
    print('trace_enable', trace_enable)

def is_trace_enable():
    print(trace_enable)

if __name__ == '__main__':
    print('[MAIN] trace_enable', trace_enable)
    set_up()
    print('[MAIN] trace_enable', trace_enable)
    is_trace_enable()
