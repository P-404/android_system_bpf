/* Common BPF helpers to be used by all BPF programs loaded by Android */

#include <linux/bpf.h>
#include <stdbool.h>
#include <stdint.h>

#include "bpf_map_def.h"

/* place things in different elf sections */
#define SEC(NAME) __attribute__((section(NAME), used))

/*
 * Helper functions called from eBPF programs written in C. These are
 * implemented in the kernel sources.
 */

/* generic functions */

/*
 * Type-unsafe bpf map functions - avoid if possible.
 *
 * Using these it is possible to pass in keys/values of the wrong type/size,
 * or, for 'bpf_map_lookup_elem_unsafe' receive into a pointer to the wrong type.
 * You will not get a compile time failure, and for certain types of errors you
 * might not even get a failure from the kernel's ebpf verifier during program load,
 * instead stuff might just not work right at runtime.
 *
 * Instead please use:
 *   DEFINE_BPF_MAP(foo_map, TYPE, KeyType, ValueType, num_entries)
 * where TYPE can be something like HASH or ARRAY, and num_entries is an integer.
 *
 * This defines the map (hence this should not be used in a header file included
 * from multiple locations) and provides type safe accessors:
 *   ValueType * bpf_foo_map_lookup_elem(const KeyType *)
 *   int bpf_foo_map_update_elem(const KeyType *, const ValueType *, flags)
 *   int bpf_foo_map_delete_elem(const KeyType *)
 *
 * This will make sure that if you change the type of a map you'll get compile
 * errors at any spots you forget to update with the new type.
 *
 * Note: these all take 'const void* map' because from the C/eBPF point of view
 * the map struct is really just a readonly map definition of the in kernel object.
 * Runtime modification of the map defining struct is meaningless, since
 * the contents is only ever used during bpf program loading & map creation
 * by the bpf loader, and not by the eBPF program itself.
 */
static void* (*bpf_map_lookup_elem_unsafe)(const void* map,
                                           const void* key) = (void*)BPF_FUNC_map_lookup_elem;
static int (*bpf_map_update_elem_unsafe)(const void* map, const void* key, const void* value,
                                         unsigned long long flags) = (void*)
        BPF_FUNC_map_update_elem;
static int (*bpf_map_delete_elem_unsafe)(const void* map,
                                         const void* key) = (void*)BPF_FUNC_map_delete_elem;

/* type safe macro to declare a map and related accessor functions */
#define DEFINE_BPF_MAP_NO_ACCESSORS(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries) \
    const struct bpf_map_def SEC("maps") the_map = {                                    \
            .type = BPF_MAP_TYPE_##TYPE,                                                \
            .key_size = sizeof(TypeOfKey),                                              \
            .value_size = sizeof(TypeOfValue),                                          \
            .max_entries = (num_entries),                                               \
    };

#define DEFINE_BPF_MAP(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries)                       \
    DEFINE_BPF_MAP_NO_ACCESSORS(the_map, TYPE, TypeOfKey, TypeOfValue, num_entries)              \
                                                                                                 \
    static inline __always_inline __unused TypeOfValue* bpf_##the_map##_lookup_elem(             \
            const TypeOfKey* k) {                                                                \
        return bpf_map_lookup_elem_unsafe(&the_map, k);                                          \
    };                                                                                           \
                                                                                                 \
    static inline __always_inline __unused int bpf_##the_map##_update_elem(                      \
            const TypeOfKey* k, const TypeOfValue* v, unsigned long long flags) {                \
        return bpf_map_update_elem_unsafe(&the_map, k, v, flags);                                \
    };                                                                                           \
                                                                                                 \
    static inline __always_inline __unused int bpf_##the_map##_delete_elem(const TypeOfKey* k) { \
        return bpf_map_delete_elem_unsafe(&the_map, k);                                          \
    };

static int (*bpf_probe_read)(void* dst, int size, void* unsafe_ptr) = (void*) BPF_FUNC_probe_read;
static int (*bpf_probe_read_str)(void* dst, int size, void* unsafe_ptr) = (void*) BPF_FUNC_probe_read_str;
static unsigned long long (*bpf_ktime_get_ns)(void) = (void*) BPF_FUNC_ktime_get_ns;
static int (*bpf_trace_printk)(const char* fmt, int fmt_size, ...) = (void*) BPF_FUNC_trace_printk;
static unsigned long long (*bpf_get_current_pid_tgid)(void) = (void*) BPF_FUNC_get_current_pid_tgid;
static unsigned long long (*bpf_get_current_uid_gid)(void) = (void*) BPF_FUNC_get_current_uid_gid;
static unsigned long long (*bpf_get_smp_processor_id)(void) = (void*) BPF_FUNC_get_smp_processor_id;
