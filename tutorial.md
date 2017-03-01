# DPI Development Kit 总结
Author: Cao Tong &lt;<tony.caotong@gmail.com>>   
Date: 2017-02-27
## 摘要
主要通过四块内容对当前一段时间内有关DPI的研究工作进行以下总结：
  1. DPI前期整理。
  2. DPDK及技术选型。
  3. 环境搭建。
  4. 一个小demo。

## 前期内容
DPI Development Kit的核心目标是：保持其松散耦合的特点，在面对不同的应用场景时，能够  
快速梳理业务逻辑，完成快速开发上线。并保持易维护，高性能的特性。  
将业务开发与基础设施有效分层，保持DPI基础设施的常规演进与开发迭代。

### 方向目标
  * 合理的功能区分层与功能块抽象。
  * 设置灵活的接口充分保持业务无关性。
  * 保持高性能，且高效率的资源分配。
  * 灵活的系统配置，与状态检测机制。
  * 支持分布式部署，可以利用硬件对性能线性扩展。

### 需求细化
前期主要做了一个需求分析及功能框架设计，参见文档
  * &lt;&lt;需求分析.pdf>>
  * &lt;&lt;需求分析.et>>
  * &lt;&lt;功能框架图.vsdx>>

### 计划与里程碑
略

## DPDK概述
### 为什么选用DPDK来做DPI
DPDK全称Data Plane Development Kit。是Intel在数据时代专为高性能海量网络数据处理而  
发布的一套底层开发框架。主要面向两部分市场，一个是大数据分析的数据采集整合；另一个是  
云计算领域的网络虚拟化。Intel利用DPDK结合其硬件能力，为这两大块内容，提供基础设施支撑。

经过多年的发展，DPDK的稳定性，及其高性能的包转发处理能力已经得到了全球市场的广泛认可。  
各大互联网公司都已采用该技术作为其业务支撑。同时DPDK也是openstack项目的重要底层技术  
之一。DPDK是基于BSD license开源的，且Intel作为其背后的强大技术支撑。业内大多数DPI  
厂商目前也在使用DPDK。

### DPDK可以为DPI提供哪些能力
  * 通过UIO采用poll模式提供高效率的收包能力，从网卡硬件至用户态内存。
  * 框架集成了CPU CORE绑定能力，并且能够针对收包队列进行映射。
  * 采用大页内存，内存池等机制，优化内存命中，提高内存效率。
  * 提供hash，无所队列等高效数据结构。
  * 其他

### DPDK相关资料
  * __官方网站：__http://dpdk.org
  * __文档：__文档很详细深度也很深，只看官方文档基本就够了，但是对基础知识的要求稍微  
  高一些。http://dpdk.org/doc/guides/  
  * __API：__API都是从源码自动导出的，与看头文件没有区别。http://dpdk.org/doc/api/  
  * __源码: __没有比源码更好的文档了，使用中遇见任何不理解的地方个人建议都应该去读它的  
  源码。此外，源码中有大量丰富的例子，都必须读。 http://dpdk.org/download  
  * __中文资料：__全文讲内部机制与知识背景，对接口的使用没有指导意义，但是可以了解内容原理。
  官方文档中也包含了其大部分内容。[<<深入浅出dpdk>>](https://www.amazon.cn/dp/B01FQ9SMZO/ref=sr_1_1?ie=UTF8&qid=1488189066&sr=8-1&keywords=%E6%B7%B1%E5%85%A5%E6%B5%85%E5%87%BAdpdk)
  * __我的笔记：__都是用来自己看的，可读性比较差，可以适当参考，也许有错误，欢迎指出。
  http://www.cnblogs.com/hugetong/category/890194.html

### 技术选型
  * x86_64硬件架构。
  * 基于intel芯片的采集网卡。
  * CentOS/Redhat 7，原则上只对systemd，libc产生依赖。
  * 内核：kernel-3.10.0-514.6.1.el7.x86_64
  * SDK：dpdk-stable-16.07.2

## 环境搭建
### 环境准备
使用主机dpitest(172.168.10.54), 配备一块万兆网卡，作为实验环境。
  * 四核超线程CPU，Intel(R) Core(TM) i7-6700 CPU @ 3.40GHz
  * 16GB 内存
  * 网卡：Intel Corporation 82599ES 10-Gigabit SFI/SFP+ Network Connection (rev 01)
  * 操作系统：CentOS Linux release 7.3.1611 (Core) [安装时选择最小安装]
  * kernel: Linux dpitest 3.10.0-514.el7.x86_64 [系统安装初始内核]

### 搭建步骤
#### 1. 操作系统安装
  * 常规安装，安装时选择最小安装，理论上不最小也可以，应该没啥区别。安装完成后，为管理网络  
设置IP，启动ssh服务。
  * 安装基础包  
  >     yum install vim  
  >     yum install gcc  
  >     yum install kernel-devel  

#### 2. 系统设置
  * 设置大页内存
  > \# 调整内核参数  
  >     default_hugepagesz=1G hugepagesz=1G hugepages=8  
  > \# 设置挂载点  
  >     mkdir /mnt/huge
  > \# 添加以下内容至文件/etc/fstab
  >     nodev   /mnt/huge  hugetlbfs  pagesize=1GB  0 0
  >
  * 设置孤立核心
  > \# 调整内核参数  
  >     isolcpus=1,2,3,4,5,6,7

#### 3. 安装DPDK
  * 将源码包dpdk-16.07.2.tar.xz拷贝至工作目录/home/tong/，并解压。
  * 设置环境变量。
  >     RTE_SDK=/home/tong/dpdk-stable-16.07.2  
  >     RTE_TARGET=x86_64-native-linuxapp-gcc  
  * 配置编译选项
  > \# 进入RTE_SDK目录  
  >     make config T=$RTE_TARGET O=$RTE_TARGET  
  > \# 编辑$RTE_SDK/$RTE_TARGET/.config，禁用KNI模块  
  >     CONFIG_RTE_LIBRTE_KNI=n  
  >     CONFIG_RTE_KNI_KMOD=n

  * 编译
  > \# 进入$RTE_TARGET目录  
  >     make  
  > \# 编译DEBUG版本
  >     make EXTRA_CFLAGS="-O0 -g" V=y D=y
  > \# 修改.config可以开启trance
  >     CONFIG_RTE_LIBRTE_ETHDEV_DEBUG=y
  * 安装
  >     make install DESTDIR=/dpdk prefix=/  
  > 设置环境变量，将如下内容，添加至 ~/.bash_profile  
  >>     export DPDK_ROOT=/dpdk  
  >>     export RTE_SDK=$DPDK_ROOT/share/dpdk/  
  >>     export RTE_TARGET=x86_64-native-linuxapp-gcc/    
  >>     export PATH=$PATH:$DPDK_ROOT/bin/:$DPDK_ROOT/sbin/  
  >
  > \# 创建脚本env.sh在/dpdk目录  
  > cat env.sh
  >>     #! /bin/bash
  >>     
  >>     modprobe uio  
  >>     insmod /dpdk/lib/modules/\`uname -r\`/extra/dpdk/igb_uio.ko
  >>     /dpdk/sbin/dpdk-devbind -b igb_uio enp1s0f1  
  >>

  > \# 添加进rc.local  
  >     echo "/bin/bash /dpdk/env.sh" >> /etc/rc.local  
  >     chmod a+x /etc/rc.d/rc.local  

### demo 编译运行
#### 改造dap source
demo主要用于演示与功能验证，故快速集成了dap代码中的decoder模块。由于dapcomm库中  
集成一部分框架代码，无法成功与demo程序进行编译链接。故做了一部分改造。
  * checkout dapcomm与dapprotocol两部分代码至dpitest主机。
  * 修改dapprotocol/Makefile
  > \# 注释掉libsources变量的以下四部分
  >     $(wildcard decinterface/*.cpp) \
  >     $(wildcard interface/pkt/*.cpp) \
  >     $(wildcard cdr/common/*.cpp) \
  >     $(wildcard cdr/business/*.cpp)
  * 修改dapcomm/Makefile
  > \# 注释掉libcsources变量的以下部分
  >     lch/license/license_public.c \
  >     $(wildcard lch/apframe/*.c) \
  >     $(wildcard lch/adapter/*.c) \
  >     $(wildcard lch/sdtp/*.c) \
  >     lch/cmdsvr/cmdsvr.c \
  > \# 注释掉libcppsources变量的以下部分  
  >     $(wildcard lch/userinfo/*.cpp) \
  >     $(wildcard lch/cpputil/*.cpp)

  * 安装dap的依赖
  >     yum install libpcap-devel
  >     yum install zlib-devel
  >     yum install bzip2-devel
  >     yum install gcc-c++
  >     yum install curl-devel
  >     yum install libevent-devel
  >     yum install bison
  >     yum install flex

  * 编译
  >     cd dapcomm/
  >     make libdapcomm.so
  >     cd dapprotocol
  >     make libdapprotocol.so

#### 编译demo程序
  * 将hally源码拷贝至dpitest /home/tong/目录
  * 配置
  > \# 在hally/sdk/下创建到dapsrc的链接
  >     ln -s /home/tong/dapsrc/ dap_root
  * 编译
  >     make

#### 运行demo程序
  >     # 启动
  >     ./build/hally -l7,6,5,4,3,2,1  
  >     # 状态请求
  >     kill -USR1 `pidof hally`


     
***
The End
