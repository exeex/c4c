# RV64 Duff Fallthrough Pointer Update Producers

## Goal

Repair semantic/prepared producer publication for pointer update values in
Duff's-device-style fallthrough copy blocks where later cases store next
`from`/`to` values into pointer locals without corresponding `bir.add ptr`
producer facts.

## Why This Exists

Idea 323 Step 5 reprobed `src/00143.c` after loop-carried pointer
post-increment consumption was repaired. The focused loop-carried pointer
post-increment route now passes, and the first Duff block can update pointer
locals from current pointer values. The remaining candidate failure is a
runtime segfault classified as
`separate-duff-fallthrough-pointer-update-publication-residual`.

The first bad block is `block_9`, reached by `count % 8 == 7`. BIR stores
`%t39` and `%t42` into `%lv.from` and `%lv.to`, but there is no `bir.add ptr`
producer for those next-pointer values:

```text
block_9:
  %t38 = bir.load_local ptr %lv.from
  bir.store_local %lv.from, ptr %t39
  %t40 = bir.load_local i16 %t40.addr, addr %t38
  %t41 = bir.load_local ptr %lv.to
  bir.store_local %lv.to, ptr %t42
  bir.store_local %t41.store.addr, i16 %t40, addr %t41
```

Prepared-BIR gives `%t39` and `%t42` stack homes with
`source_producer=unknown`; emitted RV64 then loads those uninitialized stack
homes and publishes them as pointer locals before qemu segfaults at
`0x0000000500000002`.

## In Scope

- Semantic/prepared producer facts for pointer update values in later Duff
  fallthrough copy blocks.
- Pointer local publication for `%lv.from`/`%lv.to`-style updates when the
  next pointer value should be produced from the current pointer value.
- Focused backend coverage that distinguishes missing producer facts from RV64
  consumption of producer facts that already exist.
- Candidate reprobe for `src/00143.c` after ideas 322 and 323 have advanced.

## Out Of Scope

- RV64 consumption of valid loop-carried pointer post-increment facts from idea
  323.
- Empty loop-exit successor emission from idea 322.
- I16 local-array select/store publication from idea 321.
- Stack-homed fused compare control flow from idea 319.
- Nested select-chain/store-source publication for `src/00144.c` from idea
  320.
- Aggregate/byval repair, function-pointer repair, external-call policy, or
  broad switch/control-flow rewrites beyond Duff fallthrough pointer update
  producer publication.

## Acceptance Criteria

- Focused tests cover later fallthrough pointer update blocks where next
  pointer values require explicit producer facts before being stored back into
  pointer locals.
- `src/00143.c` either emits, links, and runs under qemu, or any remaining
  failure is reclassified with concrete emitted-code evidence as a different
  mechanism after Duff fallthrough pointer update producers are present.
- Prepared facts no longer assign stack homes with `source_producer=unknown`
  to next pointer values that are immediately published as pointer locals in
  focused proof cases.
- Repairs create or preserve semantic/prepared pointer producer facts rather
  than teaching RV64 to guess from uninitialized stack homes.

## Completion Note

Closed on 2026-06-23 after Step 4/5 evidence satisfied the source idea. Focused
Duff fallthrough producer dump/codegen/runtime coverage is positive:

- `backend_dump_riscv64_duff_fallthrough_pointer_update_producers`
- `backend_codegen_route_riscv64_duff_fallthrough_pointer_update_producers`
- `backend_rv64_runtime_riscv64_duff_fallthrough_pointer_update_producers`

Probe artifacts under
`build/rv64_c_testsuite_probe_latest/triage_324_step4/` show the focused Duff
fixture, `src/00143.c`, and the clang reference for `src/00143.c` all emit,
link, and pass qemu. The old missing-producer publication residual is no
longer blocking this route, and no concrete follow-up residual was identified.

Close gate: existing accepted backend guard state passed with
`test_before.log` compared to itself using `--allow-non-decreasing-passed`:
287 passed, 1 failed before and after, with the existing
`backend_riscv_prepared_edge_publication` failure unchanged.

## Reviewer Reject Signals

- The implementation special-cases `src/00143.c`, Duff's-device block names,
  `%t39`, `%t42`, `%lv.from`, `%lv.to`, fixed stack offsets, or fixed array
  sizes instead of repairing missing pointer update producer facts.
- Progress is claimed by changing the fixture to avoid later Duff fallthrough
  entries such as the `count % 8 == 7` path.
- The route reopens RV64 consumption of already-valid pointer post-increment
  facts from idea 323 while later fallthrough blocks still have unknown source
  producers.
- Runtime proof is weakened, skipped, or replaced with unsupported
  expectations for a candidate that remains on the supported path.
- RV64 code is taught to consume uninitialized stack homes or synthesize
  pointer updates without prepared semantic authority.
