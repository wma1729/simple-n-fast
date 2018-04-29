#ifndef _SNF_TF_TESTMACROS_H_
#define _SNF_TF_TESTMACROS_H_

#if defined(_WIN32)

#define ASSERT_EQ(N1, N2, FMT, ...) do {                        \
    Log(DBG, name(), (FMT), __VA_ARGS__);                       \
    if ((N1) != (N2)) {                                         \
        Log(ERR, name(),                                        \
            "(" #N1 " == " #N2 ") failed at %s.%d\n",           \
            __FILE__, __LINE__);                                \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_NE(N1, N2, FMT, ...) do {                        \
    Log(DBG, name(), (FMT), __VA_ARGS__);                       \
    if ((N1) == (N2)) {                                         \
        Log(ERR, name(),                                        \
            "(" #N1 " != " #N2 ") failed at %s.%d\n",           \
            __FILE__, __LINE__);                                \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_MEM_EQ(S1, S2, N, FMT, ...) do {                 \
    Log(DBG, name(), (FMT), __VA_ARGS__);                       \
    if (memcmp((S1), (S2), (N)) != 0) {                         \
        Log(ERR, name(),                                        \
            "(" #S1 " == " #S2 ") failed at %s.%d\n",           \
            __FILE__, __LINE__);                                \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_MEM_NE(S1, S2, N, FMT, ...) do {                 \
    Log(DBG, name(), (FMT), __VA_ARGS__);                       \
    if (memcmp((S1), (S2), (N)) == 0) {                         \
        Log(ERR, name(),                                        \
            "(" #S1 " != " #S2 ") failed at %s.%d\n",           \
            __FILE__, __LINE__);                                \
        return false;                                           \
    }                                                           \
} while (0)

#else // !_WIN32

#define ASSERT_EQ(N1, N2, FMT, ...) do {                        \
    Log(DBG, name(), (FMT), ##__VA_ARGS__);                     \
    if ((N1) != (N2)) {                                         \
        Log(ERR, name(),                                        \
            "(" #N1 " == " #N2 ") failed at %s.%d\n",           \
            __FILE__, __LINE__);                                \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_NE(N1, N2, FMT, ...) do {                        \
    Log(DBG, name(), (FMT), ##__VA_ARGS__);                     \
    if ((N1) == (N2)) {                                         \
        Log(ERR, name(),                                        \
            "(" #N1 " != " #N2 ") failed at %s.%d\n",           \
            __FILE__, __LINE__);                                \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_MEM_EQ(S1, S2, N, FMT, ...) do {                 \
    Log(DBG, name(), (FMT), ##__VA_ARGS__);                     \
    if (memcmp((S1), (S2), (N)) != 0) {                         \
        Log(ERR, name(),                                        \
            "(" #S1 " == " #S2 ") failed at %s.%d\n",           \
            __FILE__, __LINE__);                                \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_MEM_NE(S1, S2, N, FMT, ...) do {                 \
    Log(DBG, name(), (FMT), ##__VA_ARGS__);                     \
    if (memcmp((S1), (S2), (N)) == 0) {                         \
        Log(ERR, name(),                                        \
            "(" #S1 " != " #S2 ") failed at %s.%d\n",           \
            __FILE__, __LINE__);                                \
        return false;                                           \
    }                                                           \
} while (0)

#endif

#endif // _SNF_TF_TESTMACROS_H_
