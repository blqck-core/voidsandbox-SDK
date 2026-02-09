// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#define iferr(cond) do { \
    if (cond) { \
        error("Error: %s (%s:%d)", #cond, __FILE__, __LINE__); \
    } \
} while(0)

#define err(text) do { \
    error("Error: %s (%s:%d)", #text, __FILE__, __LINE__); \
} while(0)
   