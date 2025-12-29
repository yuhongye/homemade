# -*- coding: utf-8 -*-
from pyexpat.errors import messages

from openai import OpenAI
import json

import pandas as pd
from openpyxl import Workbook


OPENAI_API_KEY = ""

client = OpenAI(
    base_url="https://qianfan.baidubce.com/v2",
    default_headers={
        "Authorization": "Bearer bce-v3/ALTAK-EqUeg3v0bhJcVAXkYjfa7/3e378d60b460103257b907870596a32f1b8019ee"
    },
    api_key=""
)

lines = [
[{"id":"23146037551","role":"user","content":[{"type":"image","url":"https://dwz.cn/DBhFW806","format":"png","name":"1762843769113.png","detail":"low","file_id":"file-740177376697157"},{"type":"text","text":"按照图中场景，给我生成一张小猪在此地奔跑的图片。"}],"content_type":"text"}],
[{"id":"23144970876","role":"user","content":[{"type":"image","url":"https://dwz.cn/6aB0LNn1","format":"png","name":"1762840631457.png","detail":"low","file_id":"file-740164524737413"},{"type":"image","url":"https://dwz.cn/Pee1Y1v4","format":"png","name":"1762840635247.png","detail":"low","file_id":"file-740164540444101"},{"type":"image","url":"https://dwz.cn/R6xDignO","format":"png","name":"1762840637687.png","detail":"low","file_id":"file-740164550553029"},{"type":"text","text":"立即执行三图融合：\n\n第一张提取构图结构和主体形态\n\n第二张提取色彩方案和光影方向\n\n第三张提取细节纹理和氛围\n将三者融合生成一张新图，必须高分辨率、细节完整、视觉冲击力强，风格统一且自然衔接。"}],"content_type":"text"}],
[{"id":"23145671153","role":"user","content":[{"type":"image","url":"https://dwz.cn/zcKd4aSC","format":"png","name":"1762842907387.png","detail":"low","file_id":"file-740173851054789"},{"type":"text","text":"把它换成小狗"}],"content_type":"text"}],
[{"id":"23144172398","role":"user","content":[{"type":"image","url":"https://dwz.cn/01r9yroA","format":"png","name":"微信图片_2025111112.png","detail":"low","file_id":"file-740144271098757"},{"type":"image","url":"https://dwz.cn/qyJKWaKK","format":"png","name":"微信图片_2025111112.png","detail":"low","file_id":"file-740144271095045"},{"type":"text","text":"将第二张图片换成图1风格"}],"content_type":"text"}]
]

data = []

for list in lines:
    if len(list) == 1:
        line = list[0]
        content = []
        for part in line["content"]:
            type = part["type"]
            if type == "image":
                content.append({"type": "image_url", "image_url": {"url": part["url"]}})
            else:
                content.append({"type": "text", "text": part["text"]})

        messages = [{"role": "user", "content": content}]
    else:
        system = {"role": "system", "content": list[0]["content"]}
        line = list[1]
        content = []
        for part in line["content"]:
            type = part["type"]
            if type == "image":
                content.append({"type": "image_url", "image_url": {"url": part["url"]}})
            else:
                content.append({"type": "text", "text": part["text"]})
        messages = [system, {"role": "user", "content": content}]
    print(messages)
    response = client.chat.completions.create(
                     model="ernie-5.0-thinking-preview",
                     messages=messages
                 )
    reason = response.choices[0].message.reasoning_content
    res_content = response.choices[0].message.content
    id = response.id

    print(reason)
    print(res_content)
    data.append({"id": id, "message": json.dumps(messages, ensure_ascii=False), "思考": reason, "回复": res_content})

df = pd.DataFrame(data)
df.to_excel('图生图_随机4条.xlsx', sheet_name='图生图', index=False)


#
# with open('/Users/caoxiaoyong/Downloads/superclue_178-0.jsonl', 'r', encoding='utf-8') as f:
#     # 使用 next() 函数读取文件的第一行
#     lines = f.readlines()
#     count = 0
#     for line in lines:
#         object = json.loads(line)
#         message = object['message']
#         content = object['content']
#
#         response = client.chat.completions.create(
#             model="ernie-x1-turbo-latest",
#             messages=[{"role": "user", "content": message}],
#         )
#         res_content = response.choices[0].message.content
#         # print(res_content)
#         print(count, content == res_content)
#         count = count + 1
