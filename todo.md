Status: Active
Source Idea Path: ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Core Pointer-Derived Lvalue Address Publication

# Current Packet

## Just Finished

Step 2 post-commit regression repair is complete. The AArch64 global GEP alias
publication now requires a valid `LinkNameId`, so raw-only/no-`TextId`
string-pool globals cannot recover through spelling-only aliases. The `00217.c`
starter repair remains intact: `char *data = t` still reaches semantic BIR as
`bir.store_local %lv.data, ptr @t`, and AArch64 dispatch still emits both the
same-index direct-global materialization and the real local pointer store.

Generated evidence: `00217.c` semantic BIR now contains the intended
load-compute-store chain:
`%t10 = bir.load_global i32 @t, offset 4`, `%t12 = bir.add i64 %t11, %t6`,
`%t13 = bir.trunc i64 %t12 to i32`, and `bir.store_global @t, offset 4, i32
%t13`. Generated AArch64 now emits `adrp x20, t`, `add x20, x20, :lo12:t`, and
`str x20, [sp]`, so `printf` receives the non-null `data` pointer. The
store-global value publication now emits the update chain before the existing
memory store: `ldr w13, [x9]` from `t+4`, `ldr w9, [sp, #24]`, `ldr x10, [sp,
#16]`, `sub x9, x9, x10`, `add x13, x13, x9`, then `str w13, [x9]` back to
`t+4`.

Regression evidence: the raw-only `.str.raw_only` fixture in
`backend_lir_to_bir_notes` now fails closed because its global has
`kInvalidLinkName`, while real linked globals such as `00217.c`'s `@t` still
publish the AArch64 alias through durable link identity.

## Suggested Next

Ask the supervisor to review/accept the regression repair or delegate the next
Step 2 packet. The notes regression and focused `00217.c` starter subset are
both green.

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

## Proof

Ran the delegated proof:

```sh
{ cmake --build build --target backend_lir_to_bir_notes_test c4cll && ctest --test-dir build -R '^backend_lir_to_bir_notes$' --output-on-failure && cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00032|00130|00180|00217)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

`test_after.log` records `backend_lir_to_bir_notes` passing and all four
AArch64 delegated tests passing: `00032.c`, `00130.c`, `00180.c`, and
`00217.c`. The passing `00217.c` test proves the generated output remains
`data = "0123-5678"`. Both build portions passed. `git diff --check` passed.
Stale-process check found no stale `ctest`, `c4cll`, `qemu`, or build/test
process beyond the check command itself.
