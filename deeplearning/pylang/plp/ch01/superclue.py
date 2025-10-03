import json

# 假设你的 JSONL 文件名为 'data.jsonl'

try:
    with open('/Users/caoxiaoyong/Downloads/superclue_178.jsonl', 'r', encoding='utf-8') as f:
        # 使用 next() 函数读取文件的第一行
        lines = f.readlines()
        for line in lines:
            object = json.loads(line)
            message = object['message']
            print(message)
            print('-------------------------------------')

except FileNotFoundError:
    print("错误：文件 'data.jsonl' 不存在。请检查文件名和路径。")
except json.JSONDecodeError:
    print("错误：第一行内容不是有效的 JSON 格式。")
except StopIteration:
    print("错误：文件是空的。")