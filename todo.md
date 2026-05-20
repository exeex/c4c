Status: Active
Source Idea Path: ideas/open/344_semantic_bir_loop_carried_pointer_deref_provenance.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Representative And Reclassify Residuals

# Current Packet

## Just Finished

Step 3 proved the semantic pointer-local dereference repair remains in place
for `00143` and reclassified the remaining runtime failure outside this
semantic-provenance idea. The regenerated semantic/prepared BIR copy chain
still loads `%lv.from`/`%lv.to`, advances those loaded pointer values, and
dereferences through pointer-value addresses such as `addr %t28`/`addr %t31`
instead of stale direct `%lv.a.0`/`%lv.b.0` base slots.

`00143` still exits 1. The new first bad fact is in AArch64 scalar select
publication/result materialization: prepared BIR has stack-homed `i16`
select results such as `%t9.store0` feeding `bir.store_local %lv.a.0`, but
generated AArch64 computes `csel w9, ...` and then immediately reloads the
select result home, for example `[sp,#240]`, before storing to the array slot.
That reload uses an unpublished stack home for the select result. The concrete
owner is the AArch64 scalar select/result-publication path, centered on
`src/backend/mir/aarch64/codegen/alu.cpp` `lower_scalar_select_publication`
and its interaction with store-local value publication in
`src/backend/mir/aarch64/codegen/dispatch.cpp`.

## Suggested Next

Open a new AArch64-owned packet or idea for stack-homed scalar select result
publication before store/consumer reloads. Keep it separate from semantic BIR
loop-carried pointer provenance.

## Watchouts

- The previous semantic first bad fact is gone: `/tmp/00143.step3.semantic.bir`
  and `/tmp/00143.step3.prepared.bir` show pointer-local loads and
  pointer-value dereferences in the Duff copy.
- Generated AArch64 for the copy body now reloads and updates pointer locals
  through `[sp,#160]`/`[sp,#168]`, then uses `ldrh`/`strh` through the loaded
  pointer registers. The remaining failure is not the stale direct local-slot
  dereference repaired in Step 2.
- The first bad emitted-code fact appears before the final comparison path:
  array initialization/zeroing select-store sequences compute `csel` results
  but reload unpublished stack homes such as `%t9.store0`/`%t13.store0`.
- The final verification loop has related symptoms for stack-homed select
  chains (`%t76`/`%t80`) and wide `ldr` reloads from short-element slots, but
  the earliest concrete owner is still AArch64 scalar select result
  publication, not semantic pointer dereference lowering.
- Do not reopen fixed-offset fallthrough copy emission unless fresh evidence
  again shows generated AArch64 skips consecutive prepared short-copy offsets.
- Preserve idea 342's repaired latch behavior; this packet reconfirmed the
  latch branches on the single post-decrement counter value.
- Do not special-case `00143`, `%lv.from`, `%lv.to`, `%lv.a.0`, `%lv.b.0`,
  labels, block numbers, stack offsets, source lines, emitted instruction
  strings, expectations, unsupported lists, runner, timeout, CTest, or
  proof-log policy.

## Proof

Build: `cmake --build --preset default` passed.

Focused residual proof: `ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c' > test_after.log 2>&1` still fails with `[RUNTIME_NONZERO] .../00143.c exit=1`. Canonical proof log: `test_after.log`.

Supporting probes were regenerated under `/tmp`: `/tmp/00143.step3.semantic.bir`
via `--dump-bir`, `/tmp/00143.step3.prepared.bir` via
`--dump-prepared-bir`, and `/tmp/00143.step3.s` via `--codegen asm`.
