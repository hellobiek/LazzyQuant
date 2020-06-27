# LazzyQuant
### Quantative trading platform for geeks.

## 主要功能:
1. 接收市场行情数据, 生成K线数据并保存至数据库  
2. 根据交易策略, 寻找获利机会并做出开仓/平仓, 止损/止盈等决策  
3. 根据决策自动执行报, 撤单操作  
4. 支持上期CTP交易接口  
5. 支持多种数据源用于盘后复盘  
6. 支持美式/欧式期权盘中实时定价 (基于二叉树模型)  
7. 支持各种套利策略  
8. 支持预埋单, 组合单  
9. 支持兼容MQL5语法的策略设计  

## 开发与测试环境:
&emsp;&emsp;Visual Studio 2015/2017 (Windows)  
&emsp;&emsp;GCC 7.3/9.2 (Linux)  
&emsp;&emsp;Qt 5.12.9 (最低要求5.10)  
&emsp;&emsp;Boost 1.68  
&emsp;&emsp;D-Bus 1.10.30  
&emsp;&emsp;MySQL 5.7.30  

## 在Linux下编译的步骤
git clone https://github.com/zc8424/LazzyQuant.git  
cd LazzyQuant  
qmake  
make -j4  

&emsp;注:  

1. 编译时找不到 boost 库的头文件可通过以下命令安装  
RedHat/CentOS: sudo yum install boost  
Debian/Ubuntu: sudo apt-get install libboost-dev  
2. Linux发行版自带的Qt版本如果太低(<5.10)，可安装Qt官网上的最新版本  

#### 部分开发文档存放在LazzyDocument仓库中  
