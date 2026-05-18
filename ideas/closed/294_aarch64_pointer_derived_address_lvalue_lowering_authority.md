# AArch64 Pointer-Derived Address/Lvalue Lowering Authority

Status: Closed
Created: 2026-05-18
Split From: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Closed: 2026-05-18

## Intent

Repair the AArch64 backend path where pointer-derived addresses, array-decayed
addresses, subobject offsets, and lvalue update addresses must be derived and
published to the loads, stores, and calls that consume them.

## Why This Exists

The refreshed post-293 inventory selected a coherent backend/runtime family
where generated AArch64 either loses the concrete object address, uses an
uninitialized pointer register, reads from an out-of-frame stack slot, applies
the wrong element offset, or updates only a partial fallback value instead of
the addressed lvalue.

Starter evidence from inventory Step 2:

- `src/00217.c` combines global string data, pointer offset arithmetic, a cast
  to `unsigned *`, and `*(unsigned *)(data + r) += a - b`; generated AArch64
  stores only `a` to `t+4`, ignores the old loaded value and `b`, then prints
  through an uninitialized pointer slot.
- `src/00032.c` uses a local `int arr[2]` with post/pre-increment pointer
  dereferences; generated AArch64 reads pointer values from out-of-frame stack
  offsets such as `[sp, #64]` instead of deriving `&arr[0]` and `&arr[1]`.
- `src/00130.c` uses a two-dimensional local char array, pointer-to-array,
  scalar `char *`, and `*q`; generated AArch64 reads `arr[1][3]` from
  `[sp, #2]`, showing broken array-element address scaling and offset
  authority.
- `src/00180.c` passes a local char buffer to `strcpy` and then prints from
  `&a[1]`; generated AArch64 calls `strcpy` with uninitialized `x20` instead
  of the local buffer address.

This owner is broader than one c-testsuite file. It covers the authority chain
that turns object identity plus pointer/array operations into the concrete
address consumed by AArch64 loads, stores, compound lvalue updates, and
address-valued call arguments.

## In Scope

- AArch64 derivation of concrete addresses for local arrays, global objects,
  array-to-pointer decay, pointer arithmetic, pointer increments, pointer to
  array indexing, casts to pointer types, and subobject offsets.
- Lvalue loads and stores that must consume a pointer-derived address rather
  than a fallback stack slot, stale scratch register, or uninitialized pointer.
- Compound lvalue updates such as `*p += value` where the backend must load
  the old object value, compute the new value, and store back through the same
  derived address.
- Address-valued direct-call arguments when the argument should be the address
  of a local buffer, global object, or subobject.
- Focused proof beginning with `src/00217.c`, `src/00032.c`, `src/00130.c`,
  and `src/00180.c`, then sampling nearby pointer-heavy cases such as
  `src/00019.c`, `src/00137.c`, `src/00138.c`, and string or indirect-call
  cases only after the first repair is proven.

## Out of Scope

- Frontend/admission failures from the current 52-case frontend bucket.
- Timeout or hang repair such as `src/00220.c`, including timeout policy and
  stale-process discipline.
- Floating or scalar-conversion cases such as `src/00174.c`, `src/00175.c`,
  and `src/00119.c`.
- String/library-only behavior unless generated-code evidence shows the string
  failure is caused by wrong pointer-derived address publication.
- Aggregate/global initializer work such as `src/00050.c` unless later
  evidence connects it directly to this address/lvalue lowering primitive.
- Closed-owner overlap such as `src/00159.c`, `src/00168.c`, `src/00193.c`,
  and `src/00196.c` unless fresh generated-code evidence contradicts the
  closed AArch64 owners 285-293.
- Runner behavior, expected outputs, allowlists, unsupported classifications,
  timeout policy, CTest registration, parser/sema rewrites, libc behavior, or
  broad aggregate ABI work.

## Acceptance Criteria

- Starter representatives `src/00217.c`, `src/00032.c`, `src/00130.c`, and
  `src/00180.c` no longer fail because generated AArch64 uses uninitialized
  pointer registers, out-of-frame stack offsets, wrong element offsets, or
  fallback slots for pointer-derived lvalues.
- Generated AArch64 assembly shows correct local/global object address
  derivation, pointer increment/decrement scaling, subobject offsets,
  address-valued call arguments, and lvalue load/store reuse of the same
  derived address.
- Compound lvalue updates load the old value from the derived address, compute
  the semantic result, and store the new value back through that address.
- Nearby same-family pointer/address samples are inspected enough to show the
  repair is semantic rather than filename-specific.
- Frontend, timeout, floating/conversion, string/library-only, aggregate
  initializer, and closed-owner overlap buckets remain separated unless fresh
  generated-code proof shows they share this exact pointer-derived address
  lowering primitive.
- No progress is claimed through expectation, runner, allowlist, timeout, or
  unsupported-classification changes.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `src/00217.c`, `src/00032.c`, `src/00130.c`, `src/00180.c`,
  one local variable name, one global symbol, one array shape, or one emitted
  instruction sequence instead of repairing pointer-derived address/lvalue
  authority;
- changes expected outputs, runner behavior, allowlists, unsupported
  classifications, timeout policy, CTest registration, or testcase contracts;
- claims capability progress through helper renames, classification rewrites,
  expectation churn, or source-test narrowing while generated AArch64 still
  consumes uninitialized pointer registers, out-of-frame stack offsets, wrong
  element offsets, or fallback slots for pointer-derived lvalues;
- broadens into frontend/admission, timeout-loop investigation,
  floating/conversion behavior, libc/string semantics, aggregate initializer
  layout, parser/sema, or broad aggregate ABI work before proving the focused
  pointer-derived address/lvalue owner;
- reopens a closed AArch64 owner from residual failing counts alone without
  generated-code evidence contradicting that owner's closure;
- preserves the old failure mode behind a new abstraction name where object or
  pointer facts exist but emission still does not publish the concrete address
  to the load, store, compound update, or call argument that consumes it.

## Closure Note

Closed after Step 4 boundary sampling. Starter representatives `src/00032.c`,
`src/00130.c`, `src/00180.c`, and `src/00217.c` are green, with generated
AArch64 evidence for local/global address derivation, byte/subobject loads,
address-valued call arguments, same-address compound lvalue updates, and
pointer-derived store-local publication.

Nearby same-family sampling repaired `src/00019.c`: generated AArch64 now
co-emits the same-index frame-slot materialization and scalar pointer producer
before storing the self-pointer chain, so the test passes. Step 4 also
inspected `src/00137.c` and `src/00138.c`; their string byte loads and
compares are generated from `.str0`, but the selected branch writes the return
value into `x13` and returns without publishing the phi/control result to
`w0`. Those residual failures are separated return/control-value publication
work, not pointer-derived address/lvalue authority.

Regression guard accepted the Step 4 before/after logs: the focused subset
improved from 4/7 to 5/7, resolving `c_testsuite_aarch64_backend_src_00019_c`
with no new failures. The full-suite baseline at commit `5b37c5906` was
accepted clean at 3159/3159. No progress was claimed through expectation,
runner, allowlist, timeout, or unsupported-classification changes.
