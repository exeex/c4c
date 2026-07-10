Status: Active
Source Idea Path: ideas/open/665_aarch64_instruction_dispatch_internal.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select The AArch64 Internal Boundary

# Current Packet

## Just Finished

Step 2 selected the first AArch64 internal repair boundary without
implementation changes.

Selected owner for the first code packet: AArch64 entry-formal publication
lowering in `src/backend/mir/aarch64/codegen/prologue.cpp`, specifically the
`lower_entry_formal_publications` path through
`entry_formal_stack_source_publication_lines`. AST-backed lookup showed
`dispatch_prepared_block` calls `record_current_block_entry_publication_registers`
and then `lower_entry_formal_publications` before any instruction or call
lowering. The row-284 fixture has two stack-passed `f128` formals with
`PreparedValueHomeKind::None`; the existing helper already has a general
`home.kind == None && param.type == F128` stack-source branch that should
publish incoming stack lanes through a q scratch carrier.

Row 322 remains AArch64-owned, but it should split into a later packet inside
the AArch64 call-publication path, not into a lifecycle reroute. Its likely
boundary is `src/backend/mir/aarch64/codegen/calls.cpp` around
`materialize_missing_frame_slot_call_arguments` and
`find_prepared_frame_slot_call_argument_move`, because the current prepared dump
contains nearby `value_bank=vreg` frame-slot facts but reports the required
`source_value_id=2728` as `arg index=15 source_slot=#3145` instead of the
row-322 required `arg index=8 source_slot=#3138`. That is still AArch64
prepared call-argument publication/classification evidence, not generic CLI
formatting.

## Suggested Next

Delegate Step 3 as a code packet to repair only the selected
`lower_entry_formal_publications` / `entry_formal_stack_source_publication_lines`
boundary for stack-passed f128 formals with `None` homes. The expected narrow
delta is row 284 emitting the two `ldr q16, [sp, #64|#80]` plus `str q16,
[sp|#16]` entry-formal publications before the return, without touching row 322
yet.

## Watchouts

- Nearby row-284 guards in `backend_aarch64_instruction_dispatch` include
  `block_dispatch_uses_bank_local_indices_for_mixed_gpr_hfa_formals`,
  `block_dispatch_exposes_f128_constant_argument_carrier_to_selection`, and
  the f128 HFA call-boundary guard
  `f128_hfa_call_boundary_requires_structured_q_register_authority`.
- Keep malformed or unsupported entry-formal shapes fail-closed: missing BIR
  function context, missing incoming stack offset, missing frame size, missing
  f128/q scratch register, unsupported non-f128 `None` homes, invalid stack
  home offsets, unsupported register views, and byval aggregate paths that
  already have their own helper must continue to produce no publication rather
  than testcase-shaped assembly.
- Do not make row 284 pass through expectation edits, unsupported-marker
  changes, allowlist edits, timeout changes, runtime policy changes, baseline
  accounting, or named-case dispatch shortcuts.
- Do not include row 322 in the first code packet. It remains AArch64-owned for
  a later packet unless future inspection proves the AArch64 call facts are
  fully correct before printing and only dump exposure is stale.

## Proof

Ran exact delegated proof:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1
```

Result: failed as expected for this inspection-only boundary-selection packet.
The build completed with no work to do, then rows 284 and 322 failed with the
same boundaries: row 284 still reports `expected stack-passed f128 HFA formals
to seed local carriers before return`, and row 322 still misses
`arg index=8 value_bank=vreg source_encoding=frame_slot source_value_id=2728
source_slot=#3138 source_stack_offset=8224 source_bank=fpr dest_bank=none
dest_stack_offset=0` while the dump contains `source_value_id=2728` at
`arg index=15 source_slot=#3145 source_stack_offset=8336`. Proof log:
`test_after.log`.
