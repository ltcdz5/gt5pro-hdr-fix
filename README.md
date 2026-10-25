# GT5 Pro HDR Fix

一个试图修复 **realme GT5 Pro（RMX3888）播放 HDR 视频偏黄** 的 KernelSU 模块。

> 这台机器屏幕素质很好，但 HDR 视频播放时画面明显发黄，和同机器的 SDR 内容比像是换了一块屏。
>
> 排查下来问题不在 HAL，也不在播放器——**是进入 HDR 模式那一刻，驱动往 DSPP 的
> 3D gamut 表里写了一张偏暖的表，之后整个会话不再更新**。
> 本模块在那一次写入时把它换成校正过的表，其余场景一概不碰。

---

## 特性

- **只改 HDR**：白名单识别，SDR 内容完全不受影响
- **横竖屏都支持**：各自独立校正表，按场景自动选择
- **保持原厂亮度**：只调色温，不压亮度
- **无侵入**：只改运行时显存里的表内容，**不写任何分区**
- **随开机生效**：KernelSU 模块，装完重启即用
- **可随时关掉**：一个参数或一条 `rmmod`

---

## 效果

在 realme GT5 Pro（RMX3888 / SM8650 / Android 16 / 内核 6.1.141）上验证：

| 场景 | 修正前 | 修正后 |
|---|---|---|
| 横屏 HDR 白色 | 明显偏黄 | 接近中性，亮度保持 |
| 横屏 HDR 中灰 | 偏棕 | 接近中性 |
| 竖屏 HDR | 偏黄 | 白点灰点均中性，饱和度略增 |
| **SDR 内容** | — | **完全不受影响**（实测 822 次调用全部放行）|

主观评价来自实际观感比对与用户取色校准，不是仪器测量，不同批次可能有差异。

---

## 安装

1. 下载 [Releases](https://github.com/ltcdz5/gt5pro-hdr-fix/releases) 里的 zip
2. 在 KernelSU 管理器中刷入
3. 重启

装好后不需要任何配置。

验证：

```bash
su -c 'lsmod | grep sidecar56'
su -c 'dmesg | grep sidecar56'      # 应看到 rc=0
su -c 'cat /proc/sdehdrstat'        # 查看命中统计
```

---

## 卸载 / 临时停用

```bash
# 临时停用（本次开机有效）
su -c 'echo 0 > /sys/module/sidecar56/parameters/enable'
su -c 'rmmod sidecar56'

# 彻底卸载
su -c 'rmmod sidecar56; rm -rf /data/adb/modules/hdrfix'
```

---

## 兼容性

- 需要 **KernelSU**
- 已验证：realme GT5 Pro（RMX3888 / SM8650 / Android 16 / 内核 6.1.141）
- 模块内含**针对该内核构建的 `.ko`**，内核版本不一致会加载失败（不影响开机）
- 表指纹是设备相关的，其他机型需自行采集

---

## 已知边界

| 场景 | 是否生效 | 说明 |
|---|---|---|
| 全屏 HDR 视频（横屏） | 是 | 显示进入 HDR 模式，指纹命中横屏表 |
| 全屏 HDR 视频（竖屏） | 是 | 指纹命中竖屏表 |
| **半屏 HDR + SDR 界面** | **是** | **走竖屏 HDR 路线**：SurfaceFlinger 把 HDR 色调映射后产出的表与竖屏 HDR 一致（或白点蓝偏低），命中竖屏补蓝表 |
| 普通 SDR 内容 | 不修改 | SDR 指纹最高优先级放行（设计目标）|

半屏场景实际走的就是竖屏 HDR 那条路，因此同样被修正。
v1.0 文档里「半屏不生效」是当时的误判，已更正。

---

## 文档

原理、通道映射、调参方法、踩过的坑见 [module/README.md](module/README.md)。

---

## 许可

GPL-3.0
