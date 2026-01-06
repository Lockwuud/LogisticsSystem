import csv
import random
import os

# 确保 data 目录存在
os.makedirs('data', exist_ok=True)

regions = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H']
n = len(regions)
inf = 10000

# 1. 基础拓扑图 (邻接关系)
# 1表示连通, 0表示不连通
connectivity = [
    [0, 1, 1, 1, 0, 0, 0, 0], # A
    [1, 0, 1, 0, 1, 0, 0, 0], # B
    [1, 1, 0, 0, 0, 1, 0, 0], # C
    [1, 0, 0, 0, 0, 1, 1, 0], # D
    [0, 1, 0, 0, 0, 0, 1, 1], # E
    [0, 0, 1, 1, 0, 0, 1, 1], # F
    [0, 0, 0, 1, 1, 1, 0, 1], # G
    [0, 0, 0, 0, 1, 1, 1, 0], # H
]

# 生成矩阵的辅助函数
def generate_matrix(base_weight, fluctuation):
    matrix = [['' for _ in range(n + 1)] for _ in range(n + 1)]
    # 表头
    matrix[0] = [''] + regions
    for i in range(n):
        matrix[i+1][0] = regions[i]
        for j in range(n):
            if i == j:
                matrix[i+1][j+1] = 0
            elif connectivity[i][j] == 1:
                # 连通的点，赋予权重
                matrix[i+1][j+1] = base_weight + random.randint(0, fluctuation)
            else:
                matrix[i+1][j+1] = inf
    return matrix

# --- 生成 regionDistance.csv (时间矩阵，单位：小时) ---
dist_matrix = generate_matrix(5, 10) # 基础时间5小时，波动10
with open('data/regionDistance.csv', 'w', newline='') as f:
    csv.writer(f).writerows(dist_matrix)

# --- 生成 regionPrice.csv (价格矩阵，单位：元) ---
# 价格和距离通常正相关，但为了体现差异，我们重新随机一次
price_matrix = generate_matrix(50, 100) # 基础价格50元，波动100
with open('data/regionPrice.csv', 'w', newline='') as f:
    csv.writer(f).writerows(price_matrix)

# --- 生成 regionAir.csv (航空矩阵，极快但连通性差) ---
air_matrix = [['' for _ in range(n + 1)] for _ in range(n + 1)]
air_matrix[0] = [''] + regions
for i in range(n):
    air_matrix[i+1][0] = regions[i]
    for j in range(n):
        if i == j: air_matrix[i+1][j+1] = 0
        else: air_matrix[i+1][j+1] = inf

# 设定几条航空专线 (A-H, B-G, C-F)
air_lines = [(0, 7), (1, 6), (2, 5)] # 索引
for u, v in air_lines:
    air_matrix[u+1][v+1] = 2 # 只要2小时
    air_matrix[v+1][u+1] = 2

with open('data/regionAir.csv', 'w', newline='') as f:
    csv.writer(f).writerows(air_matrix)

print("矩阵文件已生成到 data/ 目录。")