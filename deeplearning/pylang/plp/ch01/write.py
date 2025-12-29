import pandas as pd
from openpyxl import Workbook

# 1. 准备您的数据
data = [
    {'姓名': '张三', '年龄': 30, '城市': '北京'},
    {'姓名': '李四', '年龄': 25, '城市': '上海'},
    {'姓名': '王五', '年龄': 42, '城市': '广州'}
]

# 2. 将数据转换为 DataFrame
df = pd.DataFrame(data)

# 3. 写入 Excel 文件
# 'output_pandas.xlsx' 是您要保存的文件名
# index=False 表示不把 pandas 默认的行索引 (0, 1, 2...) 写入 Excel
df.to_excel('output_pandas.xlsx', sheet_name='员工信息', index=False)

print("使用 pandas 写入 Excel 成功！")