
# POS System – 711 Convenience Store

基于 C++ 开发的便利店收银管理系统，支持商品管理、购物车、结账、销售记录查询、库存管理等功能。适用于课程项目（如 Dian 团队 2026 秋招大一题 Level 1~2）。

## 目录

- #环境要求
- #快速开始
- #数据文件格式
- #功能说明
- #命令列表
- #项目结构
- #常见问题

---

## 环境要求

- **操作系统**：Windows / Linux / macOS
- **编译器**：支持 C++17 的编译器（如 g++ 8+、MinGW-w64）
- **构建工具**：CMake（≥3.10）
- **依赖**：标准库（无需第三方库）

> 在 Windows 下推荐使用 MinGW-w64 + CMake，构建命令示例：
> ```bash
> cmake -B build -G "MinGW Makefiles"
> cmake --build build
> ```

---

## 快速开始

1. **克隆或下载项目**
   ```bash
   git clone <your-repo-url>
   cd pos-system
   ```

2. **准备数据文件**  
   在项目根目录下创建 `data/` 文件夹，并放入 `items.csv`（参见下方格式说明）。

3. **编译**
   ```bash
   cmake -B build -G "MinGW Makefiles"
   cmake --build build
   ```

4. **运行**
   ```bash
   ./build/pos.exe      # Windows
   ./build/pos          # Linux/macOS
   ```

5. **首次运行**  
   程序会自动读取 `data/items.csv`，若文件不存在或格式错误会报错。请确保文件存在且格式正确。

---

## 数据文件格式

### `data/items.csv`（商品主文件）

| 字段 | 说明 |
|------|------|
| code | 商品条码（字符串，唯一） |
| name | 商品名称 |
| price | 单价（浮点数，保留两位小数） |
| stock | 库存数量（整数） |

**示例：**
```csv
code,name,price,stock
001,Cola,3.50,100
002,Lollipop,0.50,200
003,Noodles,6.00,50
```

> 注意：表头必须为 `code,name,price,stock`，顺序不可颠倒。程序启动时读取该文件，管理员修改后会自动覆写。

### `sales.csv`（销售记录文件）

程序自动生成，位于项目根目录。格式为：
```
serial,date,time,items_detail,total
```
其中 `items_detail` 使用 `<br>` 作为商品明细的分隔符（输出时自动替换为换行符）。

---

## 功能说明

### 收银员模式（默认）

- **商品浏览**：查看所有商品信息（名称、条码、价格）
- **购物车操作**：添加/减少商品、查看小票、清空购物车
- **结账**：预检库存 → 扣减库存 → 生成销售记录 → 持久化
- **销售查询**：按日期查看销售记录及日营业额
- **新的一天**：模拟切换到下一个营业日（日期+1）
- **管理员入口**：输入 `admin` 进入管理员模式（需密码）

### 管理员模式（密码：`admin123`）

- **商品管理**：查看完整信息（含库存）、修改价格、添加/删除商品
- **库存管理**：进货（增加库存）、盘点（直接设置库存）
- **数据持久化**：所有修改即时写回 `data/items.csv`

---

## 命令列表

### 收银员模式

| 命令 | 说明 |
|------|------|
| `prices` | 显示所有商品的名称、条码、价格 |
| `<条码>` | 将对应商品加入购物车（数量+1） |
| `-<条码>` | 将购物车中对应商品数量-1（减至0则移除） |
| `print` | 打印当前购物车小票（含小计） |
| `drop` | 清空购物车 |
| `checkout` | 结账（预检库存→扣减→记录→清空购物车） |
| `sales [日期]` | 查看指定日期的销售记录（默认当天） |
| `newday` | 进入下一个营业日（日期+1，清空当日内存记录） |
| `admin` | 进入管理员模式（需输入密码） |
| `quit` / `exit` | 退出程序 |

> 支持一行输入多个条码，如 `001 002 001`。

### 管理员模式

| 命令 | 说明 |
|------|------|
| `prices` | 显示所有商品的名称、条码、价格、库存 |
| `setprice <条码> <新价格>` | 修改指定商品的价格 |
| `itemadd <条码> <名称> <价格>` | 添加新商品（库存默认为0） |
| `itemdel <条码>` | 删除指定商品 |
| `restock <条码> <数量>` | 增加指定商品的库存（进货） |
| `setstock <条码> <数量>` | 直接设置指定商品的库存（盘点） |
| `back` | 退出管理员模式，返回收银员模式 |

---

## 项目结构

```
pos-system/
├── CMakeLists.txt           # CMake 构建配置
├── main.cpp                 # 主程序源代码
├── data/
│   └── items.csv            # 商品数据文件（需自行准备）
├── sales.csv                # 销售记录文件（程序自动生成）
└── README.md                # 本文件
```

---

## 常见问题

### Q1: 编译时报错 `'std::put_time' is not a member of 'std'`
请确保编译器支持 C++17，并在 `CMakeLists.txt` 中设置：
```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### Q2: 运行后提示 `No items loaded. Check data/items.csv`
请检查 `data/items.csv` 是否存在且格式正确（UTF-8 编码，无 BOM）。可参考上方示例创建。

### Q3: 结账时提示库存不足
- 先用管理员模式 `restock` 或 `setstock` 设置足够库存。
- 确保 `data/items.csv` 中 `stock` 列不为空或0。
- 若之前手动编辑过文件，注意不要遗漏 `stock` 列。

### Q4: `newday` 后销售记录仍显示同一天？
`newday` 会将内部营业日期+1，但不会修改系统时钟。使用 `sales` 查看时应指定新日期（如 `sales 2026-09-27`），或省略参数查看当前营业日。

### Q5: 如何重置所有数据？
- 删除 `sales.csv`（销售记录重置）
- 恢复 `data/items.csv` 为初始状态（可备份原始文件）

---

## License

本项目仅供学习交流使用。