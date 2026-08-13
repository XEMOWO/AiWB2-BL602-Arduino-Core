/*
 * freestanding.cpp — C++ runtime stubs for the freestanding BL602 build.
 *
 * The link is driven by gcc (not g++), so libstdc++'s runtime support objects
 * are NOT pulled in. Anything that needs them — std::function's empty-call
 * guard, operator new/delete for WString, a stray pure-virtual call — must be
 * provided here. With -fno-exceptions the throw helpers can never throw, so
 * they trap; -ffunction-sections + --gc-sections drop any stub that's unused.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <new>
#include <memory> /* declares std::_Sp_make_shared_tag (_S_eq stub below) */

extern "C" void __attribute__((noreturn)) wb2_runtime_panic(void);

void __attribute__((noreturn)) wb2_runtime_panic(void)
{
    for (;;) {
        /* deliberate infinite loop: no exception machinery to unwind to */
    }
}

/* ---- operator new / delete (heap via newlib malloc) ---- */

void *operator new(std::size_t size)
{
    void *p = malloc(size);
    if (!p) {
        wb2_runtime_panic();
    }
    return p;
}

void *operator new[](std::size_t size)
{
    return ::operator new(size);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept
{
    return malloc(size);
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept
{
    return malloc(size);
}

void operator delete(void *p) noexcept
{
    free(p);
}

void operator delete[](void *p) noexcept
{
    free(p);
}

void operator delete(void *p, std::size_t) noexcept
{
    free(p);
}

void operator delete[](void *p, std::size_t) noexcept
{
    free(p);
}

/* ---- libstdc++ throw helpers (never called thanks to -fno-exceptions) ---- */

namespace std {

void __attribute__((noreturn)) __throw_bad_function_call(void)
{
    wb2_runtime_panic();
}

void __attribute__((noreturn)) __throw_bad_alloc(void)
{
    wb2_runtime_panic();
}

void __attribute__((noreturn)) __throw_length_error(char const *what)
{
    (void)what;
    wb2_runtime_panic();
}

void __attribute__((noreturn)) __throw_out_of_range(char const *what)
{
    (void)what;
    wb2_runtime_panic();
}

/* std::make_shared on _Sp_counted_ptr_inplace needs this libstdc++ runtime
 * symbol. With -fno-rtti we cannot use typeid() in the stub; callers already
 * compare against the fake _S_ti() address before falling through to _S_eq(),
 * so always-false is behaviourally identical for our code paths (get_deleter
 * just returns null for non-matching tag types). */
bool _Sp_make_shared_tag::_S_eq(const type_info &) noexcept
{
    return false;
}

} /* namespace std */

extern "C" void __cxa_pure_virtual(void)
{
    wb2_runtime_panic();
}
