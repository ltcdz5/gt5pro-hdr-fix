# 技术说明

## 它拦截的是什么

DSPP 的 **3D gamut 表**（17×17×17，每项 12 bit，整表 39536 字节）。

进入 HDR 模式时，驱动调用：

```
reg_dmav2_setup_dspp_3d_gamutv43(struct sde_hw_dspp *ctx, void *cfg)
```

其中 `cfg->payload` 指向驱动缓存的表数据，`cfg->len = 39536`。
本模块用 kprobe 挂在这个函数上，**把 payload 的内容整体替换**成校正后的表。

之所以能持久：`payload` 指向的是驱动自己缓存的 blob（不是栈上临时结构），
改它就是改驱动状态；而 HDR 期间驱动不会再次重写，所以一次替换管整个会话。

## 为什么能只影响 HDR

驱动在 SDR 时用的是**另一张表**，两张表内容不同。模块对每次调用计算
FNV-1a 32 位指纹，只有等于 HDR 表的指纹（`0xb9b4c7e9`）时才替换，
其余一律原样放行。

指纹是设备/固件相关的，换机型或 OTA 后需要重新取。

## 表的编码

```
struct drm_msm_3d_gamut {
    u64 flags;
    u32 mode;                  // 必须为 1（mode 17）
    u32 scale_off[3][16];
    struct drm_msm_3d_col col[4][1229];   // 偏移 204，共 39328 字节
};

struct drm_msm_3d_col {
    u32 c2_c1;   // 高 16 位 = G，低 16 位 = B
    u32 c0;      // R
};
```

**通道映射：c0 = R，c1 = B，c2 = G。**（注意 c1 是蓝，不是绿——
这一点搞错会让所有调色都往绿色跑，是本项目踩过最大的坑。）

网格索引：

```
n = g*289 + b*17 + r      // r 变化最快
table = n % 4
entry = n / 4
```

## 校正曲线

逐通道、随电平变化：

```
gain(x) = a + b * (x / 4095)
out    = clamp(x * gain(x))
```

当前参数：

| 通道 | a（中段） | b | 白点 |
|---|---|---|---|
| R | 0.700 | -0.110 | 0.590 |
| G | 0.755 | -0.135 | 0.620 |
| B | 1.030 | -0.280 | 0.750 |

思路：面板本身偏暖，但低端和高端的偏移量不同，所以用一条随电平的直线去贴，
而不是固定增益——固定增益会把白点一起拉偏，表现为"要么黄要么青"。

## 重新生成表

```bash
python3 tools/gain5.py 0.70 -0.11 0.755 -0.135 1.03 -0.28 out.bin
```

脚本读取基准表（需要你自己从设备上取，见下）后按参数生成。

## 取自己设备的表

模块里的 `sidecar42.ko` 带一个只读接口：

```bash
su -c 'insmod sidecar42.ko'
# 播放一段 HDR 视频，然后
su -c 'cat /proc/sdehdrlut > /sdcard/hdr_table.bin'
```

不过这个接口返回的是**已经被替换过**的表（如果之前写进去了）。
要取原始表，先 `rmmod` 并重启，或用 `/proc/sdehdrlut` 的写入接口先清空。

（开发期另有 `sdegamut_last` 版本用于抓取最后一次编程的表，未随模块发布。）

## 调试

`/proc/sdehdrlut` 可读可写：

```bash
su -c 'cat /proc/sdehdrlut > /sdcard/cur.bin'      # 读当前注入的表
su -c 'cat /sdcard/new.bin > /proc/sdehdrlut'      # 换一张（下次 HDR 切换生效）
```

注意：只在**下一次进入 HDR**时生效，播放中途改不会立即反映。
