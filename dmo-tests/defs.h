#pragma once

#if 1
#define DEBUG_SIGNAL(n,v) osmo_cxvec_dbg_dump(v, "/tmp/dbg_" n ".cfile");
#else
#define DEBUG_SIGNAL(n,v) do { } while (0)
#endif
