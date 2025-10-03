from openai import OpenAI
import json

OPENAI_API_KEY = ""

client = OpenAI(
    base_url="https://qianfan.baidubce.com/v2",
    default_headers={
        "Authorization": "Bearer bce-v3/ALTAK-xz9cShgBNpx2ZzdFZfLoQ/3f38cb3515a50d8094e96013179d94c9f33dcb91",
        "appid": "app-wvdxMsS9"
    },
    api_key=""
)

with open('/Users/caoxiaoyong/Downloads/superclue_178-0.jsonl', 'r', encoding='utf-8') as f:
    # 使用 next() 函数读取文件的第一行
    lines = f.readlines()
    count = 0
    for line in lines:
        object = json.loads(line)
        message = object['message']
        content = object['content']

        response = client.chat.completions.create(
            model="ernie-x1-turbo-latest",
            messages=[{"role": "user", "content": message}],
        )
        res_content = response.choices[0].message.content
        # print(res_content)
        print(count, content == res_content)
        count = count + 1
