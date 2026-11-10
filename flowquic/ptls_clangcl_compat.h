/* picotls.h defines PTLS_BUILD_ASSERT_EXPR only under __GNUC__ >= 3, and clang-cl does not set
 * __GNUC__. PTLS_ASSERT_IS_ARRAY_EXPR (so PTLS_ELEMENTSOF) is gated on __clang__ instead, so under
 * clang-cl it is live and needs the macro. clang-cl has __builtin_constant_p, so this is the
 * definition from picotls.h's GNU branch, unchanged. flowquic/CMakeLists.txt checks at configure
 * time that PTLS_ELEMENTSOF accepts an array and refuses a pointer. */
#ifndef PTLS_BUILD_ASSERT_EXPR
#define PTLS_BUILD_ASSERT_EXPR(cond) (sizeof(char[2 * !!(!__builtin_constant_p(cond) || (cond)) - 1]) != 0)
#endif

/* wincompat.h does `#define __attribute__(X)` unless __attribute__ is already a macro. Emptied, it
 * breaks clang's own intrinsic headers (mmintrin.h and the rest), which picotls/openssl.c reaches.
 * clang-cl understands __attribute__, so make the macro exist and expand to itself: the #ifndef
 * in wincompat.h then skips the empty definition and nothing else changes. */
#ifndef __attribute__
#define __attribute__ __attribute__
#endif
