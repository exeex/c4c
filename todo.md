Status: Active
Source Idea Path: ideas/open/639_pointer_loaded_from_global_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh and classify pointer-loaded-from-global rows

# Current Packet

## Just Finished

Step 1 refreshed the three representative rows with the supervisor-selected
allowlist and classified their current owners.

Evidence artifacts are under
`build/agent_state/639_step1_pointer_loaded_from_global/`; the fixed allowlist
is `build/agent_state/639_step1_pointer_loaded_from_global.allowlist`.

| Row | First owner now | Policy bucket | Refreshed evidence |
| --- | --- | --- | --- |
| `src/pr46309.c` | `unsupported_local_memory_access` in the RV64 object route. | Genuine pointer-loaded-from-global local-memory candidate. | `bar`: `%t15 = bir.load_global ptr @q`, then `%t16 = bir.load_local i32 %t16.addr, addr %t15`. Prepared addressing has global-source load `symbol=q` with offset `0`, width `8`, align `8`, scalar layout, then pointer-value local load via `%t15` with selected offset `0`, width `4`, align `4`, unknown-compatible range. `main` publishes `@q` from `%lv.y`, but `store_source` for that global publication still reports `source_producer=unknown` and `source_freshness_status=no_candidate`. `%t15` is stored in a GPR, address space is default by absence of TLS/address-space markers on this access. |
| `src/pr58984.c` | `unsupported_call_abi` before the local-memory row is reached: `function=main; block=entry; instruction_index=23; callee=foo; args=1; planned_args=1; result=i32 %t7`. | Route out of idea 639 for the next packet: aggregate/byval call ABI first, with a later ordinary frame-slot pointer consumer behind it. | `foo` and `bar` contain `%t8 = bir.load_global ptr @c`, but the later local-memory store uses `%t14 = bir.load_local ptr %lv.i` followed by `bir.store_local ..., addr %t14`; `%lv.i` is populated from `%t7 = bir.add ptr %lv.f.0, 0`, not from `%t8`. Prepared addressing classifies the consumers as pointer-value accesses with selected offset `0`, width `8`, align `8`, bounded-by-element-count, while call plans show byval aggregate stack-copy transport for `foo`/`bar`. |
| `src/pr66556.c` | `unsupported_local_memory_access` in the RV64 object route. | Route out of direct pointer-loaded-from-global policy as currently evidenced: ordinary local pointer slot/global-array address-publication boundary. | HIR says `short *n = &i[4]` and global `i` is `short[5]`; BIR later has `%t23 = bir.load_local ptr %lv.n` and `bir.store_local ..., addr %t23`. Prepared addressing records `%lv.n` as a frame slot, then a pointer-value access through `%t23` with selected offset `0`, width `2`, align `2`, unknown-compatible range. There are unrelated `bir.load_global ptr @k`/`@f` rows in `fn3`, but the later local-memory consumer in `main` is not directly fed by them. Store-source evidence for `%lv.n` reports `source=%t1`, `source_producer=unknown`, and `source_freshness_status=no_candidate`, so global source identity/extent for the `&i[4]` publication is not available to RV64. |

## Suggested Next

Proceed to Step 2 using `src/pr46309.c` as the clean candidate: locate the
authority boundary that can legally carry `@q -> %t15` pointer identity,
freshness, extent, selected offset, width, and default address-space facts from
the producer side into the RV64 local-memory consumer without admitting
ordinary frame-slot, aggregate/byval, prepared value-location, or direct
global-symbol rows.

## Watchouts

- `pr58984` should not drive idea 639 while its first owner is the byval
  same-module call ABI. Its later `%lv.i` consumer is a frame-slot pointer
  publication, not a `%t8 = bir.load_global ptr @c` consumer.
- `pr66556` currently needs a producer/value-location audit for the `&i[4]`
  local pointer publication before it can be treated as a complete-authority
  pointer source. Do not infer the missing global-array identity from source
  text or the reused `%t1` name.
- For `pr46309`, the missing fact is not direct `addr @q` support: the local
  memory address is the pointer value `%t15` loaded from `@q`, and the current
  rejection remains fail-closed at `unsupported_local_memory_access`.

## Proof

Command:

```bash
cmake --build --preset default && ALLOWLIST=build/agent_state/639_step1_pointer_loaded_from_global.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1
```

Result: build succeeded; narrow probe failed all three refreshed rows as
expected for this evidence-only packet. `test_after.log` is the canonical proof
log. Focused `--dump-bir`, `--dump-prepared-bir`, `--dump-hir`, and
`--codegen obj` diagnostics for each target row are preserved under
`build/agent_state/639_step1_pointer_loaded_from_global/`.
