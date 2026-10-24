# GT5 Pro HDR Fix

一个试图修复 **realme GT5 Pro（RMX3888）播放 HDR 视频偏黄、偏暗** 的 KernelSU 模块。

> 这台机器的屏幕素质很好，但 HDR 视频全屏播放时画面明显发黄、发灰，
> 和同机器的 SDR 内容比像是换了一块屏。
>
> 排查下来，问题不在 HAL，也不在应用——而是**进入 HDR 模式那一刻，驱动会往
> DSPP 的 3D gamut 表里写一张偏暖的表，之后整个会话不再更新**。
> 本模块在这一次写入时把它换成校正过的表，其余场景一概不碰。

---

## 特性

- **只改 HDR**：按表指纹识别，SDR 内容完全不受影响
- **无侵入**：只改运行时显存里的表内容，**不写任何分区**
- **随开机生效**：KernelSU 模块，装完重启即用
- **可随时关掉**：`rmmod` 一条命令回到原始状态
- **无二进制魔改**：不替换 vendor、不改 boot、不碰 HAL

---

## 效果

在 realme GT5 Pro（RMX3888 / SM8650 / Android 16 / 内核 6.1.141）上验证：

| 项目 | 修正前 | 修正后 |
|---|---|---|
| HDR 白色 | 明显偏黄 | 接近中性（略冷） |
| HDR 中灰 | 偏棕 | 接近中性 |
| SDR 内容 | — | 不受影响 |

主观评价来自实际观感比对（同画面与显示器参考并排），不是仪器测量，
不同机器/批次可能有差异。

---

## 安装

1. 下载 [Releases](https://github.com/ltcdz5/gt5pro-hdr-fix/releases) 里的 zip
2. 在 KernelSU 管理器中刷入
3. 重启

装好后不需要任何配置。

验证是否生效：

```bash
su -c 'lsmod | grep sidecar42'
su -c 'dmesg | grep sidecar42'          # 应看到 rc=0
su -c 'md5sum /proc/sdehdrlut /data/adb/modules/hdrfix/ghdr_lut.bin'   # 两者应一致
```

---

## 卸载

```bash
# 临时停用（本次开机有效）
su -c 'rmmod sidecar42'

# 彻底卸载
su -c 'rmmod sidecar42; rm -rf /data/adb/modules/hdrfix'
```

或者在 KernelSU 管理器里直接移除模块后重启。

---

## 兼容性

- 需要 **KernelSU**
- 已验证：realme GT5 Pro（RMX3888 / SM8650 / Android 16 / 内核 6.1.141）
- 模块内含**针对该内核构建的 `.ko`**，内核版本不一致会加载失败（不会影响开机）
- 其他机型/内核理论可自行编译 `src/sidecar42.c`，但表指纹是设备相关的，需重新取

---

## 已知边界

| 场景 | 是否生效 | 原因 |
|---|---|---|
| 全屏 HDR 视频 | 是 | 显示进入 HDR 模式，驱动会写 HDR 表 |
| 半屏 HDR + SDR 界面 | 否 | SurfaceFlinger 自行把 HDR 色调映射成 SDR，走的是 SDR 表 |
| 普通 SDR 内容 | 不修改 | 指纹不匹配，原样放行（这是设计目标） |

半屏场景的偏黄属于另一条链路（SurfaceFlinger 的色调映射质量），不在本模块范围内。

---

## 文档

原理、通道映射、调参方法见 [module/README.md](module/README.md)。

---

## 许可

GPL-3.0
