import numpy as np
import matplotlib.pyplot as plt

# 定义幅度和 dB 之间的关系
def amplitude_to_db(amplitude):
    # 避免对零取对数，确保幅度为正值
    return 20 * np.log10(amplitude)

# 生成幅度值，从0.0001到1（避免零）
amplitude_values = np.linspace(0.001, 1, 1000)
# 计算对应的 dB 值
db_values = amplitude_to_db(amplitude_values)

# 绘制幅度与 dB 值之间的关系图
plt.figure(figsize=(10, 6))
plt.plot(amplitude_values, db_values, label='dB = 20 * log10(amplitude)', color='blue')
plt.title('Amplitude to dB')
plt.xlabel('Amplitude (normalized to [0,1])')
plt.ylabel('dB Value')
plt.grid()
plt.axhline(0, color='red', linewidth=0.9, linestyle='--')  # 添加0 dB水平线
plt.axvline(1, color='red', linewidth=0.5, linestyle='--', label='0 dB (amplitude=1)')  # 1对应0 dB
plt.legend()
plt.xscale('linear')
plt.ylim(-80, 10)  # 限制 y 轴范围
plt.xlim(0, 1)      # 限制 x 轴范围
plt.show()