import csv
import random
import string
from datetime import date, timedelta

def generate_goods_data(file_path, num_records=1000):
    # 1. 定义表头 (与你的示例保持一致)
    headers = ['物品ID', '物品名称', '所属地', '发往地', '物品类型', '客户等级', '收获日期']
    
    # 2. 定义数据的取值范围
    # 地区列表 (根据你的 regionDistance.csv 设定为 A-H)
    regions = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H']
    
    rows = []
    
    # 获取脚本运行时的“今天”
    today = date.today()
    
    print(f"正在生成 {num_records} 条数据...")
    print(f"日期基准: {today} (数据将分布在过去7天内)")

    for i in range(1, num_records + 1):
        # ---------------- 数据生成逻辑 ----------------
        
        # 物品ID: 顺序生成
        goods_id = i
        
        # 物品名称: 随机生成一个大写字母或小写字母
        goods_name = random.choice(string.ascii_lowercase)
        
        # 所属地: 随机选一个
        belonging_area = random.choice(regions)
        
        # 发往地: 随机选一个 (尽量不与所属地相同，虽然不是强制)
        sending_area = random.choice(regions)
        while sending_area == belonging_area:
            sending_area = random.choice(regions)
            
        # 物品类型: 1 到 4
        goods_type = random.randint(1, 4)
        
        # 客户等级: 1 到 7
        client_grade = random.randint(1, 7)
        
        # 收获日期: 核心逻辑 - 生成 [今天 - 7天, 今天] 范围内的日期
        days_ago = random.randint(0, 7)
        record_date = today - timedelta(days=days_ago)
        date_str = record_date.strftime('%Y-%m-%d')
        
        # ---------------------------------------------
        
        rows.append([goods_id, goods_name, belonging_area, sending_area, goods_type, client_grade, date_str])
        
    # 3. 写入 CSV 文件
    try:
        # 使用 utf-8 编码
        with open(file_path, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(headers) # 写入表头
            writer.writerows(rows)   # 写入数据
            
        print(f"成功！文件已保存为: {file_path}")
        
    except IOError as e:
        print(f"文件写入失败: {e}")

if __name__ == '__main__':
    # 你可以在这里修改生成的文件名和数量
    generate_goods_data('goodsInfo_generated.csv', 1000)