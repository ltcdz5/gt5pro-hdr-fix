---
name: Bug 反馈
about: 模块没生效、画面异常、或其他问题
title: '[Bug] '
labels: bug
---

## 设备信息

- 机型：
- 系统版本：
- 内核版本（`uname -r`）：
- KernelSU 版本：

## 问题描述

<!-- 什么现象？什么时候出现？ -->

## 已做的检查

```bash
su -c 'lsmod | grep sidecar42'
su -c 'dmesg | grep sidecar42'
su -c 'md5sum /proc/sdehdrlut /data/adb/modules/hdrfix/ghdr_lut.bin'
```

<!-- 把上面三条命令的输出贴这里 -->

## 复现步骤

1.
2.

## 补充

<!-- 截图、视频、日志等 -->
