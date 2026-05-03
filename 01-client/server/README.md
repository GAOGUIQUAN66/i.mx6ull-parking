# server 目录

本目录包含所有 Python 端服务端代码和识别资源。

## 文件

- `server.py`: TCP 服务端入口
- `lpr_service.py`: 车牌识别封装层
- `lpr_predict.py`: 从 `License-Plate-Recognition` 适配的识别核心
- `config.js`: 识别配置
- `svm.dat`: 英文/数字 SVM 模型
- `svmchinese.dat`: 中文省份 SVM 模型

## 部署位置

将整个 `server/` 目录复制到：

`/home/chentao/iflytek_xtts_light/Linux_aisound_ece9d3c90_v1.1.5_v2.2.15-rc5`

复制后目录结构应为：

- `/home/chentao/.../aikit_test`
- `/home/chentao/.../libs`
- `/home/chentao/.../aisound`
- `/home/chentao/.../server/server.py`

`server.py` 假定 `aikit_test` 和 `libs/` 在父目录中。

## Python 依赖

- `python3`
- `opencv-python` 或系统 `cv2`
- `numpy`

在 Ubuntu 上可通过以下命令安装系统包：

```bash
sudo apt-get update
sudo apt-get install python3-opencv python3-numpy
```

## 运行

在复制后的 `server/` 目录中运行：

```bash
cd /home/chentao/iflytek_xtts_light/Linux_aisound_ece9d3c90_v1.1.5_v2.2.15-rc5/server
python3 server.py
```

## 协议

- 接收图像数据包（`type=1`）或原始帧数据包（`type=4`）
- 返回识别结果 JSON（`type=3`）
- 识别成功时，同时返回 WAV 音频数据包（`type=5`）

## 注意事项

- 原始 `RGBP` 帧解码为 `RGB565` 格式
- 识别使用集成的 `License-Plate-Recognition` SVM 管道
- TTS 使用父目录中的可执行文件 `aikit_test`
