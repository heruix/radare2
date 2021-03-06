/* radare2 - LGPL - Copyright 2016-2017 - n4x0r, soez, pancake */

#ifndef INCLUDE_HEAP_GLIBC_C
#define INCLUDE_HEAP_GLIBC_C
#define HEAP32 1
#include "linux_heap_glibc.c"
#undef HEAP32
#endif

#undef GH
#undef GHT
#undef GHT_MAX

#if HEAP32
#define GH(x) x##_32
#define GHT ut32
#define GHT_MAX UT32_MAX
#else
#define GH(x) x##_64
#define GHT ut64
#define GHT_MAX UT64_MAX
#endif

static void GH(update_arena_with_tc)(GH(RHeap_MallocState_tcache) *cmain_arena, MallocState *main_arena) {
	int i = 0;
	main_arena->mutex = cmain_arena->mutex;
	main_arena->flags = cmain_arena->flags;
	for (i = 0; i <= BINMAPSIZE; i++ ) {
		main_arena->binmap[i] = cmain_arena->binmap[i];
	}
	main_arena->have_fast_chunks = cmain_arena->have_fast_chunks;
	main_arena->attached_threads = cmain_arena->attached_threads;
	for (i = 0; i <= NFASTBINS; i++) {
		main_arena->GH(fastbinsY)[i] = cmain_arena->fastbinsY[i];
	}
	main_arena->GH(top) = cmain_arena->top;
	main_arena->GH(last_remainder) = cmain_arena->last_remainder;
	for (i = 0; i <= NBINS * 2 - 2; i++) {
		main_arena->GH(bins)[i] = cmain_arena->bins[i];
	}
	main_arena->GH(next) = cmain_arena->next;
	main_arena->GH(next_free) = cmain_arena->next_free;
	main_arena->GH(system_mem) = cmain_arena->system_mem;
	main_arena->GH(max_system_mem) = cmain_arena->max_system_mem;
}

static void GH(update_arena_without_tc)(GH(RHeap_MallocState) *cmain_arena, MallocState *main_arena) {
	int i = 0;
	main_arena->mutex = cmain_arena->mutex;
	main_arena->flags = cmain_arena->flags;
	for (i = 0; i <= BINMAPSIZE; i++ ) {
		main_arena->binmap[i] = cmain_arena->binmap[i];
	}
	for (i = 0; i <= NFASTBINS; i++) {
		main_arena->GH(fastbinsY)[i] = cmain_arena->fastbinsY[i];
	}
	main_arena->GH(top) = cmain_arena->top;
	main_arena->GH(last_remainder) = cmain_arena->last_remainder;
	for (i = 0; i <= NBINS * 2 - 2; i++) {
		main_arena->GH(bins)[i] = cmain_arena->bins[i];
	}
	main_arena->GH(next) = cmain_arena->next;
	main_arena->GH(next_free) = cmain_arena->next_free;
	main_arena->GH(system_mem) = cmain_arena->system_mem;
	main_arena->GH(max_system_mem) = cmain_arena->max_system_mem;
}

static void GH(update_main_arena)(RCore *core, GHT m_arena, MallocState *main_arena) {
	const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");
	if (tcache) {
		GH(RHeap_MallocState_tcache) *cmain_arena = R_NEW0 (GH(RHeap_MallocState_tcache));
		(void)r_io_read_at (core->io, m_arena, (ut8 *)cmain_arena, sizeof (GH(RHeap_MallocState_tcache)));
		GH(update_arena_with_tc)(cmain_arena,main_arena);
	}
	else {
		GH(RHeap_MallocState) *cmain_arena = R_NEW0 (GH(RHeap_MallocState));
		(void)r_io_read_at (core->io, m_arena, (ut8 *)cmain_arena, sizeof (GH(RHeap_MallocState)));
		GH(update_arena_without_tc)(cmain_arena,main_arena);
	}
}

static void GH(update_global_max_fast)(RCore *core, GHT g_max_fast, GHT *global_max_fast) {
	(void)r_io_read_at (core->io, g_max_fast, (ut8 *)global_max_fast, sizeof (GHT));
}

static void GH(get_brks)(RCore *core, GHT *brk_start, GHT *brk_end) {
	RListIter *iter;
	RDebugMap *map;
	r_debug_map_sync (core->dbg);
	r_list_foreach (core->dbg->maps, iter, map) {
		if (strstr (map->name, "[heap]")) {
		        *brk_start = map->addr;
			*brk_end = map->addr_end;
		        break;
		}
	}
}

static void GH(print_main_arena)(RCore *core, GHT m_arena, MallocState *main_arena, GHT global_max_fast, int format) {
	int i, j, k, start;
	GHT align = 12 * SZ + sizeof (int) * 2;
	const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");

	if (tcache) {
		align = 16;
	}

	GHT apart[NSMALLBINS + 1] = { 0LL };
	if (format == '*') {
		for (i = 0; i < NBINS * 2 - 2; i += 2) {
			GHT addr = m_arena + align + SZ * i - SZ * 2;
			GHT bina = main_arena->GH(bins)[i];
			r_cons_printf ("f chunk.%d.bin = 0x%"PFMT64x"\n", i, (ut64)addr);
			r_cons_printf ("f chunk.%d.fd = 0x%"PFMT64x"\n", i, (ut64)bina);
			bina = main_arena->GH(bins)[i + 1];
			r_cons_printf ("f chunk.%d.bk = 0x%"PFMT64x"\n", i, (ut64)bina);
		}
		for (i = 0; i < BINMAPSIZE; i++) {
			r_cons_printf ("f binmap.%d = 0x%"PFMT64x, i, (ut64) main_arena->binmap[i]);
		}
		{	/* maybe use SDB instead of flags for this? */
			char *units = r_num_units (NULL, main_arena->GH(max_system_mem));
			r_cons_printf ("f heap.maxmem = %s\n", units);
			free (units);
			units = r_num_units (NULL, main_arena->GH(system_mem));
			r_cons_printf ("f heap.sysmem = %s\n", units);
			free (units);
			units = r_num_units (NULL, main_arena->GH(next_free));
			r_cons_printf ("f heap.nextfree = %s\n", units);
			free (units);
			units = r_num_units (NULL, main_arena->GH(next));
			r_cons_printf ("f heap.next= %s\n", units);
			free (units);
		}
		return;
	}

	PRINT_GA ("malloc_state @ ");
	PRINTF_BA ("0x%"PFMT64x"\n\n", (ut64)m_arena);
	PRINT_GA ("struct malloc_state main_arena {\n");
	PRINT_GA ("  mutex = ");
	PRINTF_BA ("0x%08x\n", (ut32)main_arena->mutex);
	PRINT_GA ("  flags = ");
	PRINTF_BA ("0x%08x\n", (ut32)main_arena->flags);
	PRINT_GA ("  fastbinsY = {\n");

	for (i = 0, j = 1, k = SZ * 4; i < NFASTBINS; i++, j++, k += SZ * 2) {
		if (FASTBIN_IDX_TO_SIZE(j) <= global_max_fast) {
			PRINTF_YA (" Fastbin %02d\n", j);
		} else {
			PRINTF_RA (" Fastbin %02d\n", j);
		}
		PRINT_GA (" chunksize:");
		PRINTF_BA (" == %04d ", k);
		PRINTF_GA ("0x%"PFMT64x, (ut64)main_arena->GH(fastbinsY)[i]);
		PRINT_GA (",\n");
	}
	PRINT_GA ("}\n");
	PRINT_GA ("  top = ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)main_arena->GH(top));
	PRINT_GA (",\n");
	PRINT_GA ("  last_remainder = ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)main_arena->GH(last_remainder));
	PRINT_GA (",\n");
	PRINT_GA ("  bins {\n");

	/* Index & size for largebins */
	start = SZ * 128;
	for (i = start, k = 0, j = 0; j < NBINS - 2 && i < 1024 * 1024; i += 64) {
		j = largebin_index (i);
		if (j == k + NSMALLBINS + 1) {
			apart[k++] = i;
		}
	}
	for (i = 0, j = 1, k = SZ * 4; i < NBINS * 2 - 2; i += 2, j++) {
		PRINTF_YA (" Bin %03d: ", j);
		if (j == 1) {
			PRINT_GA ("Unsorted Bin");
			PRINT_GA (" [");
			PRINT_GA (" chunksize:");
			PRINT_BA (" undefined ");
		} else if (j > 1 && j <= NSMALLBINS) {
			if (j == 2) {
				PRINT_GA ("             ┌");
			} else if (j == (NSMALLBINS / 2)) {
				PRINT_GA ("  Small Bins │");
			} else if (j != 2 && j != (NSMALLBINS / 2) && j != NSMALLBINS) {
				PRINT_GA ("             │");
			} else {
				PRINT_GA ("             └");
			}
			PRINT_GA (" chunksize:");
			PRINTF_BA (" == %06d  ", k);
			if (j < NSMALLBINS) {
				k += SZ * 2;
			}
		} else {
			if (j == NSMALLBINS + 1) {
				PRINT_GA ("             ┌");
			} else if (j == (NSMALLBINS / 2) * 3) {
				PRINT_GA ("  Large Bins │");
			} else if (j != NSMALLBINS + 1 && j != (NSMALLBINS / 2) * 3 && j != NBINS - 1) {
				PRINT_GA ("             │");
			} else {
				PRINT_GA ("             └");
			}
			PRINT_GA (" chunksize:");
			if (j != NBINS - 1) {
				PRINTF_BA (" >= %06d  ", apart[j - NSMALLBINS - 1]);
			} else {
				PRINT_BA (" remaining ");
			}
		}
		GHT bin = m_arena + align + SZ * i - SZ * 2;
		PRINTF_GA ("0x%"PFMT64x"->fd = ", (ut64)bin);
		PRINTF_BA ("0x%"PFMT64x, (ut64)main_arena->GH(bins)[i]);
		PRINT_GA (", ");
		PRINTF_GA ("0x%"PFMT64x"->bk = ", (ut64)bin);
		PRINTF_BA ("0x%"PFMT64x, (ut64) main_arena->GH(bins)[i + 1] );
		PRINT_GA (", ");
		r_cons_newline ();
	}

	PRINT_GA ("  }\n");
	PRINT_GA ("  binmap = {");

	for (i = 0; i < BINMAPSIZE; i++) {
		if (i) {
			PRINT_GA (",");
		}
		PRINTF_BA ("0x%x", (ut32)main_arena->binmap[i]);
	}
	PRINT_GA ("}\n");
	PRINT_GA ("  next = ");
	PRINTF_BA ("0x%"PFMT64x, (ut64) main_arena->GH(next));
	PRINT_GA (",\n");
	PRINT_GA ("  next_free = ");
	PRINTF_BA ("0x%"PFMT64x, (ut64) main_arena->GH(next_free));
	PRINT_GA (",\n");
	PRINT_GA ("  system_mem = ");
	PRINTF_BA ("0x%"PFMT64x, (ut64) main_arena->GH(system_mem));
	PRINT_GA (",\n");
	PRINT_GA ("  max_system_mem = ");
	PRINTF_BA ("0x%"PFMT64x, (ut64) main_arena->GH(max_system_mem));
	PRINT_GA (",\n");
	PRINT_GA ("}\n\n");
}

static GHT GH(get_vaddr_symbol)(RCore *core, const char *path, const char *symname) {
	RListIter *iter;
	RBinSymbol *s;
	RBinOptions *bo = r_bin_options_new (0LL, 0LL, false);
	if (!bo) {
		eprintf ("Failed to create bin options\n");
		return (GHT) -1;
	}

	// TODO: avoid loading twice?
	if (r_bin_open (core->bin, path, bo) == -1) {
		eprintf ("Failed to open binary\n");
		r_bin_options_free (bo);
		return (GHT) -1;
	}

	RList *syms = r_bin_get_symbols (core->bin);
	if (!syms) {
		r_bin_options_free (bo);
		return (GHT) -1;
	}
	GHT vaddr = 0LL;
	r_list_foreach (syms, iter, s) {
		if (strstr (s->name, symname)) {
			vaddr = s->vaddr;
			break;
		}
	}
	r_bin_options_free (bo);
	return vaddr;
}

static bool GH(r_resolve_symbol)(RCore *core, GHT *symbol, const char *symname) {
	const char *dir_dbg = "/usr/lib/debug";
	const char *dir_build_id = "/.build-id";
	const char *libc_ver_end = NULL;
	char hash[64] = R_EMPTY, *path = NULL;
	bool is_debug_file[6];
	GHT libc_addr = GHT_MAX, vaddr = GHT_MAX;
	RListIter *iter;
	RDebugMap *map;

	if (!core || !core->dbg || !core->dbg->maps) {
		return false;
	}

	r_debug_map_sync (core->dbg);
	r_list_foreach (core->dbg->maps, iter, map) {
		if (strstr (map->name, "/libc-")) {
			libc_addr = map->addr;
			libc_ver_end = map->name;
			break;
		}
	}
	if (!libc_ver_end) {
		eprintf ("Warning: Can't find glibc mapped in memory (see dm)\n");
		return false;
	}
	is_debug_file[0] = str_start_with (libc_ver_end, "/usr/lib/");
	is_debug_file[1] = str_start_with (libc_ver_end, "/usr/lib32/");
	is_debug_file[2] = str_start_with (libc_ver_end, "/usr/lib64/");
	is_debug_file[3] = str_start_with (libc_ver_end, "/lib/");
	is_debug_file[4] = str_start_with (libc_ver_end, "/lib32/");
	is_debug_file[5] = str_start_with (libc_ver_end, "/lib64/");

	if (!is_debug_file[0] && !is_debug_file[1] && \
	!is_debug_file[2] && !is_debug_file[3] && \
	!is_debug_file[4] && !is_debug_file[5]) {
		path = r_cons_input ("Is a custom library? (LD_PRELOAD=..) Enter full path glibc: ");
		if (r_file_exists (path)) {
			goto found;
		}
	}

	if (is_debug_file[0] || is_debug_file[1] || is_debug_file[2]) {
		free (path);
		path = r_str_newf ("%s", libc_ver_end);
		if (r_file_exists (path)) {
			goto found;
		}
	}

	if ((is_debug_file[3] || is_debug_file[4] || is_debug_file[5]) && \
	r_file_is_directory ("/usr/lib/debug")) {
		free (path);
		path = r_str_newf ("%s%s", dir_dbg, libc_ver_end);
		if (r_file_exists (path)) {
			goto found;
		}
		path = r_str_append (path, ".debug");
		if (r_file_exists (path)) {
			goto found;
		}
	}

	if ((is_debug_file[3] || is_debug_file[4] || is_debug_file[5]) && \
	r_file_is_directory ("/usr/lib/debug/.build-id")) {
		get_hash_debug_file (core, libc_ver_end, hash, sizeof (hash) - 1);
		libc_ver_end = hash;
		free (path);
		path = r_str_newf ("%s%s%s", dir_dbg, dir_build_id, libc_ver_end);
		if (r_file_exists (path)) {
			goto found;
		}
	}

	goto not_found;
found:
	vaddr = GH(get_vaddr_symbol) (core, path, symname);
	if (libc_addr != GHT_MAX && vaddr && vaddr != GHT_MAX) {
		const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");
		const int offset = r_config_get_i (core->config, "dbg.glibc.ma_offset");
		if (tcache) {
			*symbol = libc_addr + vaddr + offset;
		}
		else {
			*symbol = libc_addr + vaddr;
		}
		free (path);
		return true;
	}
not_found:
	eprintf (
	  "Warning: glibc library with symbol %s could not be "
	"found. Is libc6-dbg installed?\n", symname);
	free (path);
	return false;
}

static bool GH(r_resolve_global_max_fast)(RCore *core, GHT *g_max_fast, GHT *global_max_fast) {
	if (!core || !core->dbg || !core->dbg->maps) {
		return false;
	}
	if (*g_max_fast == GHT_MAX) {
		if (GH(r_resolve_symbol) (core, g_max_fast, "global_max_fast")) {
			if (global_max_fast) {
				GH(update_global_max_fast) (core, *g_max_fast, global_max_fast);
				return true;
			}
		}
		return false;
	} else {
		GH(update_global_max_fast) (core, *g_max_fast, global_max_fast);
	}
	return true;
}

static bool GH(r_resolve_main_arena)(RCore *core, GHT *m_arena, MallocState *main_arena) {
	if (!core || !core->dbg || !core->dbg->maps) {
		return false;
	}
	if (*m_arena == GHT_MAX) {
		if (GH(r_resolve_symbol) (core, m_arena, "main_arena")) {
			if (main_arena) {
				GH(update_main_arena) (core, *m_arena, main_arena);
				return true;
			}
		}
		return false;
	} else {
		GH(update_main_arena) (core, *m_arena, main_arena);
	}
	return true;
}

void GH(print_heap_chunk)(RCore *core) {
	GH(RHeapChunk) *cnk = R_NEW0 (GH(RHeapChunk));
	GHT chunk = core->offset;

	if (!cnk) {
		return;
	}

	(void) r_io_read_at (core->io, chunk, (ut8 *)cnk, sizeof (*cnk));

	PRINT_GA ("struct malloc_chunk @ ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)chunk);
	PRINT_GA (" {\n  prev_size = ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)cnk->prev_size);
	PRINT_GA (",\n  size = ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)cnk->size & ~(NON_MAIN_ARENA | IS_MMAPPED | PREV_INUSE));
	PRINT_GA(",\n  flags: |N:");
	PRINTF_BA("%1d", cnk->size & NON_MAIN_ARENA);
	PRINT_GA(" |M:");
	PRINTF_BA("%1d", cnk->size & IS_MMAPPED);
	PRINT_GA(" |P:");
	PRINTF_BA("%1d", cnk->size & PREV_INUSE);

	PRINT_GA (",\n  fd = ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)cnk->fd);

	PRINT_GA (",\n  bk = ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)cnk->bk);

	if (cnk->size  > SZ * 128) {
		PRINT_GA (",\n  fd-nextsize = ");
		PRINTF_BA ("0x%"PFMT64x, (ut64) cnk->fd_nextsize);
		PRINT_GA (",\n  bk-nextsize = ");
		PRINTF_BA ("0x%"PFMT64x, (ut64) cnk->bk_nextsize);
	}

	PRINT_GA (",\n}\n");
	GHT size = ((cnk->size >> 3) << 3) - SZ * 2;
	if (size > SZ * 128) {
		PRINT_GA ("chunk too big to be displayed\n");
		size = SZ * 128;
	}

	char *data = calloc (1, size);
	if (data) {
		r_io_read_at (core->io, chunk + SZ * 2, (ut8 *)data, size);
		PRINT_GA ("chunk data = \n");
		r_print_hexdump (core->print, chunk + SZ * 2, (ut8 *)data, size, SZ * 8, SZ, 1);
		free (data);
	}
	free (cnk);
}

static int GH(print_double_linked_list_bin_simple)(RCore *core, GHT bin, MallocState *main_arena, GHT brk_start) {
	GHT next = GHT_MAX;
	GH(RHeapChunk) *cnk = R_NEW0 (GH(RHeapChunk));

	if (!cnk) {
		return -1;
	}

	r_io_read_at (core->io, bin, (ut8 *)cnk, sizeof (GH(RHeapChunk)));

	PRINTF_GA ("    0x%"PFMT64x, (ut64)bin);
	while (cnk->fd != bin) {
		PRINTF_BA ("->fd = 0x%"PFMT64x, (ut64)cnk->fd);
		next = cnk->fd;
		if (next < brk_start || next > main_arena->GH(top)) {
			PRINT_RA ("Double linked list corrupted\n");
			free (cnk);
			return -1;
		}
		r_io_read_at (core->io, next, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
	}

	PRINTF_GA ("->fd = 0x%"PFMT64x, (ut64)cnk->fd);
	next = cnk->fd;

	if (next != bin) {
		PRINT_RA ("Double linked list corrupted\n");
		free (cnk);
		return -1;
	}
	(void)r_io_read_at (core->io, next, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
	PRINTF_GA ("\n    0x%"PFMT64x, (ut64)bin);

	while (cnk->bk != bin) {
		PRINTF_BA ("->bk = 0x%"PFMT64x, (ut64) cnk->bk);
		next = cnk->bk;
		if (next < brk_start || next > main_arena->GH(top)) {
			PRINT_RA ("Double linked list corrupted.\n");
			free (cnk);
			return -1;
		}
		(void)r_io_read_at (core->io, next, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
	}

	PRINTF_GA ("->bk = 0x%"PFMT64x, (ut64)cnk->bk);
	free (cnk);
	return 0;
}

static int GH(print_double_linked_list_bin_graph)(RCore *core, GHT bin, MallocState *main_arena, GHT brk_start) {
	RAGraph *g = r_agraph_new (r_cons_canvas_new (1, 1));
	GHT next = GHT_MAX;
	char title[256], chunk[256];
	RANode *bin_node = NULL, *prev_node = NULL, *next_node = NULL;
	GH(RHeapChunk) *cnk = R_NEW0 (GH(RHeapChunk));

	if (!cnk || !g) {
		free (cnk);
		r_agraph_free (g);
		return -1;
	}
	g->can->color = r_config_get_i (core->config, "scr.color");

	(void)r_io_read_at (core->io, bin, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
	snprintf (title, sizeof (title) - 1, "bin @ 0x%"PFMT64x"\n", (ut64)bin);
	snprintf (chunk, sizeof (chunk) - 1, "fd: 0x%"PFMT64x"\nbk: 0x%"PFMT64x"\n",
		(ut64)cnk->fd, (ut64)cnk->bk);
	bin_node = r_agraph_add_node (g, title, chunk);
	prev_node = bin_node;

	while (cnk->bk != bin) {
		next = cnk->bk;
		if (next < brk_start || next > main_arena->GH(top)) {
			PRINT_RA ("Double linked list corrupted\n");
			free (cnk);
			free (g);
			return -1;
		}

		r_io_read_at (core->io, next, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
		snprintf (title, sizeof (title) - 1, "Chunk @ 0x%"PFMT64x"\n", (ut64)next);
		snprintf (chunk, sizeof (chunk) - 1, "fd: 0x%"PFMT64x"\nbk: 0x%"PFMT64x"\n",
			(ut64)cnk->fd, (ut64)cnk->bk);
		next_node = r_agraph_add_node (g, title, chunk);
		r_agraph_add_edge (g, prev_node, next_node);
		r_agraph_add_edge (g, next_node, prev_node);
		prev_node = next_node;
	}

	r_agraph_add_edge (g, prev_node, bin_node);
	r_agraph_add_edge (g, bin_node, prev_node);
	r_agraph_print (g);

	free (cnk);
	r_agraph_free (g);
	return 0;
}

static int GH(print_double_linked_list_bin)(RCore *core, MallocState *main_arena, GHT m_arena, GHT offset, GHT num_bin, int graph) {
	if (!core || !core->dbg || !core->dbg->maps) {
                return -1;
	}
	int ret = 0;
	GHT brk_start = GHT_MAX, brk_end = GHT_MAX;
	if (num_bin > 126) {
		return -1;
	}
	GHT bin = main_arena->GH(bins)[num_bin];

	if (!bin) {
		return -1;
	}

	GH(get_brks) (core, &brk_start, &brk_end);
	if (brk_start == GHT_MAX || brk_end == GHT_MAX) {
		eprintf ("No Heap section\n");
		return -1;
	}

#if __GLIBC_MINOR__ > 25
	bin = m_arena + offset + SZ * num_bin * 2 + 10 * SZ;
#else
	bin = m_arena + offset + SZ * num_bin * 2 - SZ * 2;
#endif

	switch (num_bin) {
	case 0:
		PRINT_GA ("  double linked list unsorted bin {\n");
		break;
	case 1 ... NSMALLBINS - 1:
		PRINT_GA ("  double linked list small bin {\n");
		break;
	case NSMALLBINS ... NBINS - 2:
		PRINT_GA ("  double linked list large bin {\n");
		break;
	}

	if (!graph || graph == 1) {
		ret = GH(print_double_linked_list_bin_simple)(core, bin, main_arena, brk_start);
	} else {
		ret = GH(print_double_linked_list_bin_graph)(core, bin,  main_arena, brk_start);
	}
	PRINT_GA ("\n  }\n");
	return ret;
}

static void GH(print_heap_bin)(RCore *core, GHT m_arena, MallocState *main_arena, const char *input) {
	int i, j = 2;
	GHT num_bin = GHT_MAX;

#if __GLIBC_MINOR__ > 25
	GHT offset = 16;
#else
	GHT offset = 12 * SZ + sizeof (int) * 2;
#endif

	switch (input[0]) {
	case '\0': // dmhb
		PRINT_YA ("Bins {\n");
		for (i = 0; i < NBINS - 1; i++) {
			PRINTF_YA (" Bin %03d:\n", i + 1);
			if (!GH(print_double_linked_list_bin)(core, main_arena, m_arena, offset, i, 0)) {
				PRINT_GA ("  Empty bin");
				PRINT_BA ("  0x0\n");
			}
		}
		PRINT_YA ("\n}\n");
		break;
	case ' ': // dmhb [bin_num]
		j--; // for spaces after input
		/* fallthu */
	case 'g': // dmhbg [bin_num]
		num_bin = r_num_math (core->num, input + j) - 1;
		if (num_bin > NBINS - 2) {
			eprintf ("Error: 0 < bin <= %d\n", NBINS - 1);
			break;
		}
		PRINTF_YA ("  Bin %03d:\n", num_bin + 1);
		if (!GH(print_double_linked_list_bin)(core, main_arena, m_arena, offset, num_bin, j)) {
			PRINT_GA ("Empty bin");
			PRINT_BA (" 0x0\n");
		}
		break;
	}
}

static int GH(print_single_linked_list_bin)(RCore *core, MallocState *main_arena, GHT m_arena, GHT offset, GHT bin_num) {
	if (!core || !core->dbg || !core->dbg->maps) {
                return -1;
	}
	GHT next = GHT_MAX, brk_start = GHT_MAX, brk_end = GHT_MAX;
	GHT bin = main_arena->GH(fastbinsY)[bin_num];
	if (!bin) {
		return -1;
	}
	GH(RHeapChunk) *cnk = R_NEW0 (GH(RHeapChunk));

	if (!cnk) {
		return 0;
	}
	bin = m_arena + offset + SZ * bin_num;
	r_io_read_at (core->io, bin, (ut8 *)&next, SZ);

        GH(get_brks)(core, &brk_start, &brk_end);
	if (brk_start == GHT_MAX || brk_end == GHT_MAX) {
		eprintf ("No Heap section\n");
		free (cnk);
		return 0;
	}

	PRINTF_GA ("  fastbin %d @ ", bin_num + 1);
	PRINTF_GA ("0x%"PFMT64x" {\n   ", (ut64)bin);

	GHT size = main_arena->GH(top) - brk_start;

	GHT next_root = next, next_tmp = next, double_free = GHT_MAX;
	while (next && next >= brk_start && next < main_arena->GH(top)) {
		PRINTF_BA ("0x%"PFMT64x, (ut64)next);
		while (double_free == GHT_MAX && next_tmp && next_tmp >= brk_start && next_tmp <= main_arena->GH(top)) {
			r_io_read_at (core->io, next_tmp, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
			next_tmp = cnk->fd;
			if (cnk->prev_size > size || ((cnk->size >> 3) << 3) > size) {
				break;
			}
			if (next_root == next_tmp) {
				double_free = next_root;
				break;
			}
		}
		r_io_read_at (core->io, next, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
		next = cnk->fd;
		PRINTF_BA ("%s", next ? "->fd = " : "");
		if (cnk->prev_size > size || ((cnk->size >> 3) << 3) > size) {
			PRINTF_RA (" 0x%"PFMT64x, (ut64)next);
			PRINT_RA (" Linked list corrupted\n");
			PRINT_GA ("\n  }\n");
			free (cnk);
			return -1;
		}

		next_root = next_tmp = next;
		if (double_free == next) {
			PRINTF_RA ("0x%"PFMT64x, (ut64)next);
			PRINT_RA (" Double free detected\n");
			PRINT_GA ("\n  }\n");
			free (cnk);
			return -1;
		}
	}

	if (next && (next < brk_start || next >= main_arena->GH(top))) {
		PRINTF_RA ("0x%"PFMT64x, (ut64)next);
		PRINT_RA (" Linked list corrupted\n");
		PRINT_GA ("\n  }\n");
		free (cnk);
		return -1;
	}

	PRINT_GA ("\n  }\n");
	free (cnk);
	return 0;
}

void GH(print_heap_fastbin)(RCore *core, GHT m_arena, MallocState *main_arena, GHT global_max_fast, const char *input) {
	int i;
	GHT num_bin = GHT_MAX, offset = sizeof (int) * 2;
	const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");
	if (tcache) {
		offset = 16;
	}

	switch (input[0]) {
	case '\0': // dmhf
		PRINT_YA ("fastbinY {\n");
		for (i = 1; i <= NFASTBINS; i++) {
			if (FASTBIN_IDX_TO_SIZE(i) <= global_max_fast) {
				PRINTF_YA (" Fastbin %02d\n", i);
			} else {
				PRINTF_RA (" Fastbin %02d\n", i);
			}
			if (!GH(print_single_linked_list_bin) (core, main_arena, m_arena, offset, i - 1)) {
				PRINT_GA ("  Empty bin");
				PRINT_BA ("  0x0\n");
			}
		}
		PRINT_YA ("}\n");
		break;
	case ' ': // dmhf [bin_num]
		num_bin = r_num_math (core->num, input + 1) - 1;
		if (num_bin >= NFASTBINS) {
			eprintf ("Error: 0 < bin <= %d\n", NFASTBINS);
			break;
		}
		if (!GH(print_single_linked_list_bin)(core, main_arena, m_arena, offset, num_bin)) {
			PRINT_GA (" Empty bin");
			PRINT_BA (" 0x0\n");
		}
		break;
	}
}

static void GH(print_mmap_graph)(RCore *core, MallocState *malloc_state, GHT m_state) {
	int w, h;
	GHT top_size = GHT_MAX, min_size = SZ * 4;

	if (!core || !core->dbg || !core->dbg->maps) {
		return;
	}

	RConfigHold *hc = r_config_hold_new (core->config);
	if (!hc) {
		return;
	}

	w = r_cons_get_size (&h);
	RConsCanvas *can = r_cons_canvas_new (w, h);
	if (!can) {
		r_config_hold_free (hc);
		return;
	}
	can->linemode = r_config_get_i (core->config, "graph.linemode");
	can->color = r_config_get_i (core->config, "scr.color");
	core->cons->use_utf8 = r_config_get_i (core->config, "scr.utf8");
	RAGraph *g = r_agraph_new (can);
	if (!g) {
		r_cons_canvas_free (can);
		r_config_restore (hc);
		r_config_hold_free (hc);
		return;
	}
	g->layout = r_config_get_i (core->config, "graph.layout");
	RANode *top = R_EMPTY, *chunk_node = R_EMPTY, *prev_node = R_EMPTY;
	GH(RHeapChunk) *cnk = R_NEW0 (GH(RHeapChunk)),*prev_c = R_NEW0 (GH(RHeapChunk));
	if (!cnk || !prev_c) {
		free (cnk);
		free (prev_c);
		r_cons_canvas_free (can);
		r_agraph_free (g);
		r_config_restore (hc);
		r_config_hold_free (hc);
		return;
	}

	GHT next_chunk_ref, prev_chunk_ref, size_tmp;
	char *top_title, *top_data, *node_title, *node_data;
	bool first_node = true;

	r_agraph_set_title (g, "Mmmaped Heap");
	top_title = r_str_newf ("Top chunk @ 0x%"PFMT64x"\n", (ut64)malloc_state->GH(top));

	GHT start_mmap = m_state + sizeof (MallocState); //0x8b0;
	r_io_read_at (core->io, malloc_state->GH(top), (ut8*)cnk, sizeof (GH(RHeapChunk)));
	GHT end_mmap = malloc_state->GH(top);

	top_data = r_str_newf ("[mmap_start:0x%"PFMT64x", mmap_end:0x%"PFMT64x"]\n",
		(ut64)start_mmap, (ut64)end_mmap + ((cnk->size >> 3) << 3));
	next_chunk_ref = start_mmap, prev_chunk_ref = next_chunk_ref;
	top_size = (cnk->size >> 3) << 3;

	while (next_chunk_ref != malloc_state->GH(top) && next_chunk_ref != end_mmap) {
		r_io_read_at (core->io, next_chunk_ref, (ut8 *)prev_c, sizeof (GH(RHeapChunk)));
	       	node_title = r_str_newf ("  Malloc chunk @ 0x%"PFMT64x" ", (ut64)prev_chunk_ref);
		size_tmp = (prev_c->size >> 3) << 3;
		if (size_tmp < min_size || size_tmp > top_size  || next_chunk_ref + size_tmp > malloc_state->GH(top)) {
			node_data = r_str_newf ("[corrupted] size: 0x%"PFMT64x"\n fd: 0x%"PFMT64x", bk: 0x%"PFMT64x
				"\nHeap graph could not be recovered\n", (ut64)prev_c->size, (ut64)prev_c->fd, (ut64)prev_c->bk) ;
			r_agraph_add_node (g, node_title, node_data);
			if (first_node) {
				first_node = false;
			}
			break;
		}
		next_chunk_ref += size_tmp;
		prev_chunk_ref = next_chunk_ref;
		r_io_read_at (core->io, next_chunk_ref, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
		node_data = r_str_newf ("size: 0x%"PFMT64x"\n fd: 0x%"PFMT64x", bk: 0x%"PFMT64x"\n",
			(ut64)prev_c->size, (ut64)prev_c->fd, (ut64)prev_c->bk) ;
		chunk_node = r_agraph_add_node (g, node_title, node_data);
		if (first_node) {
			first_node = false;
		} else {
			r_agraph_add_edge (g, prev_node, chunk_node);
		}
		prev_node = chunk_node;
	}
	top = r_agraph_add_node (g, top_title, top_data);
	if (!first_node) {
		r_agraph_add_edge (g, prev_node, top);
		free (node_data);
		free (node_title);
	}
	r_agraph_print (g);
	r_cons_canvas_free (can);
	r_config_restore (hc);
	r_config_hold_free (hc);
	free (g);
	free (cnk);
	free (prev_c);
	free (top_data);
	free (top_title);
}

static void GH(print_heap_graph)(RCore *core, MallocState *main_arena, GHT *initial_brk) {
	int w, h;
	GHT top_size = GHT_MAX, min_size = SZ * 4;

	if (!core || !core->dbg || !core->config || !core->dbg->maps) {
		return;
	}

	RConfigHold *hc = r_config_hold_new (core->config);
	if (!hc) {
		return;
	}

	w = r_cons_get_size (&h);
	RConsCanvas *can = r_cons_canvas_new (w, h);
	if (!can) {
		r_config_hold_free (hc);
		return;
	}
	can->linemode = r_config_get_i (core->config, "graph.linemode");
	can->color = r_config_get_i (core->config, "scr.color");
	core->cons->use_utf8 = r_config_get_i (core->config, "scr.utf8");
	RAGraph *g = r_agraph_new (can);
	if (!g) {
		r_cons_canvas_free (can);
		r_config_restore (hc);
		r_config_hold_free (hc);
		return;
	}
	g->layout = r_config_get_i (core->config, "graph.layout");
	RANode *top = R_EMPTY, *chunk_node = R_EMPTY, *prev_node = R_EMPTY;
	GH(RHeapChunk) *cnk = R_NEW0 (GH(RHeapChunk)), *prev_c = R_NEW0 (GH(RHeapChunk));

	if (!cnk || !prev_c) {
		r_cons_canvas_free (can);
		r_config_restore (hc);
		r_config_hold_free (hc);
		free (cnk);
		free (prev_c);
		free (g);
		return;
	}

	GHT next_chunk_ref, prev_chunk_ref, brk_start = GHT_MAX, brk_end = GHT_MAX, size_tmp;
	char *top_title, *top_data, *node_title, *node_data;
	bool first_node = true;

	r_agraph_set_title (g, "Heap Layout");
	top_title = r_str_newf ("Top chunk @ 0x%"PFMT64x"\n", (ut64)main_arena->GH(top));

	GH (get_brks)(core, &brk_start, &brk_end);
	const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");
	const int offset = r_config_get_i (core->config, "dbg.glibc.fc_offset");
	if (tcache) {
		*initial_brk = ( (brk_start >> 12) << 12 ) + offset;
	}
	else {
		*initial_brk = (brk_start >> 12) << 12;
	}

	if (brk_start == GHT_MAX || brk_end == GHT_MAX || *initial_brk == GHT_MAX) {
		eprintf ("No Heap section\n");
		r_cons_canvas_free (can);
		r_config_restore (hc);
		r_config_hold_free (hc);
		free (cnk);
		free (prev_c);
		free (g);
		free (top_title);
		return;
	}

	top_data = r_str_newf ("[brk_start:0x%"PFMT64x", brk_end:0x%"PFMT64x"]\n", (ut64)brk_start, (ut64)brk_end);
	next_chunk_ref = *initial_brk, prev_chunk_ref = next_chunk_ref;
	top_size = main_arena->GH(top) - brk_start;

	while (next_chunk_ref != main_arena->GH(top) && next_chunk_ref != brk_end) {
		r_io_read_at (core->io, next_chunk_ref, (ut8 *)prev_c, sizeof (GH(RHeapChunk)));
	       	node_title = r_str_newf ("  Malloc chunk @ 0x%"PFMT64x" ", (ut64)prev_chunk_ref);
		size_tmp = (prev_c->size >> 3) << 3;
		if (top_size < min_size || size_tmp > top_size  || next_chunk_ref + size_tmp > main_arena->GH(top)) {
			node_data = r_str_newf ("[corrupted] size: 0x%"PFMT64x"\n fd: 0x%"PFMT64x", bk: 0x%"PFMT64x
				"\nHeap graph could not be recovered\n", (ut64)prev_c->size, (ut64)prev_c->fd, (ut64)prev_c->bk) ;
			r_agraph_add_node (g, node_title, node_data);
			if (first_node) {
				first_node = false;
			}
			//r_agraph_add_edge (g, prev_node, chunk_node);
			break;
		}
		next_chunk_ref += size_tmp;
		prev_chunk_ref = next_chunk_ref;
		r_io_read_at (core->io, next_chunk_ref, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
		node_data = r_str_newf ("size: 0x%"PFMT64x"\n fd: 0x%"PFMT64x", bk: 0x%"PFMT64x"\n",
			(ut64)prev_c->size, (ut64)prev_c->fd, (ut64)prev_c->bk) ;
		chunk_node = r_agraph_add_node (g, node_title, node_data);
		if (first_node) {
			first_node = false;
		} else {
			r_agraph_add_edge (g, prev_node, chunk_node);
		}
		prev_node = chunk_node;
	}
	top = r_agraph_add_node (g, top_title, top_data);
	if (!first_node) {
		r_agraph_add_edge (g, prev_node, top);
		free (node_data);
		free (node_title);
	}
	r_agraph_print (g);
	r_cons_canvas_free (can);
	r_config_restore (hc);
	r_config_hold_free (hc);
	free (cnk);
	free (g);
	free (prev_c);
	free (top_data);
	free (top_title);
}

static void GH(print_tcache_instance)(RCore *core, MallocState *main_arena, GHT *initial_brk) {
	if (!core || !core->dbg || !core->dbg->maps) {
		return;
	}
	const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");
	if (!tcache) {
		return;
	}
	GHT align = 16;
	GHT brk_start = GHT_MAX, brk_end = GHT_MAX, tcache_fd = GHT_MAX;
	GH(get_brks) (core, &brk_start, &brk_end);
	GHT tcache_tmp = GHT_MAX;

	*initial_brk = ( (brk_start >> 12) << 12 ) + sizeof(GH(RHeapTcache)) + align;

	if (brk_start == GHT_MAX || brk_end == GHT_MAX || *initial_brk == GHT_MAX) {
		eprintf ("No heap section\n");
		return;
	}

	GH(RHeapTcache) *tcache_heap = R_NEW0 (GH(RHeapTcache));

	(void)r_io_read_at (core->io, brk_start + align, (ut8 *)tcache_heap, sizeof (GH(RHeapTcache)));

	PRINT_GA("Thread cache @\n");
	for (int i = 0; i < TCACHE_MAX_BINS; i++) {
		if (tcache_heap->counts[i] > 0) {
			PRINT_GA("bin :");
			PRINTF_BA("%2d",i);
			PRINT_GA(", items :");
			PRINTF_BA("%2d",tcache_heap->counts[i]);
			PRINT_GA(", fd :");
			PRINTF_BA("0x%"PFMT64x, (ut64)tcache_heap->entries[i] - align);
			if (tcache_heap->counts[i] > 1) {
				tcache_fd = (ut64)tcache_heap->entries[i];
				for(int n=1; n < tcache_heap->counts[i]; n++) {
					(void)r_io_read_at (core->io, tcache_fd, &tcache_tmp, sizeof (GHT));
					PRINTF_BA("->0x%"PFMT64x, tcache_tmp - align);
					tcache_fd = tcache_tmp;
				}
			}
			PRINT_BA("\n");
		}
	}
}

static void GH(print_heap_segment)(RCore *core, MallocState *main_arena, GHT *initial_brk, GHT global_max_fast) {
	if (!core || !core->dbg || !core->dbg->maps) {
		return;
	}

	GHT brk_start = GHT_MAX, brk_end = GHT_MAX, size_tmp, top_size = GHT_MAX, min_size = SZ * 4;
	GHT tcache_fd = GHT_MAX, tcache_tmp = GHT_MAX, align = 12 * SZ + sizeof (int) * 2;
	GH(get_brks) (core, &brk_start, &brk_end);
	const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");
	const int offset = r_config_get_i (core->config, "dbg.glibc.fc_offset");
	if (tcache) {
		align = 16;
		*initial_brk = ( (brk_start >> 12) << 12 ) + offset;
	}
	else {
		*initial_brk = (brk_start >> 12) << 12;
	}

	if (brk_start == GHT_MAX || brk_end == GHT_MAX || *initial_brk == GHT_MAX) {
		eprintf ("No heap section\n");
		return;
	}

	GHT next_chunk = *initial_brk, prev_chunk = next_chunk;
	top_size = main_arena->GH(top) - brk_start;
	GH(RHeapChunk) *cnk = R_NEW0 (GH(RHeapChunk));
	GH(RHeapChunk) *cnk_next = R_NEW0 (GH(RHeapChunk));
	if (!cnk || !cnk_next) {
		return;
	}

	(void)r_io_read_at (core->io, next_chunk, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
	size_tmp = (cnk->size >> 3) << 3;

	while (next_chunk && next_chunk >= brk_start && next_chunk < main_arena->GH(top)) {
		if (size_tmp < min_size || size_tmp > top_size || next_chunk + size_tmp > main_arena->GH(top)) {
			PRINT_YA ("\n  Malloc chunk @ ");
			PRINTF_BA ("0x%"PFMT64x" ", (ut64)next_chunk);
			PRINT_RA ("[corrupted]\n");
			PRINTF_RA ("   size: 0x%"PFMT64x"\n   fd: 0x%"PFMT64x", bk: 0x%"PFMT64x"\n",
			(ut64)cnk->size, (ut64)cnk->fd, (ut64)cnk->bk);
			break;
		}

		PRINT_YA ("\n  Malloc chunk @ ");
		PRINTF_BA ("0x%"PFMT64x" ", (ut64)next_chunk);

		PRINT_GA ("[size: ");
		PRINTF_BA ("0x%"PFMT64x, (ut64)cnk->size);
		PRINT_GA ("]");

		bool fastbin = size_tmp >= SZ * 4 && size_tmp <= global_max_fast;
		bool is_free = false, double_free = false;
		if (fastbin) {
			int i = (size_tmp / (SZ * 2)) - 2;
			GHT idx = (GHT)main_arena->GH(fastbinsY)[i];
			(void)r_io_read_at (core->io, idx, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
			GHT next = cnk->fd;
			if (prev_chunk == idx && idx && !next) {
				is_free = true;
			}
			while (next && next >= brk_start && next < main_arena->GH(top)) {
				if (prev_chunk == idx || prev_chunk == next || idx == next) {
					is_free = true;
					if (idx == next) {
						double_free = true;
						break;
					}
					(void)r_io_read_at (core->io, next, (ut8 *)cnk_next, sizeof (GH(RHeapChunk)));
					GHT next_node = cnk_next->fd;
					// avoid triple while?
					while (next_node && next_node >= brk_start && next_node < main_arena->GH(top)) {
						if (prev_chunk == next_node) {
							double_free = true;
							break;
						}
						(void)r_io_read_at (core->io, next_node, (ut8 *)cnk_next, sizeof (GH(RHeapChunk)));
						next_node = cnk_next->fd;
					}
					if (double_free) {
						break;
					}
				}
				(void)r_io_read_at (core->io, next, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
				next = cnk->fd;
			}
			if (double_free) {
				PRINT_RA (" Double free in simple-linked list detected ");
				break;
			}
		}

		if (tcache) {
			GH(RHeapTcache) *tcache_heap = R_NEW0 (GH(RHeapTcache));
			(void)r_io_read_at (core->io, brk_start + align, (ut8 *)tcache_heap, sizeof (GH(RHeapTcache)));
			for (int i=0; i < TCACHE_MAX_BINS; i++) {
				if (tcache_heap->counts[i] > 0) {
					if ((ut64)tcache_heap->entries[i] - SZ * 2 == (ut64)prev_chunk) {
						is_free = true;
						break;
					}
					if (tcache_heap->counts[i] > 1) {
						tcache_fd = (ut64)tcache_heap->entries[i];
						for (int n = 1; n < tcache_heap->counts[i]; n++) {
							(void)r_io_read_at (core->io, tcache_fd, (ut8*)&tcache_tmp, sizeof (GHT));
							if ((ut64)tcache_tmp - SZ * 2 == (ut64)prev_chunk) {
								is_free = true;
								break;
							}
							tcache_fd = (ut64)tcache_tmp;
						}
					}
				}
			}
		}

		next_chunk += size_tmp;
		prev_chunk = next_chunk;
		r_io_read_at (core->io, next_chunk, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
		size_tmp = (cnk->size >> 3) << 3;

		if (fastbin) {
			if (is_free) {
				PRINT_GA ("[free]");
			} else {
				PRINT_GA ("[allocated]");
			}
		} else {
			if (!(cnk->size & 1)) {
				PRINT_GA ("[free]");
			} else {
				if (tcache) {
					if (is_free) {
						PRINT_GA ("[free]");
					} else {
						PRINT_GA ("[allocated]");
					}
				} else {
					PRINT_GA ("[allocated]");
				}
			}
		}
	}

	PRINT_YA ("\n  Top chunk @ ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)main_arena->GH(top));
	PRINT_GA (" - [brk_start: ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)brk_start);
	PRINT_GA (", brk_end: ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)brk_end);
	PRINT_GA ("]\n");
	//r_cons_println (); giving me a compile error
	r_cons_printf ("\n");
	free (cnk);
	free (cnk_next);
}

static void GH(print_heap_segment_json)(RCore *core, MallocState *main_arena, GHT *initial_brk, GHT global_max_fast) {
	if (!core || !core->dbg || !core->dbg->maps) {
		return;
	}

	GHT brk_start = GHT_MAX, brk_end = GHT_MAX, size_tmp, top_size = GHT_MAX, min_size = SZ * 4;
	GH(RHeapChunk) *cnk = R_NEW0 (GH(RHeapChunk));

	if (!cnk) {
		return;
	}

	GH(get_brks) (core, &brk_start, &brk_end);
	const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");
	const int offset = r_config_get_i (core->config, "dbg.glibc.fc_offset");
	if (tcache) {
		*initial_brk = ( (brk_start >> 12) << 12 ) + offset;
	}
	else {
		*initial_brk = (brk_start >> 12) << 12;
	}

	if (brk_start == GHT_MAX || brk_end == GHT_MAX || *initial_brk == GHT_MAX) {
		eprintf ("No heap section\n");
		free (cnk);
		return;
	}

	GHT next_chunk = *initial_brk, prev_chunk = next_chunk;
	top_size = main_arena->GH(top) - brk_start;

	r_cons_printf ("{\"chunks\":[");
	const char *comma = "";
	while (next_chunk && next_chunk >= brk_start && next_chunk < main_arena->GH(top)) {
		(void)r_io_read_at (core->io, next_chunk, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
		size_tmp = (cnk->size >> 3) << 3;
		if (size_tmp < min_size || size_tmp > top_size || next_chunk + size_tmp > main_arena->GH(top)) {
			const char *status = "corrupted";
			r_cons_printf ("%s{\"addr\":%"PFMT64d",\"size\":%"PFMT64d",\"status\":\"%s\",\"fd\":"PFMT64d",\"bk\":"PFMT64d"}",
					comma, (ut64)next_chunk, (ut64)cnk->size, status, (ut64)cnk->fd, (ut64)cnk->bk);
			comma = ",";
			break;
		}
		bool is_free = false;
		GHT double_free = GHT_MAX;
		if (size_tmp >= SZ * 4 && size_tmp <= global_max_fast) {
			int i = (size_tmp / (SZ * 2)) - 2;
			GHT next = (GHT)main_arena->GH(fastbinsY)[i];
			double_free = next;
			while (next && next >= brk_start && next < main_arena->GH(top)) {
				if (prev_chunk == next) {
					is_free = true;
				}
				(void)r_io_read_at (core->io, next, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
				next = cnk->fd;
				if (double_free == next) {
					if (prev_chunk <= double_free) {
						PRINT_RA ("Double free detected ");
					}
					break;
				}
			}
		}

		const char *status = "allocated";
		if (is_free || (cnk->size % 2) == 0) {
			status = "free";
		}
		r_cons_printf ("%s{\"addr\":%"PFMT64d",\"size\":%"PFMT64d",\"status\":\"%s\"}",
				comma, (ut64)prev_chunk, (ut64)cnk->size, status);
		comma = ",";

		next_chunk += size_tmp;
		prev_chunk = next_chunk;
		r_io_read_at (core->io, next_chunk, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
	}
	r_cons_printf ("],");
	r_cons_printf ("\"top\":%"PFMT64d",", (ut64)main_arena->GH(top));
	r_cons_printf ("\"brk\":%"PFMT64d",", (ut64)brk_start);
	r_cons_printf ("\"end\":%"PFMT64d",", (ut64)brk_end);
	r_cons_printf ("\"hdr\":16");
	r_cons_printf ("}");
	free (cnk);
}

static void GH(print_heap_segment_r2)(RCore *core, MallocState *main_arena, GHT *initial_brk, GHT global_max_fast) {
	if (!core || !core->dbg || !core->dbg->maps) {
		return;
	}

	GHT brk_start = GHT_MAX, brk_end = GHT_MAX, size_tmp, top_size = GHT_MAX, min_size = SZ * 4;

	GH(get_brks) (core, &brk_start, &brk_end);
	const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");
	const int offset = r_config_get_i (core->config, "dbg.glibc.fc_offset");
	if (tcache) {
		*initial_brk = ( (brk_start >> 12) << 12 ) + offset;
	}
	else {
		*initial_brk = (brk_start >> 12) << 12;
	}

	if (brk_start == GHT_MAX || brk_end == GHT_MAX || *initial_brk == GHT_MAX) {
		eprintf ("No heap section\n");
		return;
	}

	GH(RHeapChunk) *cnk = R_NEW0 (GH(RHeapChunk));
	if (!cnk) {
		return;
	}
	GHT next_chunk = *initial_brk, prev_chunk = next_chunk;
	top_size = main_arena->GH(top) - brk_start;

	r_cons_printf ("fs+heap.allocated\n");
	while (next_chunk && next_chunk >= brk_start && next_chunk < main_arena->GH(top)) {
		(void)r_io_read_at (core->io, next_chunk, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
		size_tmp = (cnk->size >> 3) << 3;
		if (size_tmp < min_size || size_tmp > top_size || next_chunk + size_tmp > main_arena->GH(top)) {
			r_cons_printf ("fs heap.corrupted\n");
			char *name = r_str_newf ("chunk.corrupted.%06x", ((prev_chunk>>4) & 0xffff));
			r_cons_printf ("f %s %d 0x%"PFMT64x"\n", name, (int)cnk->size, prev_chunk);
			free (name);
			break;
		}
		bool is_free = false;
		GHT double_free = GHT_MAX;
		if (size_tmp >= SZ * 4 && size_tmp <= global_max_fast) {
			int i = (size_tmp / (SZ * 2)) - 2;
			GHT next = (GHT)main_arena->GH(fastbinsY)[i];
			double_free = next;
			while (next && next >= brk_start && next < main_arena->GH(top)) {
				if (prev_chunk == next) {
					is_free = true;
				}
				(void)r_io_read_at (core->io, next, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
				next = cnk->fd;
				if (double_free == next) {
					if (prev_chunk <= double_free) {
						PRINT_RA ("Double free detected ");
					}
					break;
				}
			}
		}

		const char *status = "allocated";
		if (is_free || (cnk->size % 2) == 0) {
			status = "free";
		}
		r_cons_printf ("fs heap.%s\n", status);
		char *name = r_str_newf ("chunk.%06x", ((prev_chunk>>4) & 0xffff));
		r_cons_printf ("f %s %d 0x%"PFMT64x"\n", name, (int)cnk->size, prev_chunk);
		free (name);

		next_chunk += size_tmp;
		prev_chunk = next_chunk;
		r_io_read_at (core->io, next_chunk, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
	}
	r_cons_printf ("fs-\n");
	r_cons_printf ("f heap.top = 0x%08"PFMT64x"\n", (ut64)main_arena->GH(top));
	r_cons_printf ("f heap.brk = 0x%08"PFMT64x"\n", (ut64)brk_start);
	r_cons_printf ("f heap.end = 0x%08"PFMT64x"\n", (ut64)brk_end);
	r_cons_printf ("f heap.hdr = 16\n");
	free (cnk);
}

static void GH(print_heap_mmaped)(RCore *core, GHT malloc_state, GHT global_max_fast) {
	if (!core || !core->dbg || !core->dbg->maps) {
		return;
	}

	GHT mmap_start = GHT_MAX, mmap_end = GHT_MAX, size_tmp;
	GHT top_size = GHT_MAX, min_size = SZ * 4;
	GH(RHeapChunk) *cnk = R_NEW0 (GH(RHeapChunk));
	MallocState *ms = R_NEW0 (MallocState);

	const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");
	if (tcache) {
		GH(RHeap_MallocState_tcache) *ma = R_NEW0 (GH(RHeap_MallocState_tcache));
		mmap_start = ((malloc_state >> 16) << 16) + sizeof (GH(RHeapInfo)) + sizeof(GH(RHeap_MallocState_tcache));
		r_io_read_at (core->io, malloc_state, (ut8*)ma, sizeof (GH(RHeap_MallocState_tcache)));
		GH(update_arena_with_tc)(ma,ms);
	}
	else {
		GH(RHeap_MallocState) *ma = R_NEW0 (GH(RHeap_MallocState));
		mmap_start = ((malloc_state >> 16) << 16) + sizeof (GH(RHeapInfo)) + sizeof(GH(RHeap_MallocState));
		r_io_read_at (core->io, malloc_state, (ut8*)ma, sizeof (GH(RHeap_MallocState)));
		GH(update_arena_without_tc)(ma,ms);
	}

	if (!cnk || !ms) {
		free (cnk);
		free (ms);
		return;
	}

	mmap_end = ms->GH(top);

	GHT next_chunk = mmap_start, prev_chunk = next_chunk;
	(void)r_io_read_at (core->io, ms->GH(top), (ut8*)cnk, sizeof (GH(RHeapChunk)));
	top_size = (cnk->size >> 3) << 3;

	while (next_chunk && next_chunk >= mmap_start && next_chunk < ms->GH(top)) {
		r_io_read_at (core->io, next_chunk, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
		size_tmp = (cnk->size >> 3) << 3;
		if (top_size < min_size || size_tmp > top_size) {
			PRINT_YA ("\n  Malloc chunk @ ");
			PRINTF_BA ("0x%"PFMT64x" ", (ut64)next_chunk);
			PRINT_RA ("[corrupted]\n");
			PRINTF_RA ("   size: %0x"PFMT64x"\n   fd: 0x%"PFMT64x", bk: 0x%"PFMT64x"\n",
			(ut64)cnk->size, (ut64)cnk->fd, (ut64)cnk->bk);
			break;
		}
		PRINT_YA ("\n  Malloc chunk @ ");
		PRINTF_BA ("0x%"PFMT64x" ", (ut64)prev_chunk);

		bool is_free = false;
		GHT double_free = GHT_MAX;
		if (size_tmp >= SZ * 4 && size_tmp <= global_max_fast) {
			int i = (size_tmp / (SZ * 2)) - 2;
			GHT next = ms->GH(fastbinsY)[i];
			double_free = next;
			while (next && next >= mmap_start && next < ms->GH(top)) {
				if (prev_chunk == next) {
					 is_free = true;
				}
				r_io_read_at (core->io, next, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
				next = cnk->fd;

				if (double_free == next) {
					if (prev_chunk <= double_free) {
						PRINT_RA ("Double free detected ");
					}
					break;
				}
			}
		}

		PRINT_GA ("[size: ");
		PRINTF_BA ("0x%"PFMT64x, (ut64)cnk->size);
		PRINT_GA ("]");

		if (is_free) {
			PRINT_GA ("[free]");
		} else  {
			if (cnk->size % 2 == 0) {
				PRINT_GA ("[free]");
			} else {
				PRINT_GA ("[allocated]");
			}
		}

		next_chunk += size_tmp;
		prev_chunk = next_chunk;
		r_io_read_at (core->io, next_chunk, (ut8 *)cnk, sizeof (GH(RHeapChunk)));
	}

	PRINT_YA ("\n  Top chunk @ ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)ms->GH(top));
	PRINT_GA (" - [mmap_start: ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)mmap_start);
	PRINT_GA (", mmap_end: ");
	r_io_read_at (core->io, ms->GH(top), (ut8*)cnk, sizeof(GH(RHeapChunk)));
	PRINTF_BA ("0x%"PFMT64x, (ut64) mmap_end + ((cnk->size >> 3) << 3));
	PRINT_GA ("]\n");
	// r_cons_println (); gives me a compile error
	r_cons_printf ("\n");
	free (cnk);
	free (ms);
}

void GH(print_malloc_states)( RCore *core, GHT m_arena, MallocState *main_arena) {
	MallocState *ta = R_NEW0 (MallocState);
	if (!ta) {
		return;
	}
	PRINT_YA ("main_arena @ ");
	PRINTF_BA ("0x%"PFMT64x"\n", (ut64)m_arena);
	if (main_arena->GH(next) != m_arena) {
		ta->GH(next) = main_arena->GH(next);
		while (ta->GH(next) != GHT_MAX && ta->GH(next) != m_arena) {
			PRINT_YA ("thread arena @ ");
			PRINTF_BA ("0x%"PFMT64x"\n", (ut64)ta->GH(next));
			r_io_read_at (core->io, ta->GH(next), (ut8 *)ta, sizeof (MallocState));
		}
	}
	free(ta);
}
void GH(print_inst_minfo)(GH(RHeapInfo) *heap_info, GHT hinfo) {
	PRINT_YA ("malloc_info @ ");
	PRINTF_BA ("0x%"PFMT64x, (ut64)hinfo);
	PRINT_YA (" {\n  ar_ptr = " );
	PRINTF_BA ("0x%"PFMT64x"\n", (ut64)heap_info->ar_ptr);
	PRINT_YA ("  prev = ");
	PRINTF_BA ("0x%"PFMT64x"\n", (ut64)heap_info->prev);
	PRINT_YA ("  size = ");
	PRINTF_BA ("0x%"PFMT64x"\n", (ut64)heap_info->size);
	PRINT_YA ("  mprotect_size = ");
	PRINTF_BA ("0x%"PFMT64x"\n", (ut64)heap_info->mprotect_size);
	PRINT_YA ("}\n\n");
}

void GH(print_malloc_info)(RCore *core, GHT m_state) {
	GHT malloc_state = core->offset, h_info;

	if (malloc_state == m_state) {
		PRINT_RA ("main_arena does not have an instance of malloc_info\n");
	} else {
		h_info = (malloc_state >> 16) << 16;
		GH(RHeapInfo) *heap_info = R_NEW0 (GH(RHeapInfo));
		r_io_read_at (core->io, h_info, (ut8*)heap_info, sizeof (GH(RHeapInfo)));
		GH(print_inst_minfo) (heap_info, h_info);
		MallocState *ms = R_NEW0 (MallocState);

		while (heap_info->prev != 0x0 && heap_info->prev != GHT_MAX) {

			const int tcache = r_config_get_i (core->config, "dbg.glibc.tcache");
			if (tcache) {
				GH(RHeap_MallocState_tcache) *ma = R_NEW0 (GH(RHeap_MallocState_tcache));
				r_io_read_at (core->io, malloc_state, (ut8*)ma, sizeof (GH(RHeap_MallocState_tcache)));
				GH(update_arena_with_tc)(ma,ms);
			}
			else {
				GH(RHeap_MallocState) *ma = R_NEW0 (GH(RHeap_MallocState));
				r_io_read_at (core->io, malloc_state, (ut8*)ma, sizeof (GH(RHeap_MallocState)));
				GH(update_arena_without_tc)(ma,ms);
			}

			if ((ms->GH(top) >> 16) << 16 != h_info) {
				h_info = (ms->GH(top) >> 16) << 16;
				r_io_read_at (core->io, h_info, (ut8*)heap_info, sizeof (GH(RHeapInfo)));
				GH(print_inst_minfo) (heap_info, h_info);
			}
		}
		free (heap_info);
		free (ms);
	}
}

static const char* GH(help_msg)[] = {
	"Usage:", " dmh", " # Memory map heap",
	"dmh", "", "List chunks in heap segment",
	"dmh", " [malloc_state]", "List heap chunks of a particular arena",
	"dmha", "", "List all malloc_state instances in application",
	"dmhb", "", "Display all parsed Double linked list of main_arena's bins instance",
	"dmhb", " [bin_num|bin_num:malloc_state]", "Display parsed double linked list of bins instance from a particular arena",
	"dmhbg"," [bin_num]", "Display double linked list graph of main_arena's bin [Under developemnt]",
	"dmhc", " @[chunk_addr]", "Display malloc_chunk struct for a given malloc chunk",
	"dmhf", "", "Display all parsed fastbins of main_arena's fastbinY instance",
	"dmhf", " [fastbin_num|fastbin_num:malloc_state]", "Display parsed single linked list in fastbinY instance from a particular arena",
	"dmhg", "", "Display heap graph of heap segment",
	"dmhg", " [malloc_state]", "Display heap graph of a particular arena",
	"dmhi", " @[malloc_state]", "Display heap_info structure/structures for a given arena",
	"dmhm", "", "List all elements of struct malloc_state of main thread (main_arena)",
	"dmhm", " [malloc_state]", "List all malloc_state instance of a particular arena",
	"dmht", "", "Display all parsed thead cache bins of main_arena's tcache instance",
	"dmh?", "", "Show map heap help",
	NULL
};

static int GH(cmd_dbg_map_heap_glibc)(RCore *core, const char *input) {
	static GHT m_arena = GHT_MAX, g_max_fast = GHT_MAX, initial_brk = GHT_MAX;
	MallocState *main_arena = R_NEW0 (MallocState);
	GHT global_max_fast = GHT_MAX;
	if (!main_arena) {
		return false;
	}
	switch (input[0]) {
	case '\0': // dmh
		if (GH(r_resolve_main_arena) (core, &m_arena, main_arena) && GH(r_resolve_global_max_fast) (core, &g_max_fast, &global_max_fast)) {
			GH(print_heap_segment) (core, main_arena, &initial_brk, global_max_fast);
		}
		break;
	case ' ' : // dmh [malloc_state]
		if (GH(r_resolve_main_arena) (core, &m_arena, main_arena)) {
			GHT m_state = strstr (input, "0x") // TODO: use r_num_get (NULL, input);
				? (GHT)strtol (input, NULL, 0)
				: (GHT)strtol (input, NULL, 16);
			if (m_state == m_arena) {
				GH(r_resolve_global_max_fast) (core, &g_max_fast, &global_max_fast);
				GH(print_heap_segment) (core, main_arena, &initial_brk, global_max_fast);
			} else {
				global_max_fast = GH(r_resolve_global_max_fast) (core, &g_max_fast, &global_max_fast)
					? global_max_fast
					: SZ * 0x10;
				GH(print_heap_mmaped)(core, m_state, global_max_fast);
 			}
		}
		break;
	case 'a': // dmha
		if (GH(r_resolve_main_arena) (core, &m_arena, main_arena)) {
			GH(print_malloc_states) (core, m_arena, main_arena);
		}
		break;
	case 'i': // dmhi
		if (GH(r_resolve_main_arena) (core, &m_arena, main_arena)) {
			GH(print_malloc_info) (core, m_arena);
		}
		break;
	case '*':
		if (GH(r_resolve_main_arena) (core, &m_arena, main_arena) && GH(r_resolve_global_max_fast) (core, &g_max_fast, &global_max_fast)) {
			GH(print_heap_segment_r2) (core, main_arena, &initial_brk, global_max_fast);
		}
        break;
	case 'm': // "dmhm"
		if (GH(r_resolve_main_arena) (core, &m_arena, main_arena) &&
			GH(r_resolve_global_max_fast) (core, &g_max_fast, &global_max_fast)) {
			input += 1;
			if (!strcmp (input, "\0")) {
				GH(print_main_arena) (core, m_arena, main_arena, global_max_fast, *input);
			} else {
				GHT m_state = strstr (input, "0x")
					? (GHT)strtol (input, NULL, 0)
					: (GHT)strtol (input, NULL, 16);
				MallocState *malloc_state = R_NEW0 (MallocState);
				(void) r_io_read_at (core->io, m_state, (ut8*)malloc_state, sizeof (MallocState));
				GH(print_main_arena) (core, m_state, malloc_state, global_max_fast, *input);
				free (malloc_state);
			}
		}
		break;
	case 'b': // "dmhb"
		if (GH(r_resolve_main_arena) (core, &m_arena, main_arena)) {

			if (!strstr (input + 1, ":")) {
				GH(print_heap_bin) (core, m_arena, main_arena, input+1);
			} else {
				char *m_state_str, *bin, *dup = strdup (input + 1);
				bin = strtok (dup, ":");
				m_state_str = strtok (NULL, ":");
				GHT m_state = strstr (m_state_str, "0x")
					? (GHT) strtol (m_state_str, NULL, 0)
					: (GHT)strtol (m_state_str, NULL, 16);
				MallocState *malloc_state = R_NEW0 (MallocState);
				(void)r_io_read_at (core->io, m_state, (ut8*)malloc_state, sizeof (MallocState));
				GH (print_heap_bin) (core, m_state, malloc_state, bin);
				free (malloc_state);
				free (dup);
			}
		}
		break;
	case 'c': // "dmhc"
		if (GH(r_resolve_main_arena)(core, &m_arena, main_arena)) {
            		GH(print_heap_chunk) (core);
		}
		break;
	case 'f': // "dmhf"
		if (GH(r_resolve_main_arena) (core, &m_arena, main_arena) &&
			GH(r_resolve_global_max_fast) (core, &g_max_fast, &global_max_fast)) {

			if (!strchr (input + 1, ':')) {
				GH(print_heap_fastbin) (core, m_arena, main_arena, global_max_fast, input+1);
			} else {
				char *m_state_str, *bin, *dup = strdup (input+1);
				bin = strtok (dup, ":");
				m_state_str = strtok (NULL, ":");
				GHT m_state = strstr (m_state_str, "0x")
					? (GHT)strtol (m_state_str, NULL, 0)
					: (GHT)strtol (m_state_str, NULL, 16);
				MallocState *malloc_state = R_NEW0 (MallocState);
				r_io_read_at (core->io, m_state, (ut8*)malloc_state, sizeof (MallocState));
				GH(print_heap_fastbin) (core, m_state, malloc_state, global_max_fast, bin);
				free (malloc_state);
				free (dup);
			}
		}
		break;
	case 'g': // "dmhg"
		if (GH (r_resolve_main_arena) (core, &m_arena, main_arena)) {
			input += 1;
			if (!strcmp (input, "\0")) {
				GH(print_heap_graph) (core, main_arena, &initial_brk);
			} else {
				GHT m_state = strstr (input, "0x")
					? (GHT)strtol (input, NULL, 0)
					: (GHT)strtol (input, NULL, 16);
				if (m_state == m_arena) {
					GH (print_heap_graph) (core, main_arena, &initial_brk);
				} else {
					MallocState *malloc_state = R_NEW0 (MallocState);
					(void)r_io_read_at (core->io, m_state, (ut8*)malloc_state, sizeof (MallocState));
					GH (print_mmap_graph) (core, malloc_state, m_state);
					free (malloc_state);
				}
			}
		}
		break;
	case 'j': // "dmhj"
		if (GH(r_resolve_main_arena) (core, &m_arena, main_arena) && GH(r_resolve_global_max_fast) (core, &g_max_fast, &global_max_fast)) {
			GH(print_heap_segment_json) (core, main_arena, &initial_brk, global_max_fast);
		}
		break;
	case 't':
		if (GH(r_resolve_main_arena) (core, &m_arena, main_arena) && GH(r_resolve_global_max_fast) (core, &g_max_fast, &global_max_fast)) {
			GH(print_tcache_instance) (core, main_arena, &initial_brk);
		}
		break;
	case '?':
		r_core_cmd_help (core, GH(help_msg));
		break;
	}
	free (main_arena);
	return true;
}
