eRPC is a fast and general-purpose RPC library for datacenter networks.
Our NSDI 2019 [paper](https://www.usenix.org/system/files/nsdi19-kalia.pdf)
describes the system in detail.
[Documentation](http://www.cs.cmu.edu/~akalia/erpc_doc) is available online.

Some highlights:
 * Multiple supported networks: Ethernet, InfiniBand, and RoCE
 * Low latency: 2.3 microseconds round-trip RPC latency with UDP over Ethernet
 * Performance for small 32-byte RPCs: ~10M RPCs/sec with one CPU core,
   60--80M RPCs/sec with one NIC.
 * Bandwidth for large RPC: 75 Gbps on one connection (one CPU core at server
   and client) for 8 MB RPCs
 * Scalability: 20000 RPC sessions per server
 * End-to-end congestion control that tolerates 100-way incasts
 * Nested RPCs, and long-running background RPCs
 * A port of [Raft](https://github.com/willemt/raft) as an example. Our 3-way
   replication latency is 5.3 microseconds with traditional UDP over Ethernet.

## Requirements
 * Toolchain: A C++11 compiler and CMake 2.8+
 * See `scripts/packages/` for required software packages for your distro.
 * The latest `rdma_core`, preferably installed from source
 * For non-Mellanox DPDK-compatible NICs, a system-wide installation from DPDK
   19.11.x LTS sources (i.e., `sudo make install T=x86_64-native-linuxapp-gcc
   DESTDIR=/usr`). Other DPDK versions are not supported.
 * NICs: Fast (10 GbE+) NICs are needed for good performance. eRPC works best
   with Mellanox Ethernet and InfiniBand NICs. Any DPDK-capable NICs
   also work well.
 * System configuration:
   * At least 1024 huge pages on every NUMA node, and unlimited SHM limits
   * On a machine with `n` eRPC processes, eRPC uses kernel UDP ports `{31850,
     ..., 31850 + n - 1}.` These ports should be open on the management
     network. See `scripts/firewalld/erpc_firewall.sh` for systems running
     `firewalld`.

## eRPC quickstart
 * Build and run the test suite:
   `cmake . -DPERF=OFF -DTRANSPORT=infiniband; make -j; sudo ctest`.
   * `DPERF=OFF` enables debugging, which greatly reduces performance. Set
     `DPERF=ON` for performance measurements.
   * Here, `infiniband` should be replaced with `raw` for Mellanox Ethernet
     NICs, or `dpdk` for Intel Ethernet NICs.
   * A machine with two ports is needed to run the unit tests if DPDK is chosen.
     Run `scripts/run-tests-dpdk.sh` instead of `ctest`.
 * Run the `hello_world` application:
   * `cd hello_world`
   * Edit the server and client hostnames in `common.h` 
   * Based on the transport that eRPC was compiled for, compile `hello_world`
     using `make infiniband`, `make raw`, or `make dpdk`.
   * Run `./server` at the server, and `./client` at the client
 * Generate the documentation: `doxygen`

## Supported bare-metal NICs:
 * Ethernet/UDP mode:
   * ConnectX-4 or newer Mellanox Ethernet NICs: Use `DTRANSPORT=raw`
   * DPDK-enabled NICs that support flow-director: Use `DTRANSPORT=dpdk`
     * Intel 82599 and Intel X710 NICs have been tested
     * `raw` transport is faster for Mellanox NICs, which also support DPDK
   * DPDK-enabled NICs on Microsoft Azure: Use `-DTRANSPORT=dpdk -DAZURE=on`
   * ConnectX-3 Ethernet NICs are supported in eRPC's RoCE mode
 * RDMA (InfiniBand/RoCE) NICs: Use `DTRANSPORT=infiniband`. Add `DROCE=on`
   if using RoCE.
 * Mellanox drivers optimized specially for eRPC are available in the `drivers`
   directory

## Running eRPC over DPDK on Microsoft Azure VMs

  * eRPC works well on Azure VMs with accelerated networking.

  * Configure two Ubuntu 18.04 VMs as below. Use the same resource group and
    availability zone for both VMs.

    * Uncheck "Accelerated Networking" when launching each VM from the Azure
      portal (e.g., F32s-v2). For now, this VM should have just the control
      network (i.e., `eth0`) and `lo` interfaces.
    * Add a NIC to Azure via the Azure CLI: `az network nic create
      --resource-group <your resource group> --name <a name for the NIC>
      --vnet-name <name of the VMs' virtual network> --subnet default
      --accelerated-networking true --subscription <Azure subscription, if
      any>`
    * Stop the VM launched earlier, and attach the NIC created in the previous
      step to the VM (i.e., in "Networking" -> "Attach network interface").
    * Re-start the VM. It should have a new interface called `eth1`, which eRPC
      will use for DPDK traffic.

  * Prepare DPDK 19.11.5:
    * [rdma-core](https://github.com/linux-rdma/rdma-core) must be installed
      from source. First, install its dependencies listed in rdma-core's README.
      Then, in the `rdma-core` directory:
       * `cmake .`
       * `sudo make install`
    * Install upstream pre-requisite libraries and modules:
       * `sudo apt install make cmake g++ gcc libnuma-dev libibverbs-dev libgflags-dev numactl`
       * `sudo modprobe ib_uverbs`
       * `sudo modprobe mlx4_ib`
    * Download the [DPDK 19.11.x LTS tarball](https://core.dpdk.org/download/) and
      extract it. Other DPDK versions are not supported.
    * Edit `config/common_base` by changing `CONFIG_RTE_IBVERBS_LINK_DLOPEN `, `CONFIG_RTE_LIBRTE_MLX5_PMD` and
      `CONFIG_RTE_LIBRTE_MLX4_PMD` to `y` instead of `n`.
    * Build and install DPDK: `sudo make install T=x86_64-native-linuxapp-gcc
      DESTDIR=/usr`

    * Create hugepages:
```
sudo bash -c "echo 2048 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages"
sudo mkdir /mnt/huge
sudo mount -t hugetlbfs nodev /mnt/huge
```

  * Build eRPC's library and latency benchmark:
```
cmake . -DTRANSPORT=dpdk -DAZURE=on
make
make latency
```

  * Create the file `scripts/autorun_process_file` like below. Here, do not use
    the IP addresses of the accelerated NIC (i.e., not of `eth1`).
```
<Public IPv4 address of VM #1> 31850 0
<Public IPv4 address of VM #2> 31850 0
```

  * Run the eRPC application (the latency benchmark by default):
    * At VM #1: `./scripts/do.sh 0 0`
    * At VM #2: `./scripts/do.sh 1 0`


## Configuring and running the provided applications
 * The `apps` directory contains a suite of benchmarks and examples. The
   instructions below are for this suite of applications. eRPC can also be
   simply linked as a library instead (see `hello_world/` for an example).
 * To build an application, create `scripts/autorun_app_file` and change its
   contents to one of the available directory names in `apps/`. See
   `scripts/example_autorun_app_file` for an example. Then generate a
   Makefile using `cmake . -DPERF=ON -DTRANSPORT=raw/infiniband/dpdk`. 
 * Each application directory in `apps/` contains a config file
   that must specify all flags defined in `apps/apps_common.h`. For example,
   `num_processes` specifies the total number of eRPC processes in the cluster.
 * The URIs of eRPC processes in the cluster are specified in
   `scripts/autorun_process_file`. Each line in this file must be
   `<hostname> <management udp port> <numa_node>`.
 * Run `scripts/do.sh` for each process:
   * With single-CPU machines: `num_processes` machines are needed.
     Run `scripts/do.sh <i> 0` on machine `i` in `{0, ..., num_processes - 1}`.
   * With dual-CPU machines: `num_machines = ceil(num_processes / 2)` machines
     are needed. Run `scripts/do.sh <i> <i % 2>` on machine i in
     `{0, ..., num_machines - 1}`.
 * To automatically run an application at all processes in
   `scripts/autorun_process_file`, run `scripts/run-all.sh`. For some
   applications, statistics generated in a run can be collected and processed
   using `scripts/proc-out.sh`.

## Getting help
 * GitHub issues are preferred over email. Please include the following
   information in the issue:
   * NIC model
   * `rdma_core` version and DPDK version
   * Operating system

## Contact
Anuj Kalia (akalia@cs.cmu.edu)

## License
		Copyright 2018, Carnegie Mellon University

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

            http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

