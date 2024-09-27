import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile

# 读取音频文件
sample_rate, audio_data = wavfile.read('pcm_16k_1ch_s16le.wav')

# 如果音频数据是立体声，取其中一个通道
if len(audio_data.shape) > 1:
    audio_data = audio_data[:, 0]

# 设置时间轴
time = np.linspace(0, len(audio_data) / sample_rate, num=len(audio_data))

# 对音频数据进行傅里叶变换
n = len(audio_data)
audio_fft = np.fft.fft(audio_data)
audio_freq = np.fft.fftfreq(n, d=1/sample_rate)

# 只取前半部分频率（因为 FFT 结果是对称的）
audio_fft = np.abs(audio_fft[:n//2])
audio_freq = audio_freq[:n//2]

# 创建图形
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

# 绘制时域图像
ax1.plot(time, audio_data, color='b')
ax1.set_title("Audio Signal in Time Domain")
ax1.set_xlabel("Time [s]")
ax1.set_ylabel("Amplitude")
ax1.grid(True)

# 绘制频域图像
ax2.plot(audio_freq, audio_fft, color='r')
ax2.set_title("Audio Signal in Frequency Domain")
ax2.set_xlabel("Frequency [Hz]")
ax2.set_ylabel("Amplitude")
ax2.grid(True)

# 显示图像
plt.tight_layout()
plt.show()
