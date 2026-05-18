Status: Active
Source Idea Path: ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Nearby Same-Family Sampling and Boundary Separation

# Current Packet

## Just Finished

Step 4 nearby same-family sampling is complete for `00019.c`, `00137.c`, and
`00138.c`. Generated-code evidence showed one in-scope same-surface break:
`00019.c` computed the local struct base address with a same-index frame-slot
materialization, but dispatch emitted only the materialization and skipped the
real scalar pointer producer feeding the following `StoreLocal`.

`00019.c` before repair: semantic BIR had `%lv.s = bir.add ptr %lv.s.0, 0`
followed by `bir.store_local %lv.s.0, ptr %lv.s`; generated AArch64 emitted
`mov x13, sp` but then stored stale `x19` with `str x19, [sp]`, causing the
self-pointer chain to load through an uninitialized pointer. The repair in
`src/backend/mir/aarch64/codegen/dispatch.cpp` narrowly co-emits a same-index
address materialization plus scalar pointer producer only when that scalar
result immediately feeds the next pointer `StoreLocal`. Generated AArch64 now
emits `mov x13, sp`, `add x19, x13, #0`, and `str x19, [sp]`; `00019.c` passes.

`00137.c` and `00138.c` are separated from the active address/lvalue family.
Their string byte loads and compares are generated correctly from `.str0`, but
the selected branch writes the return value into `x13` (`mov x13, #0` or
`mov x13, #1`) and returns without publishing that phi/control result to `w0`.
They remain return/control-value publication failures, not pointer-derived
lvalue authority failures.

## Suggested Next

Ask the supervisor to review/accept the Step 4 `00019.c` same-family repair and
boundary separation. A separate plan packet should own `00137.c` and `00138.c`
if return/control-value publication is in scope for a later initiative.

## Watchouts

- Do not repair this by naming the four tests or rewriting expectations. The
  address fact must become an authoritative value/materialization that ordinary
  load/store/call consumers can use.
- Do not reintroduce common semantic BIR pointer publications for x86; the
  regression subset is specifically guarding `%t0 = bir.load_local ptr %lv.p`
  and related load-local forms.
- `00130.c` now passes in the delegated subset; preserve the new byte-memory
  and emitted-byte fused-branch behavior when taking later pointer packets.
- `00180.c` now has the missing `%t5` address fact and passes in the delegated
  subset; preserve nonzero local array/subobject pointer publication through
  call arguments.
- The same-index direct-global materialization plus local store is now handled
  narrowly for pointer `StoreLocal`: do not widen the memory register selection
  globally. A broad memory-side exact-register preference made `00032.c` and
  `00130.c` exit nonzero and was reverted.
- The store-global producer publication is intentionally scoped: it publishes a
  same-block integer load/cast/add/sub chain into the prepared register that
  the existing `StoreGlobal` memory record already consumes. It does not name
  `00217.c` and does not rewrite expectations.
- Keep the BIR alias guard identity-based. Do not reintroduce raw-spelling
  recovery for string-pool globals without `TextId` or globals without
  `LinkNameId`.
- Step 4 should separate frontend, timeout, floating/conversion,
  string/library-only, aggregate initializer, and closed-owner overlap buckets
  before widening this source idea. Do not treat nearby sampling failures as
  address/lvalue wins without generated-code evidence of the same authority
  break.
- The new scalar/materialization co-emission is intentionally constrained to a
  scalar pointer result immediately consumed by the next pointer `StoreLocal`.
  A broader interception also caught `00180.c`'s call-argument producer and
  regressed it; keep the next-consumer guard.

## Proof

Ran the delegated proof:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00019|00137|00138|00032|00130|00180|00217)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

`test_after.log` records strict monotonic improvement over `test_before.log`:
5/7 now pass instead of 4/7. `00019.c`, `00032.c`, `00130.c`, `00180.c`, and
`00217.c` pass. `00137.c` and `00138.c` still fail with exit 1 and are
classified as separated return/control-value publication failures. The build
portion passed. `git diff --check` passed. Stale-process check found no stale
`ctest`, `c4cll`, `qemu`, or build/test process beyond the check command
itself.
