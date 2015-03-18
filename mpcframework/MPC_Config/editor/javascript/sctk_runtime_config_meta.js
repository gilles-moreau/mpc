var meta = new Array();
meta.types = {
	allocator : {type: 'struct', name: "allocator", childs: {
		numa_migration: {mode: 'param', name: "numa_migration", type: "bool", doc: "Enable or disable NUMA migration of allocator pages on thread migration.", dflt: "false", },
		realloc_factor: {mode: 'param', name: "realloc_factor", type: "int", doc: "If the new segment is less than N time smaller than factor, realloc will allocate a new segment, otherwise it will keep the same one. Use 1 to force realloc every time (may be slower but consume less memory).", dflt: "2", },
		realloc_threashold: {mode: 'param', name: "realloc_threashold", type: "size", doc: "If the new segment is smaller of N bytes than threashold, realloc will allocate a new segment, otherwise it will keep the same one. Use 0 to force realloc every time (may be slower but consume less memory).", dflt: "50MB", },
		numa: {mode: 'param', name: "numa", type: "bool", doc: "Permit to enable of disable NUMA support in MPC Allocator.", dflt: "true", },
		strict: {mode: 'param', name: "strict", type: "bool", doc: "If true, enable usage of abort() on free error, otherwise try to continue by skipping.", dflt: "false", },
		keep_mem: {mode: 'param', name: "keep_mem", type: "size", doc: "Maximum amount of memory to keep in memory sources (one per NUMA node). Use 0 to disable cache, huge value to keep all.", dflt: "500MB", },
		keep_max: {mode: 'param', name: "keep_max", type: "size", doc: "Maximum size of macro blocs to keep in memory source for reuse. Use 0 to disable cache, huge value to keep all.", dflt: "8MB", },
	}},
	launcher : {type: 'struct', name: "launcher", childs: {
		verbosity: {mode: 'param', name: "verbosity", type: "int", doc: "Default verbosity level from 0 to 3. Can be override by -vv on mpcrun.", dflt: "0", },
		banner: {mode: 'param', name: "banner", type: "bool", doc: "Display the MPC banner at launch time to print some informations about the topology. Can be override by MPC_DISABLE_BANNER.", dflt: "true", },
		autokill: {mode: 'param', name: "autokill", type: "int", doc: "Automatically kill the MPC processes after a given timeout. Use 0 to disable. Can be override by MPC_AUTO_KILL_TIMEOUT.", dflt: "0", },
		user_launchers: {mode: 'param', name: "user_launchers", type: "string", doc: "Permit to extend the launchers available via 'mpcrun -l=...' by providing scripts (named mpcrun_XXXX) in a user directory. Can be override by MPC_USER_LAUNCHERS.", dflt: "default", },
		keep_rand_addr: {mode: 'param', name: "keep_rand_addr", type: "bool", doc: "Activate randomization of base addresses", dflt: "true", },
		disable_rand_addr: {mode: 'param', name: "disable_rand_addr", type: "bool", doc: "Deactivate randomization of base addresses", dflt: "false", },
		disable_mpc: {mode: 'param', name: "disable_mpc", type: "bool", doc: "Do not use mpc for execution (deprecated?)", dflt: "false", },
		thread_init: {mode: 'param', name: "thread_init", type: "funcptr", doc: "Initialize multithreading mode", dflt: "sctk_use_ethread_mxn", },
		nb_task: {mode: 'param', name: "nb_task", type: "int", doc: "Define the number of MPI tasks", dflt: "1", },
		nb_process: {mode: 'param', name: "nb_process", type: "int", doc: "Define the number of MPC processes", dflt: "1", },
		nb_processor: {mode: 'param', name: "nb_processor", type: "int", doc: "Define the number of virtual processors", dflt: "0", },
		nb_node: {mode: 'param', name: "nb_node", type: "int", doc: "Define the number of compute nodes", dflt: "1", },
		launcher: {mode: 'param', name: "launcher", type: "string", doc: "Define which launcher to use", dflt: "none", },
		max_try: {mode: 'param', name: "max_try", type: "int", doc: "Define the max number of tries to access the topology file before failing", dflt: "10", },
		vers_details: {mode: 'param', name: "vers_details", type: "bool", doc: "Print the MPC version number", dflt: "false", },
		profiling: {mode: 'param', name: "profiling", type: "string", doc: "Select the type of outputs for the profiling", dflt: "stdout", },
		enable_smt: {mode: 'param', name: "enable_smt", type: "bool", doc: "Enable usage of hyperthreaded cores if available on current architecture.", dflt: "false", },
		share_node: {mode: 'param', name: "share_node", type: "bool", doc: "Enable the restriction on CPU number to share node", dflt: "false", },
		restart: {mode: 'param', name: "restart", type: "bool", doc: "Restart MPC from a previous checkpoint", dflt: "false", },
		checkpoint: {mode: 'param', name: "checkpoint", type: "bool", doc: "Enable MPC checkpointing", dflt: "false", },
		migration: {mode: 'param', name: "migration", type: "bool", doc: "Enable migration", dflt: "false", },
		report: {mode: 'param', name: "report", type: "bool", doc: "Enable reporting.", dflt: "false", },
	}},
	debugger : {type: 'struct', name: "debugger", childs: {
		colors: {mode: 'param', name: "colors", type: "bool", doc: "Print colored text in terminal", dflt: "true", },
		max_filename_size: {mode: 'param', name: "max_filename_size", type: "int", doc: "", dflt: "1024", },
	}},
	net_driver_infiniband : {type: 'struct', name: "net_driver_infiniband", childs: {
		network_type: {mode: 'param', name: "network_type", type: "int", doc: "Define a network's type (0=signalization, 1=data)", dflt: "0", },
		adm_port: {mode: 'param', name: "adm_port", type: "int", doc: "Defines the port number to use.", dflt: "0", },
		verbose_level: {mode: 'param', name: "verbose_level", type: "int", doc: "Defines the verbose level of the Infiniband interface .", dflt: "0", },
		eager_limit: {mode: 'param', name: "eager_limit", type: "int", doc: "Size of the eager buffers (short messages).", dflt: "12288", },
		buffered_limit: {mode: 'param', name: "buffered_limit", type: "int", doc: "Max size for using the Buffered protocol (message split into several Eager messages).", dflt: "262114", },
		qp_tx_depth: {mode: 'param', name: "qp_tx_depth", type: "int", doc: "Number of entries to allocate in the QP for sending messages. If too low, may cause an QP overrun", dflt: "15000", },
		qp_rx_depth: {mode: 'param', name: "qp_rx_depth", type: "int", doc: "Number of entries to allocate in the QP for receiving messages. Must be 0 if using SRQ", dflt: "0", },
		cq_depth: {mode: 'param', name: "cq_depth", type: "int", doc: "Number of entries to allocate in the CQ. If too low, may cause a CQ overrun", dflt: "40000", },
		max_sg_sq: {mode: 'param', name: "max_sg_sq", type: "int", doc: "Max pending RDMA operations for send", dflt: "4", },
		max_sg_rq: {mode: 'param', name: "max_sg_rq", type: "int", doc: "Max pending RDMA operations for recv", dflt: "4", },
		max_inline: {mode: 'param', name: "max_inline", type: "int", doc: "Max size for inlining messages", dflt: "128", },
		rdma_resizing: {mode: 'param', name: "rdma_resizing", type: "int", doc: "Defines if RDMA connections may be resized.", dflt: "0", },
		max_rdma_connections: {mode: 'param', name: "max_rdma_connections", type: "int", doc: "Number of RDMA buffers allocated for each neighbor", dflt: "0", },
		max_rdma_resizing: {mode: 'param', name: "max_rdma_resizing", type: "int", doc: "Max number of RDMA buffers resizing allowed", dflt: "0", },
		init_ibufs: {mode: 'param', name: "init_ibufs", type: "int", doc: "Max number of Eager buffers to allocate during the initialization step", dflt: "1000", },
		init_recv_ibufs: {mode: 'param', name: "init_recv_ibufs", type: "int", doc: "Defines the number of receive buffers initially allocated. The number is on-the-fly expanded when needed (see init_recv_ibufs_chunk)", dflt: "200", },
		max_srq_ibufs_posted: {mode: 'param', name: "max_srq_ibufs_posted", type: "int", doc: "Max number of Eager buffers which can be posted to the SRQ. This number cannot be higher than the number fixed by the HW", dflt: "1500", },
		max_srq_ibufs: {mode: 'param', name: "max_srq_ibufs", type: "int", doc: "Max number of Eager buffers which can be used by the SRQ. This number is not fixed by the HW", dflt: "1000", },
		srq_credit_limit: {mode: 'param', name: "srq_credit_limit", type: "int", doc: "Min number of free recv Eager buffers before posting a new buffer.", dflt: "500", },
		srq_credit_thread_limit: {mode: 'param', name: "srq_credit_thread_limit", type: "int", doc: "Min number of free recv Eager buffers before the activation of the asynchronous thread. If this thread is activated too many times, the performance may be decreased.", dflt: "100", },
		size_ibufs_chunk: {mode: 'param', name: "size_ibufs_chunk", type: "int", doc: "Number of new buffers allocated when no more buffers are available.", dflt: "100", },
		init_mr: {mode: 'param', name: "init_mr", type: "int", doc: "Number of MMU entries allocated during the MPC initlization.", dflt: "400", },
		steal: {mode: 'param', name: "steal", type: "int", doc: "Defines if the steal in MPI is allowed ", dflt: "2", },
		quiet_crash: {mode: 'param', name: "quiet_crash", type: "int", doc: "Defines if the Infiniband interface must crash quietly.", dflt: "0", },
		async_thread: {mode: 'param', name: "async_thread", type: "int", doc: "Defines if the asynchronous may be started at the MPC initialization.", dflt: "0", },
		wc_in_number: {mode: 'param', name: "wc_in_number", type: "int", doc: "Defines the number of entries for the CQ dedicated to received messages.", dflt: "0", },
		wc_out_number: {mode: 'param', name: "wc_out_number", type: "int", doc: "Defines the number of entries for the CQ dedicated to sent messages.", dflt: "0", },
		rdma_depth: {mode: 'param', name: "rdma_depth", type: "int", doc: "Number of outstanding RDMA reads and atomic operations on the destination QP (to be confirmed)", dflt: "0", },
		rdma_dest_depth: {mode: 'param', name: "rdma_dest_depth", type: "int", doc: "Number of responder resources for handling incoming RDMA reads and atomic operations (to be confirmed)", dflt: "0", },
		low_memory: {mode: 'param', name: "low_memory", type: "bool", doc: "Defines if the low memory mode should be activated", dflt: "false", },
		rdvz_protocol: {mode: 'param', name: "rdvz_protocol", type: "ibv_rdvz_protocol", doc: "Defines the Rendezvous protocol to use (IBV_RDVZ_WRITE_PROTOCOL or IBV_RDVZ_READ_PROTOCOL)", dflt: "IBV_RDVZ_WRITE_PROTOCOL", },
		rdma_min_size: {mode: 'param', name: "rdma_min_size", type: "int", doc: "Defines the minimum size for the Eager RDMA buffers", dflt: "1024", },
		rdma_max_size: {mode: 'param', name: "rdma_max_size", type: "int", doc: "Defines the maximun size for the Eager RDMA buffers", dflt: "4096", },
		rdma_min_nb: {mode: 'param', name: "rdma_min_nb", type: "int", doc: "Defines the minimum number of Eager RDMA buffers", dflt: "8", },
		rdma_max_nb: {mode: 'param', name: "rdma_max_nb", type: "int", doc: "Defines the maximum number of Eager RDMA buffers", dflt: "32", },
		rdma_resizing_min_size: {mode: 'param', name: "rdma_resizing_min_size", type: "int", doc: "Defines the minimum size for the Eager RDMA buffers (resizing)", dflt: "1024", },
		rdma_resizing_max_size: {mode: 'param', name: "rdma_resizing_max_size", type: "int", doc: "Defines the maximum size for the Eager RDMA buffers (resizing)", dflt: "4096", },
		rdma_resizing_min_nb: {mode: 'param', name: "rdma_resizing_min_nb", type: "int", doc: "Defines the minimum number of Eager RDMA buffers (resizing)", dflt: "8", },
		rdma_resizing_max_nb: {mode: 'param', name: "rdma_resizing_max_nb", type: "int", doc: "Defines the maximum number of Eager RDMA buffers (resizing)", dflt: "32", },
		size_recv_ibufs_chunk: {mode: 'param', name: "size_recv_ibufs_chunk", type: "int", doc: "Defines the number of receive buffers allocated on the fly.", dflt: "400", },
	}},
	ib_global : {type: 'struct', name: "ib_global", childs: {
		mmu_cache_enabled: {mode: 'param', name: "mmu_cache_enabled", type: "int", doc: "Defines if the MMU cache is enabled.", dflt: "1", },
		mmu_cache_size_global: {mode: 'param', name: "mmu_cache_size_global", type: "int", doc: "Number of MMU entries allocated when no more MMU entries are available.", dflt: "500", },
	}},
	net_driver_portals : {type: 'struct', name: "net_driver_portals", childs: {
		fake_param: {mode: 'param', name: "fake_param", type: "int", doc: "Fake param.", dflt: "0", },
	}},
	net_driver_tcp : {type: 'struct', name: "net_driver_tcp", childs: {
		tcpoib: {mode: 'param', name: "tcpoib", type: "int", doc: "Enable TCP over Infiniband (if elligible).", dflt: "1", },
	}},
	net_driver_tcp_rdma : {type: 'struct', name: "net_driver_tcp_rdma", childs: {
		tcpoib: {mode: 'param', name: "tcpoib", type: "int", doc: "Enable TCP over Infiniband (if elligible).", dflt: "1", },
	}},
	net_driver : {type: 'union', name: "net_driver", choice: {
		infiniband: {name: "infiniband", type: "net_driver_infiniband"},
		portals: {name: "portals", type: "net_driver_portals"},
		tcp: {name: "tcp", type: "net_driver_tcp"},
		tcprdma: {name: "tcprdma", type: "net_driver_tcp_rdma"},
	}},
	net_driver_config : {type: 'struct', name: "net_driver_config", childs: {
		name: {mode: 'param', name: "name", type: "string", doc: "Name of the driver configuration to be referenced in rail definitions.", dflt: null},
		driver: {mode: 'param', name: "driver", type: "net_driver", doc: "Define the related driver to use and its configuration.", dflt: null},
	}},
	gate_boolean : {type: 'struct', name: "gate_boolean", childs: {
		value: {mode: 'param', name: "value", type: "int", doc: "whereas to accept input messages or not", dflt: "1", },
		gatefunc: {mode: 'param', name: "gatefunc", type: "funcptr", doc: "Function to be called for this gate", dflt: "sctk_rail_gate_boolean", },
	}},
	gate_probabilistic : {type: 'struct', name: "gate_probabilistic", childs: {
		probability: {mode: 'param', name: "probability", type: "int", doc: "Probability to choose this rail in percents (ralatively to this single rail, integer)", dflt: "50", },
		gatefunc: {mode: 'param', name: "gatefunc", type: "funcptr", doc: "Function to be called for this gate", dflt: "sctk_rail_gate_probabilistic", },
	}},
	gate_min_size : {type: 'struct', name: "gate_min_size", childs: {
		value: {mode: 'param', name: "value", type: "size", doc: "Minimum size to choose this rail (with units)", dflt: null},
		gatefunc: {mode: 'param', name: "gatefunc", type: "funcptr", doc: "Function to be called for this gate", dflt: "sctk_rail_gate_minsize", },
	}},
	gate_max_size : {type: 'struct', name: "gate_max_size", childs: {
		value: {mode: 'param', name: "value", type: "size", doc: "Maximum size to choose this rail (with units)", dflt: null},
		gatefunc: {mode: 'param', name: "gatefunc", type: "funcptr", doc: "Function to be called for this gate", dflt: "sctk_rail_gate_maxsize", },
	}},
	gate_user : {type: 'struct', name: "gate_user", childs: {
		gatefunc: {mode: 'param', name: "gatefunc", type: "funcptr", doc: "Function to be called for this gate", dflt: "sctk_rail_gate_true", },
	}},
	net_gate : {type: 'union', name: "net_gate", choice: {
		boolean: {name: "boolean", type: "gate_boolean"},
		probabilistic: {name: "probabilistic", type: "gate_probabilistic"},
		minsize: {name: "minsize", type: "gate_min_size"},
		maxsize: {name: "maxsize", type: "gate_max_size"},
		user: {name: "user", type: "gate_probabilistic"},
	}},
	net_rail : {type: 'struct', name: "net_rail", childs: {
		name: {mode: 'param', name: "name", type: "string", doc: "Define the name of current rail.", dflt: null},
		priority: {mode: 'param', name: "priority", type: "int", doc: "Number which defines the order in which routes are tested (higher first).", dflt: "0", },
		device: {mode: 'param', name: "device", type: "string", doc: "Define the name of the device to use in this rail.", dflt: null},
		topology: {mode: 'param', name: "topology", type: "string", doc: "Define the network topology to apply on this rail.", dflt: null},
		config: {mode: 'param', name: "config", type: "string", doc: "Define the driver config to use for this rail.", dflt: null},
		gates: {mode: 'array', name: "gates", type: "net_gate", entryname: "gate", dflt: null},
	}},
	net_cli_option : {type: 'struct', name: "net_cli_option", childs: {
		name: {mode: 'param', name: "name", type: "string", doc: "Define the name of the option.", dflt: null},
		rails: {mode: 'array', name: "rails", type: "string", entryname: "rail", dflt: null},
	}},
	networks : {type: 'struct', name: "networks", childs: {
		configs: {mode: 'array', name: "configs", type: "net_driver_config", entryname: "config", dflt: null},
		rails: {mode: 'array', name: "rails", type: "net_rail", entryname: "rail", dflt: null},
		cli_options: {mode: 'array', name: "cli_options", type: "net_cli_option", entryname: "cli_option", dflt: null},
	}},
	inter_thread_comm : {type: 'struct', name: "inter_thread_comm", childs: {
		barrier_arity: {mode: 'param', name: "barrier_arity", type: "int", doc: "", dflt: "8", },
		broadcast_arity_max: {mode: 'param', name: "broadcast_arity_max", type: "int", doc: "", dflt: "32", },
		broadcast_max_size: {mode: 'param', name: "broadcast_max_size", type: "int", doc: "", dflt: "1024", },
		broadcast_check_threshold: {mode: 'param', name: "broadcast_check_threshold", type: "int", doc: "", dflt: "512", },
		allreduce_arity_max: {mode: 'param', name: "allreduce_arity_max", type: "int", doc: "", dflt: "8", },
		allreduce_max_size: {mode: 'param', name: "allreduce_max_size", type: "int", doc: "", dflt: "4096", },
		allreduce_check_threshold: {mode: 'param', name: "allreduce_check_threshold", type: "int", doc: "", dflt: "8192", },
		ALLREDUCE_MAX_SLOT: {mode: 'param', name: "ALLREDUCE_MAX_SLOT", type: "int", doc: "Slot size for allreduce", dflt: "65536", },
		collectives_init_hook: {mode: 'param', name: "collectives_init_hook", type: "funcptr", doc: "", dflt: "sctk_collectives_init_opt_noalloc_split_messages", },
	}},
	low_level_comm : {type: 'struct', name: "low_level_comm", childs: {
		checksum: {mode: 'param', name: "checksum", type: "bool", doc: "", dflt: "true", },
		send_msg: {mode: 'param', name: "send_msg", type: "funcptr", doc: "", dflt: "sctk_network_send_message_default", },
		network_mode: {mode: 'param', name: "network_mode", type: "string", doc: "", dflt: "default", },
		dyn_reordering: {mode: 'param', name: "dyn_reordering", type: "bool", doc: "", dflt: "false", },
		enable_idle_polling: {mode: 'param', name: "enable_idle_polling", type: "bool", doc: "Enable usage of polling during idle.", dflt: "false", },
		ib_global: {mode: 'param', name: "ib_global", type: "ib_global", doc: "Global parameters for IB", dflt: null},
	}},
	mpc : {type: 'struct', name: "mpc", childs: {
		log_debug: {mode: 'param', name: "log_debug", type: "bool", doc: "Print debug messages", dflt: "false", },
		hard_checking: {mode: 'param', name: "hard_checking", type: "bool", doc: "", dflt: "false", },
		buffering: {mode: 'param', name: "buffering", type: "bool", doc: "", dflt: "false", },
	}},
	openmp : {type: 'struct', name: "openmp", childs: {
		vp: {mode: 'param', name: "vp", type: "int", doc: "Number of VPs for each OpenMP team", dflt: "0", },
		schedule: {mode: 'param', name: "schedule", type: "string", doc: "Runtime schedule type and chunck size", dflt: "static", },
		nb_threads: {mode: 'param', name: "nb_threads", type: "int", doc: "Number of threads to use during execution", dflt: null},
		adjustment: {mode: 'param', name: "adjustment", type: "bool", doc: "Dynamic adjustment of the number of threads", dflt: "false", },
		proc_bind: {mode: 'param', name: "proc_bind", type: "bool", doc: "Bind threads to processor core", dflt: "true", },
		nested: {mode: 'param', name: "nested", type: "bool", doc: "Nested parallelism", dflt: "false", },
		stack_size: {mode: 'param', name: "stack_size", type: "int", doc: "Stack size for OpenMP threads", dflt: "0", },
		wait_policy: {mode: 'param', name: "wait_policy", type: "int", doc: "Behavior of threads while waiting", dflt: "0", },
		thread_limit: {mode: 'param', name: "thread_limit", type: "int", doc: "Maximum number of OpenMP threads among all teams", dflt: "0", },
		max_active_levels: {mode: 'param', name: "max_active_levels", type: "int", doc: "Maximum depth of nested parallelism", dflt: "0", },
		tree: {mode: 'param', name: "tree", type: "string", doc: "Tree shape for OpenMP construct", dflt: "", },
		max_threads: {mode: 'param', name: "max_threads", type: "int", doc: "Maximum number of threads for each team of a parallel region", dflt: "64", },
		max_alive_for_dyn: {mode: 'param', name: "max_alive_for_dyn", type: "int", doc: "Maximum number of shared for loops w/ dynamic schedule alive", dflt: "7", },
		max_alive_for_guided: {mode: 'param', name: "max_alive_for_guided", type: "int", doc: "Maximum number of shared for loops w/ guided schedule alive", dflt: "3", },
		max_alive_sections: {mode: 'param', name: "max_alive_sections", type: "int", doc: "Maximum number of alive sections construct", dflt: "3", },
		max_alive_single: {mode: 'param', name: "max_alive_single", type: "int", doc: "Maximum number of alive single construct", dflt: "3", },
		warn_nested: {mode: 'param', name: "warn_nested", type: "bool", doc: "Emit warning when entering nested parallelism", dflt: "false", },
		mode: {mode: 'param', name: "mode", type: "string", doc: "MPI/OpenMP hybrid mode (simple-mixed, alternating)", dflt: "simple-mixed", },
	}},
	profiler : {type: 'struct', name: "profiler", childs: {
		file_prefix: {mode: 'param', name: "file_prefix", type: "string", doc: "Prefix of MPC Profiler outputs", dflt: "mpc_profile", },
		append_date: {mode: 'param', name: "append_date", type: "bool", doc: "Add a timestamp to profiles file names", dflt: "true", },
		color_stdout: {mode: 'param', name: "color_stdout", type: "bool", doc: "Profile in color when outputed to stdout", dflt: "true", },
		level_colors: {mode: 'array', name: "level_colors", type: "string", entryname: "level", dflt: null},
	}},
	thread : {type: 'struct', name: "thread", childs: {
		spin_delay: {mode: 'param', name: "spin_delay", type: "int", doc: "Max number of accesses to the lock before calling thread_yield", dflt: "10", },
		interval: {mode: 'param', name: "interval", type: "int", doc: "", dflt: "10", },
		kthread_stack_size: {mode: 'param', name: "kthread_stack_size", type: "size", doc: "Define the stack size of MPC user threads", dflt: "10MB", },
	}},
};

meta.modules = {
	allocator: {name: "allocator", type: "allocator"},
	launcher: {name: "launcher", type: "launcher"},
	debugger: {name: "debugger", type: "debugger"},
	inter_thread_comm: {name: "inter_thread_comm", type: "inter_thread_comm"},
	low_level_comm: {name: "low_level_comm", type: "low_level_comm"},
	mpc: {name: "mpc", type: "mpc"},
	openmp: {name: "openmp", type: "openmp"},
	profiler: {name: "profiler", type: "profiler"},
	thread: {name: "thread", type: "thread"},
};

meta.networks = {
	networks: {type: "networks", name: "networks"},
};

meta.enum = {
	ibv_rdvz_protocol : {type: 'enum', name: "ibv_rdvz_protocol", doc: "", values: {
		IBV_RDVZ_WRITE_PROTOCOL: "IBV_RDVZ_WRITE_PROTOCOL",
		IBV_RDVZ_READ_PROTOCOL: "IBV_RDVZ_READ_PROTOCOL",
	}},
};
