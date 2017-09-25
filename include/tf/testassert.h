#ifndef _SNF_TF_TESTASSERT_H_
#define _SNF_TF_TESTASSERT_H_

#define ASSERT_NE(A, B, S) do {                                 \
    if ((A) == (B)) {                                           \
        fprintf(stderr,                                         \
            "(" #A " != " #B ") failed at %s.%d: %s\n",         \
            __FILE__, __LINE__, (S));                           \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_EQ(A, B, S) do {                                 \
    if ((A) != (B)) {                                           \
        fprintf(stderr,                                         \
            "(" #A " == " #B ") failed at %s.%d: %s\n",         \
            __FILE__, __LINE__, (S));                           \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_SEQ(S1, S2, N, S) do {                           \
    if (memcmp((S1), (S2), (N)) != 0) {                         \
        fprintf(stderr,                                         \
            "(" #S1 " == " #S2 ") failed at %s.%d: %s\n",       \
            __FILE__, __LINE__, (S));                           \
        return false;                                           \
    }                                                           \
} while (0)

#endif // _SNF_TF_TESTASSERT_H_
