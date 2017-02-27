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
### v20170227-001
formally, name Changed to hally. add tutorial, change path/name things.

### v20170210-001
struct about rte_mbuf and parameter values about mempool create, were described
as commands in codes.

### v20170208-001
At the very beginning, dpdk-2.2.0 was chosen because it was a publish version 
shipped with official CentOS 7.3 . But I don't know why my VM will be stuck on
under function rte_eal_init(), however turn to dpdk-16.07.2 and it works fine.
