# -*- coding: utf-8 -*-
import time

from openai import OpenAI


OPENAI_API_KEY = "Bearer bce-v3/ALTAK-xz9cShgBNpx2ZzdFZfLoQ/3f38cb3515a50d8094e96013179d94c9f33dcb91"

client = OpenAI(
    base_url="https://qianfan.baidubce.com/v2",
    default_headers={
        "Authorization": OPENAI_API_KEY,
        "appid": "app-wvdxMsS9"
    },
    api_key=""
)

messages = [
        {
            "role": "user",
            "content": [
                {
                    "type": "text",
                    "text": "在下述基因表达动力学系统中，将激活物浓度 A 视为一个可变控制参数。当 A 从 0 开始逐渐增加时，系统的稳态解会经历鞍结分岔（saddle-node bifurcation）。请在忽略时滞（τ→0）的条件下，计算该系统发生第一次鞍结分岔时的临界激活物浓度 A_c 和与之对应的稳态蛋白浓度 P_c。（计算结果请至少保留三位有效数字）\n\n系统描述如下：\n1. 微分方程组：\nd[mRNA](t)/dt = k₀ + (k − k₀)·F(A,R,P(t)) − γ_m·[mRNA](t)\ndP(t)/dt = k_tl·[mRNA](t) − γ_p·P(t)\n2. 组合调控函数（修改为正反馈）：\nF(A,R,P) = [A^{n₁}/(K₁^{n₁} + A^{n₁})] · [K₂^{n₂}/(K₂^{n₂} + R^{n₂})] · [P^{n₃}/(K₃^{n₃} + P^{n₃})]\n3. 固定参数值（k₀值已修改）：\nn₁ = 2, n₂ = 3, n₃ = 4;\nK₁ = 2 nM, K₂ = 1 nM, K₃ = 10 nM;\nk₀ = 1/960 nM·min⁻¹, k = 15 nM·min⁻¹;\nγ_m = 0.25 min⁻¹, γ_p = 0.01 min⁻¹, k_tl = 6 min⁻¹;\n抑制物浓度 R = 0.8 nM。\n请将最终的答案写在\\boxed{}"
                }
            ]
        }
    ]
start = time.time()
try:
    response = client.chat.completions.create(
        model="ernie-5.0-thinking-latest",
        messages=messages,
        timeout=7200
    )
    reason = response.choices[0].message.reasoning_content
    res_content = response.choices[0].message.content
    as_id = response.id
    print(f"id: {as_id} cost: {time.time() - start}\nreasoning_content: {reason}\nresponse:{res_content}")
except Exception as e:
    print(f"error, cost: {time.time() - start}")
    print(e)



