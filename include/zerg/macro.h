#pragma once

#define HAS_KEY(a, b) (((a).find(b)) != ((a).end()))
#define IS_ZERO(a) (fabs(a) < 1e-6)
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#define HAS_ELE(a, b) ((std::find((a).begin(), (a).end(), b)) != ((a).end()))
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
