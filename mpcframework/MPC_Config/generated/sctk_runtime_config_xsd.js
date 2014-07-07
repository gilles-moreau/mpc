-e var sctk_runtime_config_xsd = "<?xml version='1.0'?>\
-e <!--\
-e ############################# MPC License ##############################\
-e # Wed Nov 19 15:19:19 CET 2008                                         #\
-e # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #\
-e #                                                                      #\
-e # IDDN.FR.001.230040.000.S.P.2007.000.10000                            #\
-e # This file is part of the MPC Runtime.                                #\
-e #                                                                      #\
-e # This software is governed by the CeCILL-C license under French law   #\
-e # and abiding by the rules of distribution of free software.  You can  #\
-e # use, modify and/ or redistribute the software under the terms of     #\
-e # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #\
-e # following URL http://www.cecill.info.                                #\
-e #                                                                      #\
-e # The fact that you are presently reading this means that you have     #\
-e # had knowledge of the CeCILL-C license and that you accept its        #\
-e # terms.                                                               #\
-e #                                                                      #\
-e # Authors:                                                             #\
-e #   - VALAT Sebastien sebastien.valat@cea.fr                           #\
-e #   - AUTOMATIC GENERATION                                             #\
-e #                                                                      #\
-e ########################################################################\
-e -->\
-e <xs:schema xmlns:xs='http://www.w3.org/2001/XMLSchema'>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_allocator'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='numa_migration' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='realloc_factor' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='realloc_threashold'>\
-e <xs:simpleType>\
-e <xs:restriction base='xs:string'>\
-e <xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
-e </xs:restriction>\
-e </xs:simpleType>\
-e </xs:element>\
-e <xs:element minOccurs='0' name='numa' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='strict' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='keep_mem'>\
-e <xs:simpleType>\
-e <xs:restriction base='xs:string'>\
-e <xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
-e </xs:restriction>\
-e </xs:simpleType>\
-e </xs:element>\
-e <xs:element minOccurs='0' name='keep_max'>\
-e <xs:simpleType>\
-e <xs:restriction base='xs:string'>\
-e <xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
-e </xs:restriction>\
-e </xs:simpleType>\
-e </xs:element>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_launcher'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='verbosity' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='banner' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='autokill' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='user_launchers' type='xs:string'/>\
-e <xs:element minOccurs='0' name='keep_rand_addr' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='disable_rand_addr' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='disable_mpc' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='thread_init'>\
-e <xs:simpleType>\
-e <xs:restriction base='xs:string'>\
-e <xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
-e </xs:restriction>\
-e </xs:simpleType>\
-e </xs:element>\
-e <xs:element minOccurs='0' name='nb_task' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='nb_process' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='nb_processor' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='nb_node' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='launcher' type='xs:string'/>\
-e <xs:element minOccurs='0' name='max_try' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='vers_details' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='profiling' type='xs:string'/>\
-e <xs:element minOccurs='0' name='enable_smt' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='share_node' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='restart' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='checkpoint' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='migration' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='report' type='xs:boolean'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_debugger'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='colors' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='max_filename_size' type='xs:integer'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_net_driver_infiniband'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='network_type' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='adm_port' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='verbose_level' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='eager_limit' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='buffered_limit' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='qp_tx_depth' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='qp_rx_depth' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='cq_depth' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_sg_sq' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_sg_rq' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_inline' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='rdma_resizing' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_rdma_connections' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_rdma_resizing' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='init_ibufs' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='init_recv_ibufs' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_srq_ibufs_posted' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_srq_ibufs' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='srq_credit_limit' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='srq_credit_thread_limit' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='size_ibufs_chunk' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='init_mr' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='size_mr_chunk' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='mmu_cache_enabled' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='mmu_cache_entries' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='steal' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='quiet_crash' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='async_thread' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='wc_in_number' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='wc_out_number' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='rdma_depth' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='rdma_dest_depth' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='low_memory' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='rdvz_protocol' type='user_type_ibv_rdvz_protocol'/>\
-e <xs:element minOccurs='0' name='rdma_min_size' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='rdma_max_size' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='rdma_min_nb' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='rdma_max_nb' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='rdma_resizing_min_size' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='rdma_resizing_max_size' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='rdma_resizing_min_nb' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='rdma_resizing_max_nb' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='size_recv_ibufs_chunk' type='xs:integer'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:simpleType name='user_type_ibv_rdvz_protocol'>\
-e <xs:restriction base='xs:string'>\
-e <xs:enumeration value='IBV_RDVZ_WRITE_PROTOCOL'/>\
-e <xs:enumeration value='IBV_RDVZ_READ_PROTOCOL'/>\
-e </xs:restriction>\
-e </xs:simpleType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_net_driver_tcp'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='fake_param' type='xs:integer'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_net_driver'>\
-e <xs:choice>\
-e <xs:element name='infiniband' type='user_type_net_driver_infiniband'/>\
-e <xs:element name='tcp' type='user_type_net_driver_tcp'/>\
-e <xs:element name='tcpoib' type='user_type_net_driver_tcp'/>\
-e </xs:choice>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_net_driver_config'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='name' type='xs:string'/>\
-e <xs:element minOccurs='0' name='driver' type='user_type_net_driver'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_net_cli_option'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='name' type='xs:string'/>\
-e <xs:element minOccurs='0' name='rails'>\
-e <xs:complexType>\
-e <xs:sequence>\
-e <xs:element minOccurs='0' maxOccurs='unbounded' name='rail' type='xs:string'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e </xs:element>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_net_rail'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='name' type='xs:string'/>\
-e <xs:element minOccurs='0' name='device' type='xs:string'/>\
-e <xs:element minOccurs='0' name='topology' type='xs:string'/>\
-e <xs:element minOccurs='0' name='config' type='xs:string'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_networks'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='configs'>\
-e <xs:complexType>\
-e <xs:sequence>\
-e <xs:element minOccurs='0' maxOccurs='unbounded' name='config' type='user_type_net_driver_config'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e </xs:element>\
-e <xs:element minOccurs='0' name='rails'>\
-e <xs:complexType>\
-e <xs:sequence>\
-e <xs:element minOccurs='0' maxOccurs='unbounded' name='rail' type='user_type_net_rail'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e </xs:element>\
-e <xs:element minOccurs='0' name='cli_options'>\
-e <xs:complexType>\
-e <xs:sequence>\
-e <xs:element minOccurs='0' maxOccurs='unbounded' name='cli_option' type='user_type_net_cli_option'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e </xs:element>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_inter_thread_comm'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='barrier_arity' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='broadcast_arity_max' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='broadcast_max_size' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='broadcast_check_threshold' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='allreduce_arity_max' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='allreduce_max_size' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='allreduce_check_threshold' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='ALLREDUCE_MAX_SLOT' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='collectives_init_hook'>\
-e <xs:simpleType>\
-e <xs:restriction base='xs:string'>\
-e <xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
-e </xs:restriction>\
-e </xs:simpleType>\
-e </xs:element>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_low_level_comm'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='checksum' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='send_msg'>\
-e <xs:simpleType>\
-e <xs:restriction base='xs:string'>\
-e <xs:pattern value='[A-Za-z_][0-9A-Za-z_]*'/>\
-e </xs:restriction>\
-e </xs:simpleType>\
-e </xs:element>\
-e <xs:element minOccurs='0' name='network_mode' type='xs:string'/>\
-e <xs:element minOccurs='0' name='dyn_reordering' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='enable_idle_polling' type='xs:boolean'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_mpc'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='log_debug' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='hard_checking' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='buffering' type='xs:boolean'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_openmp'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='vp' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='schedule' type='xs:string'/>\
-e <xs:element minOccurs='0' name='nb_threads' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='adjustment' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='proc_bind' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='nested' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='stack_size' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='wait_policy' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='thread_limit' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_active_levels' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='tree' type='xs:string'/>\
-e <xs:element minOccurs='0' name='max_threads' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_alive_for_dyn' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_alive_for_guided' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_alive_sections' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='max_alive_single' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='warn_nested' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='mode' type='xs:string'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_profiler'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='file_prefix' type='xs:string'/>\
-e <xs:element minOccurs='0' name='append_date' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='color_stdout' type='xs:boolean'/>\
-e <xs:element minOccurs='0' name='level_colors'>\
-e <xs:complexType>\
-e <xs:sequence>\
-e <xs:element minOccurs='0' maxOccurs='unbounded' name='level' type='xs:string'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e </xs:element>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='user_type_thread'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='spin_delay' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='interval' type='xs:integer'/>\
-e <xs:element minOccurs='0' name='kthread_stack_size'>\
-e <xs:simpleType>\
-e <xs:restriction base='xs:string'>\
-e <xs:pattern value='[0-9]+[ ]?[K|M|G|T|P]?B'/>\
-e </xs:restriction>\
-e </xs:simpleType>\
-e </xs:element>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='modules'>\
-e <xs:all>\
-e <xs:element minOccurs='0' name='allocator' type='user_type_allocator'/>\
-e <xs:element minOccurs='0' name='launcher' type='user_type_launcher'/>\
-e <xs:element minOccurs='0' name='debugger' type='user_type_debugger'/>\
-e <xs:element minOccurs='0' name='inter_thread_comm' type='user_type_inter_thread_comm'/>\
-e <xs:element minOccurs='0' name='low_level_comm' type='user_type_low_level_comm'/>\
-e <xs:element minOccurs='0' name='mpc' type='user_type_mpc'/>\
-e <xs:element minOccurs='0' name='openmp' type='user_type_openmp'/>\
-e <xs:element minOccurs='0' name='profiler' type='user_type_profiler'/>\
-e <xs:element minOccurs='0' name='thread' type='user_type_thread'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='static_part_mapping_selector_env'>\
-e <xs:simpleContent>\
-e <xs:extension base='xs:string'>\
-e <xs:attribute name='name' type='xs:string' use='required'/>\
-e </xs:extension>\
-e </xs:simpleContent>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='static_part_mapping_selectors'>\
-e <xs:sequence>\
-e <xs:element name='env' type='static_part_mapping_selector_env' minOccurs='0' maxOccurs='unbounded'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='static_part_mapping_profiles'>\
-e <xs:sequence>\
-e <xs:element name='profile' type='xs:string' minOccurs='0' maxOccurs='unbounded'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='static_part_mapping'>\
-e <xs:all>\
-e <xs:element name='selectors' type='static_part_mapping_selectors' minOccurs='0'/>\
-e <xs:element name='profiles' type='static_part_mapping_profiles' minOccurs='0'/>\
-e </xs:all>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='static_part_mappings'>\
-e <xs:sequence>\
-e <xs:element name='mapping' type='static_part_mapping' minOccurs='0' maxOccurs='unbounded'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='static_part_profile'>\
-e <xs:sequence>\
-e <xs:element name='name' type='xs:string'/>\
-e <xs:element name='modules' type='modules' minOccurs='0'/>\
-e <xs:element name='networks' type='user_type_networks' minOccurs='0'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='static_part_profiles'>\
-e <xs:sequence>\
-e <xs:element name='profile' type='static_part_profile' minOccurs='0' maxOccurs='unbounded'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:complexType name='static_part_mpc'>\
-e <xs:sequence>\
-e <xs:element name='profiles' type='static_part_profiles' minOccurs='0'/>\
-e <xs:element name='mappings' type='static_part_mappings' minOccurs='0'/>\
-e </xs:sequence>\
-e </xs:complexType>\
-e <!-- ********************************************************* -->\
-e <xs:element name='mpc' type='static_part_mpc'/>\
-e </xs:schema>\
";
