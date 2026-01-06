import csv
import random
import string
from datetime import date, timedelta

def generate_goods_data(file_path, num_records=1000):
    headers = ['物品ID', '物品名称', '所属地', '发往地', '物品类型', '客户等级', '收获日期']
    regions = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H']
    
    # --- 关键修改：定义合法的航空路线 (必须与 regionAir.csv 的连通性一致) ---
    # 根据之前的矩阵生成逻辑，只有 A-H, B-G, C-F 是互通的
    valid_air_routes = [
        ('A', 'H'), ('H', 'A'),
        ('B', 'G'), ('G', 'B'),
        ('C', 'F'), ('F', 'C')
    ]
    
    rows = []
    today = date.today()
    print(f"正在生成 {num_records} 条数据 (已修正航空件逻辑)...")

    for i in range(1, num_records + 1):
        goods_id = i
        goods_name = random.choice(string.ascii_lowercase)
        
        # 1. 先随机生成类型
        goods_type = random.randint(1, 4) # 假设只有1-4种方案
        
        # 2. 根据类型决定起终点
        if goods_type == 4:
            # --- 航空件：必须从合法的航线中选 ---
            start, end = random.choice(valid_air_routes)
            belonging_area = start
            sending_area = end
        else:
            # --- 普通件：随便选，只要起终点不同 ---
            belonging_area = random.choice(regions)
            sending_area = random.choice(regions)
            while sending_area == belonging_area:
                sending_area = random.choice(regions)
        
        client_grade = random.randint(1, 10)
        
        # 日期生成 (过去7天)
        days_ago = random.randint(0, 7)
        record_date = today - timedelta(days=days_ago)
        date_str = record_date.strftime('%Y-%m-%d')
        
        rows.append([goods_id, goods_name, belonging_area, sending_area, goods_type, client_grade, date_str])
        
    try:
        # 编码建议使用 utf-8-sig 以便 Excel 打开不乱码，或者 utf-8
        with open(file_path, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(headers)
            writer.writerows(rows)
        print(f"成功！数据已保存到: {file_path}")
        print("请将此文件移动到 C++ 工程的 data/ 目录下并覆盖原文件。")
        
    except IOError as e:
        print(f"文件写入失败: {e}")

if __name__ == '__main__':
    generate_goods_data('goodsInfo_generated.csv', 1000)