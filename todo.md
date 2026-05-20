Status: Active
Source Idea Path: ideas/open/344_semantic_bir_loop_carried_pointer_deref_provenance.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Loop-Carried Pointer Dereference Boundary

# Current Packet

## Just Finished

Step 1 localized the first semantic BIR boundary for the `00143` Duff copy
residual: loop-carried pointer-local state is lost in
`BirFunctionLowerer::try_lower_tracked_local_pointer_slot_load`
([src/backend/bir/lir_to_bir/memory/local_slots.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir/memory/local_slots.cpp:1658)).
For AArch64, a load of mutable pointer locals such as `%lv.from` and `%lv.to`
hits the `local_slot_address_slots_` path and
`append_local_slot_address_value`
([src/backend/bir/lir_to_bir/memory/local_slots.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir/memory/local_slots.cpp:124))
publishes direct local-slot addresses like `%lv.a.0 + const` and
`%lv.b.0 + const` instead of loading the current runtime value from the
pointer slot. The compile-time address map is updated by straight-line stores
in `try_lower_local_slot_store`
([src/backend/bir/lir_to_bir/memory/local_slots.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir/memory/local_slots.cpp:1267)),
so the single-pass semantic BIR body emits advancing constants within one
fallthrough chain, but the loopback re-enters already-lowered `block_6` with
the frozen `%lv.a.0`/`%lv.b.0` bases. Semantic/prepared BIR evidence:
`block_6` starts with `%t28 = bir.add ptr %lv.a.0, 0`, stores `%lv.from =
%lv.a.0 + 2`, then dereferences `%lv.a.0`; later blocks use `%lv.a.0+2`,
`+4`, ..., `+14`, and no loop-carried load of `%lv.from`/`%lv.to` survives.

## Suggested Next

Step 2 should repair the mutable pointer-local load boundary: when a pointer
slot has been stored/reloaded across control flow, preserve the current runtime
pointer value for dereference lowering instead of republishing stale
`local_slot_address_slots_` as direct local-slot provenance. The repair should
keep proven fixed local-slot lowering for non-loop-carried/exact static
addresses, but force `*from++`/`*to++` style loads after the store/reload cycle
through `PointerAddress`/runtime pointer-value addressing or an equivalent
control-flow-safe provenance state.

## Watchouts

- The first bad fact is already present in semantic BIR; prepared BIR and
  AArch64 are downstream witnesses, not first owners.
- `lower_memory_gep_inst` and `try_lower_local_slot_pointer_gep` consume the
  statically republished pointer bases, but the state has already been made
  stale by the pointer-local load path.
- Do not reopen fixed-offset fallthrough copy emission unless fresh evidence
  again shows generated AArch64 skips consecutive prepared short-copy offsets.
- Preserve idea 342's repaired latch behavior; this packet reconfirmed the
  latch branches on the single post-decrement counter value.
- Do not special-case `00143`, `%lv.from`, `%lv.to`, `%lv.a.0`, `%lv.b.0`,
  labels, block numbers, stack offsets, source lines, emitted instruction
  strings, expectations, unsupported lists, runner, timeout, CTest, or
  proof-log policy.

## Proof

Build: `cmake --build --preset default` passed (`ninja: no work to do`).

Focused residual proof: `ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c' > test_after.log 2>&1` failed as expected with `[RUNTIME_NONZERO] .../00143.c exit=1`. Canonical proof log: `test_after.log`.

Supporting probes were regenerated under `/tmp`: `/tmp/00143.semantic.bir` via
`--dump-bir`, and `/tmp/00143.prepared.bir` via `--dump-prepared-bir`.
