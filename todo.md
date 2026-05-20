Status: Active
Source Idea Path: ideas/open/344_semantic_bir_loop_carried_pointer_deref_provenance.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Pointer-Local Dereference Lowering

# Current Packet

## Just Finished

Step 2 repaired the semantic pointer-local load boundary in
`BirFunctionLowerer::try_lower_tracked_local_pointer_slot_load` and
`try_lower_local_slot_store`: AArch64 stores of local-slot-derived pointer
values into pointer locals now publish runtime pointer-slot provenance, and
later pointer-local loads prefer the current loaded pointer value before the
old direct local-slot address publication. The `00143` semantic/prepared BIR
Duff copy now loads `%lv.from`/`%lv.to`, advances those loaded pointer values,
and dereferences through pointer-value addresses such as `addr %t28` and
`addr %t31` instead of freezing dereferences to direct `%lv.a.0`/`%lv.b.0`
slots.

## Suggested Next

Step 3 should classify and repair the new post-semantic residual: the Duff copy
pointer-local BIR and generated copy loop now advance through the current
runtime pointer slots, but `00143` still exits 1 because later generated
AArch64 validation/selection code reads short array elements back with 32-bit
stack loads during the `a[n] != b[n]` comparison path.

## Watchouts

- The previous semantic first bad fact is gone: `/tmp/00143.after.semantic.bir`
  and `/tmp/00143.after.prepared.bir` show pointer-local loads and
  pointer-value dereferences in the Duff copy.
- Generated AArch64 for the copy body now reloads and updates pointer locals
  through `[sp,#160]`/`[sp,#168]`, then uses `ldrh`/`strh` through the loaded
  pointer registers. The remaining failure is not the stale direct local-slot
  dereference repaired in Step 2.
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

Supervisor backend validation: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'` passed, 141/141 backend tests.

Supporting probes were regenerated under `/tmp`: `/tmp/00143.after.semantic.bir`
via `--dump-bir`, `/tmp/00143.after.prepared.bir` via
`--dump-prepared-bir`, and `/tmp/00143.after.s` via `--codegen asm`.
