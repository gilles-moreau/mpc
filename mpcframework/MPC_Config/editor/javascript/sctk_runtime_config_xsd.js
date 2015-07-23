var sctk_runtime_config_xsd = "<?xml version='1.0'?>\
<!--\
############################# MPC License ##############################\
# Wed Nov 19 15:19:19 CET 2008                                         #\
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #\
#                                                                      #\
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #\
# This file is part of the MPC Runtime.                                #\
#                                                                      #\
# This software is governed by the CeCILL-C license under French law   #\
# and abiding by the rules of distribution of free software.  You can  #\
# use, modify and/ or redistribute the software under the terms of     #\
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #\
# following URL http://www.cecill.info.                                #\
#                                                                      #\
# The fact that you are presently reading this means that you have     #\
# had knowledge of the CeCILL-C license and that you accept its        #\
# terms.                                                               #\
#                                                                      #\
# Authors:                                                             #\
#   - VALAT Sebastien sebastien.valat@cea.fr                           #\
#   - AUTOMATIC GENERATION                                             #\
#                                                                      #\
########################################################################\
-->\
<xs:schema xmlns:xs='http://www.w3.org/2001/XMLSchema'>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_allocator'>\
<xs:all>\
<xs:element minOccurs='0' name='numa_migration' type='xs:boolean'/>\
<xs:element minOccurs='0' name='realloc_factor' type='xs:integer'/>\
<xs:element minOccurs='0' name='realloc_threashold'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='numa' type='xs:boolean'/>\
<xs:element minOccurs='0' name='strict' type='xs:boolean'/>\
<xs:element minOccurs='0' name='keep_mem'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='keep_max'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_launcher'>\
<xs:all>\
<xs:element minOccurs='0' name='verbosity' type='xs:integer'/>\
<xs:element minOccurs='0' name='banner' type='xs:boolean'/>\
<xs:element minOccurs='0' name='autokill' type='xs:integer'/>\
<xs:element minOccurs='0' name='user_launchers' type='xs:string'/>\
<xs:element minOccurs='0' name='keep_rand_addr' type='xs:boolean'/>\
<xs:element minOccurs='0' name='disable_rand_addr' type='xs:boolean'/>\
<xs:element minOccurs='0' name='disable_mpc' type='xs:boolean'/>\
<xs:element minOccurs='0' name='thread_init'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='nb_task' type='xs:integer'/>\
<xs:element minOccurs='0' name='nb_process' type='xs:integer'/>\
<xs:element minOccurs='0' name='nb_processor' type='xs:integer'/>\
<xs:element minOccurs='0' name='nb_node' type='xs:integer'/>\
<xs:element minOccurs='0' name='launcher' type='xs:string'/>\
<xs:element minOccurs='0' name='max_try' type='xs:integer'/>\
<xs:element minOccurs='0' name='vers_details' type='xs:boolean'/>\
<xs:element minOccurs='0' name='profiling' type='xs:string'/>\
<xs:element minOccurs='0' name='enable_smt' type='xs:boolean'/>\
<xs:element minOccurs='0' name='share_node' type='xs:boolean'/>\
<xs:element minOccurs='0' name='restart' type='xs:boolean'/>\
<xs:element minOccurs='0' name='checkpoint' type='xs:boolean'/>\
<xs:element minOccurs='0' name='migration' type='xs:boolean'/>\
<xs:element minOccurs='0' name='report' type='xs:boolean'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_debugger'>\
<xs:all>\
<xs:element minOccurs='0' name='colors' type='xs:boolean'/>\
<xs:element minOccurs='0' name='max_filename_size' type='xs:integer'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_net_driver_topological'>\
<xs:all>\
<xs:element minOccurs='0' name='dummy' type='xs:integer'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_net_driver_infiniband'>\
<xs:all>\
<xs:element minOccurs='0' name='pkey' type='xs:string'/>\
<xs:element minOccurs='0' name='network_type' type='xs:integer'/>\
<xs:element minOccurs='0' name='adm_port' type='xs:integer'/>\
<xs:element minOccurs='0' name='verbose_level' type='xs:integer'/>\
<xs:element minOccurs='0' name='eager_limit' type='xs:integer'/>\
<xs:element minOccurs='0' name='buffered_limit' type='xs:integer'/>\
<xs:element minOccurs='0' name='qp_tx_depth' type='xs:integer'/>\
<xs:element minOccurs='0' name='qp_rx_depth' type='xs:integer'/>\
<xs:element minOccurs='0' name='cq_depth' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_sg_sq' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_sg_rq' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_inline' type='xs:integer'/>\
<xs:element minOccurs='0' name='rdma_resizing' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_rdma_connections' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_rdma_resizing' type='xs:integer'/>\
<xs:element minOccurs='0' name='init_ibufs' type='xs:integer'/>\
<xs:element minOccurs='0' name='init_recv_ibufs' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_srq_ibufs_posted' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_srq_ibufs' type='xs:integer'/>\
<xs:element minOccurs='0' name='srq_credit_limit' type='xs:integer'/>\
<xs:element minOccurs='0' name='srq_credit_thread_limit' type='xs:integer'/>\
<xs:element minOccurs='0' name='size_ibufs_chunk' type='xs:integer'/>\
<xs:element minOccurs='0' name='init_mr' type='xs:integer'/>\
<xs:element minOccurs='0' name='steal' type='xs:integer'/>\
<xs:element minOccurs='0' name='quiet_crash' type='xs:integer'/>\
<xs:element minOccurs='0' name='async_thread' type='xs:integer'/>\
<xs:element minOccurs='0' name='wc_in_number' type='xs:integer'/>\
<xs:element minOccurs='0' name='wc_out_number' type='xs:integer'/>\
<xs:element minOccurs='0' name='rdma_depth' type='xs:integer'/>\
<xs:element minOccurs='0' name='rdma_dest_depth' type='xs:integer'/>\
<xs:element minOccurs='0' name='low_memory' type='xs:boolean'/>\
<xs:element minOccurs='0' name='rdvz_protocol' type='user_type_ibv_rdvz_protocol'/>\
<xs:element minOccurs='0' name='rdma_min_size' type='xs:integer'/>\
<xs:element minOccurs='0' name='rdma_max_size' type='xs:integer'/>\
<xs:element minOccurs='0' name='rdma_min_nb' type='xs:integer'/>\
<xs:element minOccurs='0' name='rdma_max_nb' type='xs:integer'/>\
<xs:element minOccurs='0' name='rdma_resizing_min_size' type='xs:integer'/>\
<xs:element minOccurs='0' name='rdma_resizing_max_size' type='xs:integer'/>\
<xs:element minOccurs='0' name='rdma_resizing_min_nb' type='xs:integer'/>\
<xs:element minOccurs='0' name='rdma_resizing_max_nb' type='xs:integer'/>\
<xs:element minOccurs='0' name='size_recv_ibufs_chunk' type='xs:integer'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_ib_global'>\
<xs:all>\
<xs:element minOccurs='0' name='mmu_cache_enabled' type='xs:integer'/>\
<xs:element minOccurs='0' name='mmu_cache_entry_count' type='xs:integer'/>\
<xs:element minOccurs='0' name='mmu_cache_maximum_size'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:simpleType name='user_type_ibv_rdvz_protocol'>\
<xs:restriction base='xs:string'>\
<xs:enumeration value='IBV_RDVZ_WRITE_PROTOCOL'/>\
<xs:enumeration value='IBV_RDVZ_READ_PROTOCOL'/>\
</xs:restriction>\
</xs:simpleType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_net_driver_portals'>\
<xs:all>\
<xs:element minOccurs='0' name='fake_param' type='xs:integer'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_net_driver_tcp'>\
<xs:all>\
<xs:element minOccurs='0' name='tcpoib' type='xs:integer'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_net_driver_tcp_rdma'>\
<xs:all>\
<xs:element minOccurs='0' name='tcpoib' type='xs:integer'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_net_driver'>\
<xs:choice>\
<xs:element name='infiniband' type='user_type_net_driver_infiniband'/>\
<xs:element name='portals' type='user_type_net_driver_portals'/>\
<xs:element name='tcp' type='user_type_net_driver_tcp'/>\
<xs:element name='tcprdma' type='user_type_net_driver_tcp_rdma'/>\
<xs:element name='topological' type='user_type_net_driver_topological'/>\
</xs:choice>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_net_driver_config'>\
<xs:all>\
<xs:element minOccurs='0' name='name' type='xs:string'/>\
<xs:element minOccurs='0' name='driver' type='user_type_net_driver'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_gate_boolean'>\
<xs:all>\
<xs:element minOccurs='0' name='value' type='xs:integer'/>\
<xs:element minOccurs='0' name='gatefunc'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_gate_probabilistic'>\
<xs:all>\
<xs:element minOccurs='0' name='probability' type='xs:integer'/>\
<xs:element minOccurs='0' name='gatefunc'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_gate_min_size'>\
<xs:all>\
<xs:element minOccurs='0' name='value'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='gatefunc'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_gate_max_size'>\
<xs:all>\
<xs:element minOccurs='0' name='value'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='gatefunc'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_gate_message_type'>\
<xs:all>\
<xs:element minOccurs='0' name='process' type='xs:integer'/>\
<xs:element minOccurs='0' name='common' type='xs:integer'/>\
<xs:element minOccurs='0' name='gatefunc'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_gate_user'>\
<xs:all>\
<xs:element minOccurs='0' name='gatefunc'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_net_gate'>\
<xs:choice>\
<xs:element name='boolean' type='user_type_gate_boolean'/>\
<xs:element name='probabilistic' type='user_type_gate_probabilistic'/>\
<xs:element name='minsize' type='user_type_gate_min_size'/>\
<xs:element name='maxsize' type='user_type_gate_max_size'/>\
<xs:element name='msgtype' type='user_type_gate_message_type'/>\
<xs:element name='user' type='user_type_gate_probabilistic'/>\
</xs:choice>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:simpleType name='user_type_rail_topological_polling_level'>\
<xs:restriction base='xs:string'>\
<xs:enumeration value='RAIL_POLL_NONE'/>\
<xs:enumeration value='RAIL_POLL_PU'/>\
<xs:enumeration value='RAIL_POLL_CORE'/>\
<xs:enumeration value='RAIL_POLL_SOCKET'/>\
<xs:enumeration value='RAIL_POLL_NUMA'/>\
<xs:enumeration value='RAIL_POLL_MACHINE'/>\
</xs:restriction>\
</xs:simpleType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_topological_polling'>\
<xs:all>\
<xs:element minOccurs='0' name='range' type='user_type_rail_topological_polling_level'/>\
<xs:element minOccurs='0' name='trigger' type='user_type_rail_topological_polling_level'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_net_rail'>\
<xs:all>\
<xs:element minOccurs='0' name='name' type='xs:string'/>\
<xs:element minOccurs='0' name='priority' type='xs:integer'/>\
<xs:element minOccurs='0' name='device' type='xs:string'/>\
<xs:element minOccurs='0' name='idle_polling' type='user_type_topological_polling'/>\
<xs:element minOccurs='0' name='any_source_polling' type='user_type_topological_polling'/>\
<xs:element minOccurs='0' name='topology' type='xs:string'/>\
<xs:element minOccurs='0' name='ondemand' type='xs:integer'/>\
<xs:element minOccurs='0' name='config' type='xs:string'/>\
<xs:element minOccurs='0' name='gates'>\
<xs:complexType>\
<xs:sequence>\
<xs:element minOccurs='0' maxOccurs='unbounded' name='gate' type='user_type_net_gate'/>\
</xs:sequence>\
</xs:complexType>\
</xs:element>\
<xs:element minOccurs='0' name='subrails'>\
<xs:complexType>\
<xs:sequence>\
<xs:element minOccurs='0' maxOccurs='unbounded' name='subrail' type='user_type_net_rail'/>\
</xs:sequence>\
</xs:complexType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_net_cli_option'>\
<xs:all>\
<xs:element minOccurs='0' name='name' type='xs:string'/>\
<xs:element minOccurs='0' name='rails'>\
<xs:complexType>\
<xs:sequence>\
<xs:element minOccurs='0' maxOccurs='unbounded' name='rail' type='xs:string'/>\
</xs:sequence>\
</xs:complexType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_networks'>\
<xs:all>\
<xs:element minOccurs='0' name='configs'>\
<xs:complexType>\
<xs:sequence>\
<xs:element minOccurs='0' maxOccurs='unbounded' name='config' type='user_type_net_driver_config'/>\
</xs:sequence>\
</xs:complexType>\
</xs:element>\
<xs:element minOccurs='0' name='rails'>\
<xs:complexType>\
<xs:sequence>\
<xs:element minOccurs='0' maxOccurs='unbounded' name='rail' type='user_type_net_rail'/>\
</xs:sequence>\
</xs:complexType>\
</xs:element>\
<xs:element minOccurs='0' name='cli_options'>\
<xs:complexType>\
<xs:sequence>\
<xs:element minOccurs='0' maxOccurs='unbounded' name='cli_option' type='user_type_net_cli_option'/>\
</xs:sequence>\
</xs:complexType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_inter_thread_comm'>\
<xs:all>\
<xs:element minOccurs='0' name='barrier_arity' type='xs:integer'/>\
<xs:element minOccurs='0' name='broadcast_arity_max' type='xs:integer'/>\
<xs:element minOccurs='0' name='broadcast_max_size' type='xs:integer'/>\
<xs:element minOccurs='0' name='broadcast_check_threshold' type='xs:integer'/>\
<xs:element minOccurs='0' name='allreduce_arity_max' type='xs:integer'/>\
<xs:element minOccurs='0' name='allreduce_max_size' type='xs:integer'/>\
<xs:element minOccurs='0' name='allreduce_check_threshold' type='xs:integer'/>\
<xs:element minOccurs='0' name='ALLREDUCE_MAX_SLOT' type='xs:integer'/>\
<xs:element minOccurs='0' name='collectives_init_hook'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_low_level_comm'>\
<xs:all>\
<xs:element minOccurs='0' name='checksum' type='xs:boolean'/>\
<xs:element minOccurs='0' name='send_msg'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='network_mode' type='xs:string'/>\
<xs:element minOccurs='0' name='dyn_reordering' type='xs:boolean'/>\
<xs:element minOccurs='0' name='enable_idle_polling' type='xs:boolean'/>\
<xs:element minOccurs='0' name='ib_global' type='user_type_ib_global'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_mpc'>\
<xs:all>\
<xs:element minOccurs='0' name='log_debug' type='xs:boolean'/>\
<xs:element minOccurs='0' name='hard_checking' type='xs:boolean'/>\
<xs:element minOccurs='0' name='buffering' type='xs:boolean'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_collectives_intra'>\
<xs:all>\
<xs:element minOccurs='0' name='barrier_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='bcast_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='allgather_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='allgatherv_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='alltoall_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='alltoallv_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='alltoallw_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='gather_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='gatherv_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='scatter_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='scatterv_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='scan_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='exscan_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='reduce_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='allreduce_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='reduce_scatter_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='reduce_scatter_block_intra'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_collectives_inter'>\
<xs:all>\
<xs:element minOccurs='0' name='barrier_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='bcast_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='allgather_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='allgatherv_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='alltoall_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='alltoallv_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='alltoallw_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='gather_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='gatherv_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='scatter_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='scatterv_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='reduce_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='allreduce_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='reduce_scatter_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
<xs:element minOccurs='0' name='reduce_scatter_block_inter'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_progress_thread'>\
<xs:all>\
<xs:element minOccurs='0' name='use_progress_thread' type='xs:integer'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_openmp'>\
<xs:all>\
<xs:element minOccurs='0' name='vp' type='xs:integer'/>\
<xs:element minOccurs='0' name='schedule' type='xs:string'/>\
<xs:element minOccurs='0' name='nb_threads' type='xs:integer'/>\
<xs:element minOccurs='0' name='adjustment' type='xs:boolean'/>\
<xs:element minOccurs='0' name='proc_bind' type='xs:boolean'/>\
<xs:element minOccurs='0' name='nested' type='xs:boolean'/>\
<xs:element minOccurs='0' name='stack_size' type='xs:integer'/>\
<xs:element minOccurs='0' name='wait_policy' type='xs:integer'/>\
<xs:element minOccurs='0' name='thread_limit' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_active_levels' type='xs:integer'/>\
<xs:element minOccurs='0' name='tree' type='xs:string'/>\
<xs:element minOccurs='0' name='max_threads' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_alive_for_dyn' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_alive_for_guided' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_alive_sections' type='xs:integer'/>\
<xs:element minOccurs='0' name='max_alive_single' type='xs:integer'/>\
<xs:element minOccurs='0' name='warn_nested' type='xs:boolean'/>\
<xs:element minOccurs='0' name='mode' type='xs:string'/>\
<xs:element minOccurs='0' name='affinity' type='xs:string'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_profiler'>\
<xs:all>\
<xs:element minOccurs='0' name='file_prefix' type='xs:string'/>\
<xs:element minOccurs='0' name='append_date' type='xs:boolean'/>\
<xs:element minOccurs='0' name='color_stdout' type='xs:boolean'/>\
<xs:element minOccurs='0' name='level_colors'>\
<xs:complexType>\
<xs:sequence>\
<xs:element minOccurs='0' maxOccurs='unbounded' name='level' type='xs:string'/>\
</xs:sequence>\
</xs:complexType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='user_type_thread'>\
<xs:all>\
<xs:element minOccurs='0' name='spin_delay' type='xs:integer'/>\
<xs:element minOccurs='0' name='interval' type='xs:integer'/>\
<xs:element minOccurs='0' name='kthread_stack_size'>\
<xs:simpleType>\
<xs:restriction base='xs:string'>\
<xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
</xs:restriction>\
</xs:simpleType>\
</xs:element>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='modules'>\
<xs:all>\
<xs:element minOccurs='0' name='allocator' type='user_type_allocator'/>\
<xs:element minOccurs='0' name='launcher' type='user_type_launcher'/>\
<xs:element minOccurs='0' name='debugger' type='user_type_debugger'/>\
<xs:element minOccurs='0' name='inter_thread_comm' type='user_type_inter_thread_comm'/>\
<xs:element minOccurs='0' name='low_level_comm' type='user_type_low_level_comm'/>\
<xs:element minOccurs='0' name='mpc' type='user_type_mpc'/>\
<xs:element minOccurs='0' name='collectives_intra' type='user_type_collectives_intra'/>\
<xs:element minOccurs='0' name='collectives_inter' type='user_type_collectives_inter'/>\
<xs:element minOccurs='0' name='progress_thread' type='user_type_progress_thread'/>\
<xs:element minOccurs='0' name='openmp' type='user_type_openmp'/>\
<xs:element minOccurs='0' name='profiler' type='user_type_profiler'/>\
<xs:element minOccurs='0' name='thread' type='user_type_thread'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='static_part_mapping_selector_env'>\
<xs:simpleContent>\
<xs:extension base='xs:string'>\
<xs:attribute name='name' type='xs:string' use='required'/>\
</xs:extension>\
</xs:simpleContent>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='static_part_mapping_selectors'>\
<xs:sequence>\
<xs:element name='env' type='static_part_mapping_selector_env' minOccurs='0' maxOccurs='unbounded'/>\
</xs:sequence>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='static_part_mapping_profiles'>\
<xs:sequence>\
<xs:element name='profile' type='xs:string' minOccurs='0' maxOccurs='unbounded'/>\
</xs:sequence>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='static_part_mapping'>\
<xs:all>\
<xs:element name='selectors' type='static_part_mapping_selectors' minOccurs='0'/>\
<xs:element name='profiles' type='static_part_mapping_profiles' minOccurs='0'/>\
</xs:all>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='static_part_mappings'>\
<xs:sequence>\
<xs:element name='mapping' type='static_part_mapping' minOccurs='0' maxOccurs='unbounded'/>\
</xs:sequence>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='static_part_profile'>\
<xs:sequence>\
<xs:element name='name' type='xs:string'/>\
<xs:element name='modules' type='modules' minOccurs='0'/>\
<xs:element name='networks' type='user_type_networks' minOccurs='0'/>\
</xs:sequence>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='static_part_profiles'>\
<xs:sequence>\
<xs:element name='profile' type='static_part_profile' minOccurs='0' maxOccurs='unbounded'/>\
</xs:sequence>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:complexType name='static_part_mpc'>\
<xs:sequence>\
<xs:element name='profiles' type='static_part_profiles' minOccurs='0'/>\
<xs:element name='mappings' type='static_part_mappings' minOccurs='0'/>\
</xs:sequence>\
</xs:complexType>\
<!-- ********************************************************* -->\
<xs:element name='mpc' type='static_part_mpc'/>\
</xs:schema>\
";
