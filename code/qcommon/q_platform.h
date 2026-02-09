// Copyright (C) 1999-2005 ID Software, Inc.
// Copyright (C) 2023-2025 Noire.dev
// OpenSandbox — GPLv2; see LICENSE for details.

#define QDECL
#define ID_INLINE

// endianness
short ShortSwap(short l);
int LongSwap(int l);
float FloatSwap(const float *f);

#define LittleShort
#define LittleLong
#define LittleFloat
#define BigShort
#define BigLong
#define BigFloat
