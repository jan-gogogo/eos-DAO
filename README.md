# Decentralized Autonomous Organization (DAO)

基于 EOS 智能合约实现的去中心化自治组织系统，采用插件化架构设计，所有功能模块通过社区投票机制决定是否启用。系统支持组织治理、代币发行、NFT 管理、去中心化交易、流动性挖矿等完整的 DeFi 生态功能。

## ✨ 主要特性

- **去中心化治理**：基于代币持有量的投票机制，所有重要决策由社区投票决定
- **插件化架构**：模块化设计，支持灵活安装和配置功能插件
- **权限管理**：细粒度的权限控制系统，确保操作安全可控
- **完整 DeFi 生态**：集成代币、NFT、交易所、流动性挖矿等核心功能
- **Uniswap 式交易**：支持去中心化代币交换和流动性池管理

## 🏗️ 核心功能模块

### 核心合约

- **Organization** (`organization/`) - 组织管理核心合约
  - 组织信息初始化与管理
  - 插件安装与权限配置
  - 代币和 NFT 转账审批

- **Guide** (`guide/`) - 插件中心
  - 插件模板注册与管理
  - 插件市场展示

### 功能插件

- **Voting** (`plugins/voting/`) - 投票系统
  - 提案创建与投票
  - 投票结果执行
  - 可配置的投票参数（支持率、最低参与率、投票时长）

- **Token** (`plugins/token/`) - 代币发行
  - 代币创建与发行
  - 代币转账与销毁
  - 代币余额管理

- **NFT** (`plugins/nft/`) - 非同质化代币
  - NFT 创建与发行
  - NFT 转账与管理

- **NFT Market** (`plugins/nftmarket/`) - NFT 市场
  - NFT 上架与交易
  - 拍卖功能

- **Token Market** (`plugins/tokenmarket/`) - 代币市场
  - 代币交易撮合
  - 订单管理

- **Exchange** (`plugins/exchange/`) - 去中心化交易所
  - 交易对创建
  - 流动性添加/移除
  - EOS/代币/代币间交换

- **Mining** (`plugins/mining/`) - 流动性挖矿
  - 挖矿配置管理
  - 奖励分配

- **Swap** (`swap/`) - 代币交换
  - Uniswap 式自动做市商 (AMM)
  - 流动性池管理
  - 代币交换路由

### 公共库

- **lib/** - 公共工具库
  - `assets.hpp` - 资产相关工具
  - `auths.hpp` - 权限验证工具
  - `consts.hpp` - 常量定义
  - `safemath.hpp` - 安全数学运算
  - `structs.hpp` - 通用数据结构
  - `trxs.hpp` - 交易相关工具

## 📁 项目结构

```
ds/
├── organization/          # 组织管理核心合约
│   ├── include/
│   └── src/
├── guide/                # 插件中心
│   ├── include/
│   └── src/
├── plugins/              # 功能插件
│   ├── voting/          # 投票系统
│   ├── token/           # 代币发行
│   ├── nft/             # NFT 管理
│   ├── nftmarket/       # NFT 市场
│   ├── tokenmarket/     # 代币市场
│   ├── exchange/        # 去中心化交易所
│   └── mining/          # 流动性挖矿
├── swap/                # 代币交换 (AMM)
└── lib/                 # 公共库
```

## 🛠️ 技术栈

- **区块链平台**: EOSIO
- **编程语言**: C++
- **智能合约框架**: EOSIO Contract Development Toolkit (CDT)

## 🚀 快速开始

### 前置要求

- EOSIO 开发环境
- EOSIO Contract Development Toolkit (CDT)
- cleos 命令行工具

### 编译合约

```bash
# 编译组织合约
cd organization
eosio-cpp -I include src/organization.cpp -o organization.wasm

# 编译插件合约
cd plugins/voting
eosio-cpp -I include src/voting.cpp -o voting.wasm
```


## 📝 治理机制

系统采用基于代币持有量的投票机制：

1. **提案创建**：代币持有者可以创建提案
2. **投票阶段**：代币持有者根据持币量进行投票
3. **提案执行**：达到支持率和最低参与率要求的提案可被执行
4. **权限控制**：关键操作需要通过投票授权

所有插件功能都可通过投票机制决定是否启用，确保系统的去中心化和社区自治。

## 📄 提示
该项目只是一个DAO雏形尚未经过线上验证，请勿直接用于生产。
