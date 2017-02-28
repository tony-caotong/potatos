# Thing before others
Clone from potatos, named hally.

# Environment
  * OS: CentOS 7
  * package:
      * ~~dpdk.x86_64~~
      * ~~dpdk-devel.x86_64~~
  * package from source:
      * dpdk-16.07.2

# Logs
### v20170228-002
Useful configurations of DPDK compiling.
	[root]# diff .config .config.orig 
	106c106
	< CONFIG_RTE_LOG_LEVEL=RTE_LOG_DEBUG
	---
	> CONFIG_RTE_LOG_LEVEL=RTE_LOG_INFO
	122c122
	< CONFIG_RTE_LIBRTE_ETHDEV_DEBUG=y
	---
	> CONFIG_RTE_LIBRTE_ETHDEV_DEBUG=n
	148,149c148,149
	< CONFIG_RTE_LIBRTE_IXGBE_DEBUG_INIT=y
	< CONFIG_RTE_LIBRTE_IXGBE_DEBUG_RX=y
	---
	> CONFIG_RTE_LIBRTE_IXGBE_DEBUG_INIT=n
	> CONFIG_RTE_LIBRTE_IXGBE_DEBUG_RX=n
	152c152
	< CONFIG_RTE_LIBRTE_IXGBE_DEBUG_DRIVER=y
	---
	> CONFIG_RTE_LIBRTE_IXGBE_DEBUG_DRIVER=n
	374,375c374,375
	< CONFIG_RTE_LIBRTE_KNI=n
	< CONFIG_RTE_KNI_KMOD=n
	---
	> CONFIG_RTE_LIBRTE_KNI=y
	> CONFIG_RTE_KNI_KMOD=y
	[root]# 
### v20170228-001
Testing with 'Intel Corporation 82599ES 10-Gigabit SFI/SFP+ Network 
Connection (rev 01)', and fixed some errors.

### v20170227-001
formally, name Changed to hally. add tutorial, change path/name things.

### v20170210-001
struct about rte_mbuf and parameter values about mempool create, were described
as commands in codes.

### v20170208-001
At the very beginning, dpdk-2.2.0 was chosen because it was a publish version 
shipped with official CentOS 7.3 . But I don't know why my VM will be stuck on
under function rte_eal_init(), however turn to dpdk-16.07.2 and it works fine.
