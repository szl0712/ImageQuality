# 图像画质分析工具 ImageQuality
## 一. 项目基础信息
### 1.基础概况
项目名称：图像画质分析工具  
适用平台：windows 桌面端、Android 移动端  
技术栈：
- Windows端：Qt6.5.3 + C++ + OpenCV4.5.4
- Android端：Qt for Android 6.5.3 + C++ + OpenCV4.5.4   

开发人：尚之量  
开发周期：6.20 - 6.30  
应用场景：本地单张图片画质自动化评估  
可复现代码仓库地址：https://github.com/szl0712/ImageQuality.git
### 2.核心目标
1.兼容 JPG/JPEG/PNG/WebP/HEIC 等图片格式，稳定加载 ≥12MP 大图，无OOM崩溃；  
2.实现清晰度、曝光亮度、对比度三大画质评分维度；  
3.输出标准化结果：0~100分项得分、综合总分、分级文字评价；  
4.完整留存算法文档：标注各评分指标计算来源、参数含义、算法固有局限。

### 3.非目标
1.不支持 RAW 图片、批量图片检测、图片编辑美化功能；  
2.无深度学习模型，全部基于传统OpenCV图像处理算子实现；  
3.UI 仅保留基础预览、分析、导出功能，无美化动画  

## 二.已完成核心功能
### 1.多格式图片加载与大图性能优化
- 兼容JPG/JPEG/PNG/WebP/HEIC主流静态图片格式，文档配套各格式解码差异、编译依赖说明；  
- 采用分块解码、按需缩放、图像内存自动释放策略，Windows/Android真机均可稳定加载12MP及以上大图，无闪退、内存溢出。

### 2.画质评分模块 
**清晰度：** 拉普拉斯、Tenengrad 梯度、边缘占比多指标融合，多区域采样输出百分制清晰度分数；  
**曝光分析：** 灰度直方图分区统计明暗像素，搭配画面平均亮度综合评定曝光效果；   
**画质细节：** 8×8 分块 Michelson 对比度计算，过滤平淡区域后换算为百分制得分；  

### 3.完整日志与报告导出 
自动采集信息：图片尺寸、文件格式、解码耗时、各维度原始特征值、分项得分、文字评级、综合总分；  
支持一键导出完整画质分析报告至本地，导出格式为TXT，可自定义存储路径。

### 4.算法验证与测试 
内置test_sample测试图库，覆盖严重模糊、清晰、欠曝、过曝、低对比度等典型场景；可直观对比不同图像的评分差异；同时记录算法天然误判场景，用于局限性分析。

## 三.综合评分计算
综合总分加权公式：
`综合总分 = 清晰度得分 × 0.4 + 曝光得分 × 0.35 + 对比度得分 × 0.25`

## 四.项目目录结构

```plaintext
ImageQuality/
├── android/                  # Android端NDK、打包编译配置文件
├── build/                    # 编译输出目录（Debug/Release程序、exe、apk、依赖库）
├── lib/                      # Windows运行依赖dll：opencv_world454.dll、libgomp-1.dll
├── test_sample/              # 画质算法测试图片样本库
├── validation/               # 算法验证、误判反例素材文件夹
├── .gitignore                # Git版本控制忽略配置
├── AI协助说明.md             # 开发分工、AI辅助工作说明文档
├── app.ico                   # Windows客户端程序图标
├── ImageLoader.h/cpp         # 图片加载、解码、内存优化模块
├── ImageQuality.pro          # Qt项目工程总配置文件
├── ImageSharpness.h/cpp      # 清晰度/曝光/对比度评分算法核心模块
├── main.cpp                  # 程序入口函数
├── mainwindow.h/cpp/ui       # 主窗口UI界面、业务交互逻辑
├── README.md                 # 项目总说明文档
├── resources.qrc             # Qt静态资源配置文件
├── xiangce.png               # UI界面内置图片资源
├── 配套技术说明文档.md        # 底层算法、参数、测试对比完整技术文档
└── 约束与决策说明.md          # 开发约束、技术选型与功能取舍记录
```

## 五.编译&运行前置环境配置

### 1. 基础编译环境

| 项目 | 说明 |
|------|------|
| **编译工具** | Qt Creator 11+，套件 Qt 6.5.3 MinGW（Windows）/ Qt Android 6.5.3 |
| **图像处理依赖库** | OpenCV 4.5.4 |
| **Android 额外依赖** | 配套版本 NDK、Android SDK |

### 2. OpenCV 路径配置说明

项目 `.pro` 内使用本地绝对路径，克隆代码后需要自行修改为本地 OpenCV 存放目录。

**Windows 桌面端**
```pro
win32 {
    INCLUDEPATH += D:/zl/OpenCV454/include
    LIBS += -LD:/zl/OpenCV454 -llibopencv_world454
}
```
- 自行下载 OpenCV 4.5.4 Windows 版，解压后将 D:/zl/OpenCV454 替换为你的解压根目录；
- 项目内置 lib/ 文件夹存放运行必需两个 DLL：opencv_world454.dll、libgomp-1.dll，编译完成后脚本自动复制至 exe 输出目录。

**Android 端**
```pro
android {
    OPENCV_ANDROID_SDK = D:/zl/opencv-4.5.4-android-sdk/OpenCV-android-sdk/sdk/native
}
```
- 自行下载 OpenCV 4.5.4 Android SDK，替换上述路径为本地 SDK 根目录；
- Android 打包时会自动内置对应平台 OpenCV so 库。

### 3. Windows 完整编译运行流程
Qt Creator 打开 ImageQuality.pro，修改 .pro 中 OpenCV 本地路径；  
切换 Release 构建套件，执行构建；编译完成自动将 lib 内 DLL 复制至输出目录；  
打开 Qt 终端，进入本地生成的 build/release 目录，执行命令补齐 Qt 运行库：
```pro
windeployqt ImageQuality.exe
```
运行 ImageQuality.exe，打开图片即可执行画质分析  
如需直接分发程序：将完整 build/release 文件夹压缩即可，无需额外配置环境。

### 4. Android 打包流程
配置 Qt Android 套件、NDK、SDK 环境，修改 .pro 内 Android OpenCV SDK 路径；  
构建 Release，执行 Android 打包，生成 APK 安装包；  
安卓设备安装 APK 后可直接本地读取相册图片做画质评估。





