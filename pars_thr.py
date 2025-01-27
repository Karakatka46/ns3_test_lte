import pandas as pd
import re

dl_file_path = '/home/aplelsin/yo_test1/ns-allinone-3.43/ns-3.43/DlRlcStats.txt'
ul_file_path = '/home/aplelsin/yo_test1/ns-allinone-3.43/ns-3.43/UlRlcStats.txt'

def load_data(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()

    data = []
    for line in lines:
 
        numbers = re.findall(r'\S+', line.strip())
        data.append(numbers)

    df = pd.DataFrame(data)
    df = df.apply(pd.to_numeric, errors='ignore')

    return df

df_dl = load_data(dl_file_path)
df_ul = load_data(ul_file_path)

print(f"Количество столбцов в DL DataFrame: {df_dl.shape[1]}")
print(f"Количество столбцов в UL DataFrame: {df_ul.shape[1]}")

if df_dl.shape[1] == df_ul.shape[1]:
    column_names = [
        "start", "end", "CellId", "IMSI", "RNTI", "LCID", "nTxPDUs", "TxBytes", 
        "nRxPDUs", "RxBytes", "delay", "stdDev", "min", "max", "PduSize", 
        "stdDev_2", "min_2", "max_2", "extra_column" 
    ]
    df_dl.columns = column_names[:df_dl.shape[1]]  
    df_ul.columns = df_dl.columns
else:
    print("Ошибка: Количество столбцов в DL и UL DataFrame не совпадает!")

print("Названия столбцов для DL:")
print(df_dl.columns)

print("Названия столбцов для UL:")
print(df_ul.columns)

df_dl['start'] = pd.to_numeric(df_dl['start'], errors='coerce')
df_dl['end'] = pd.to_numeric(df_dl['end'], errors='coerce')
df_dl['TxBytes'] = pd.to_numeric(df_dl['TxBytes'], errors='coerce')

df_ul['start'] = pd.to_numeric(df_ul['start'], errors='coerce')
df_ul['end'] = pd.to_numeric(df_ul['end'], errors='coerce')
df_ul['RxBytes'] = pd.to_numeric(df_ul['RxBytes'], errors='coerce')

# Функция для расчета throughput
def calculate_throughput(df, tx_col, time_col_start, time_col_end):
    total_bytes = 0
    total_time = 0

    for _, row in df.iterrows():
        start_time = row[time_col_start]
        end_time = row[time_col_end]

        # print(f"start: {start_time}, end: {end_time}")
        if pd.isna(start_time) or pd.isna(end_time) or start_time >= end_time:
            continue  
        total_time += end_time - start_time

        
        tx_bytes = row[tx_col]
        if pd.isna(tx_bytes):
            continue
        total_bytes += tx_bytes  

    if total_time == 0:
        print("Ошибка: total_time равно 0!")
        return 0

    throughput = total_bytes / total_time / 1e6
    return throughput

dl_throughput = calculate_throughput(df_dl, 'TxBytes', 'start', 'end')
ul_throughput = calculate_throughput(df_ul, 'RxBytes', 'start', 'end')

print(f"Throughput в DL: {dl_throughput:.2f} Мбит/с")
print(f"Throughput в UL: {ul_throughput:.2f} Мбит/с")
