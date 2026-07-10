Status: Active
Source Idea Path: ideas/open/665_aarch64_instruction_dispatch_internal.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh AArch64 Dispatch And Publication Evidence

# Current Packet

## Just Finished

Step 1 refreshed the focused AArch64 evidence for rows 284 and 322 without
implementation changes.

Row 284 first owner: AArch64 block dispatch entry/formal publication for
stack-passed f128 HFA lanes. The fresh failure is
`backend_aarch64_instruction_dispatch`, at
`block_dispatch_seeds_stack_passed_f128_hfa_formals_with_none_home`, with the
message `expected stack-passed f128 HFA formals to seed local carriers before
return`. The fixture expects two incoming stack-to-local q-register carrier
publications before the return:
`ldr q16, [sp, #64]\nstr q16, [sp]` and
`ldr q16, [sp, #80]\nstr q16, [sp, #16]`.

Row 322 first owner: AArch64 prepared call-argument publication metadata for
variadic FPR frame-slot arguments. The fresh failure is
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
missing the prepared dump snippet for `arg index=8 value_bank=vreg
source_encoding=frame_slot source_value_id=2728 source_slot=#3138
source_stack_offset=8224 source_bank=fpr dest_bank=none dest_stack_offset=0`.
The dump is otherwise on `target=aarch64-linux-gnu route=semantic_bir_shared`
and emits prepared call facts, so the first owner is AArch64 prepared
call-publication classification/exposure, not generic CLI formatting.

Row 322 should stay on this AArch64 route for the next packet. It does not need
a lifecycle split unless Step 2 proves the missing `vreg`/FPR frame-slot
argument facts are fully present before printing and only the dump surface is
stale.

## Suggested Next

Delegate Step 2 to inspect the AArch64 internal boundary shared by stack-passed
f128 HFA formal publication and variadic FPR frame-slot call-argument
publication. Select one narrow repair surface before any implementation edits.

## Watchouts

- Keep RV64 runtime, RISC-V object emission, byval, stack fan-in, and LLVM
  torture rows out of this route.
- Do not treat expectation edits, unsupported-marker changes, allowlist edits,
  timeout changes, runtime policy changes, or baseline accounting as progress.
- Reject testcase-shaped dispatch-table shortcuts and CLI-format-only claims
  while AArch64 facts or dispatch coverage are missing.
- Row 322 has nearby emitted frame-slot FPR argument facts with `value_bank=fpr`
  in `test_after.log`, but the required row-322 contract asks for specific
  `value_bank=vreg` records for value ids 2728 and 2732. Do not rewrite the
  expected snippets until the AArch64 prepared fact owner is inspected.

## Proof

Ran exact delegated proof:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1
```

Result: failed as expected for evidence refresh, with both selected rows still
failing. Proof log: `test_after.log`.
