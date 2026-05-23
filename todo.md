Status: Active
Source Idea Path: ideas/open/aarch64-codegen-prepared-boundary-recovery.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Dispatch and Calls Responsibilities

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit of AArch64 `dispatch_*` and `calls_*`
responsibilities. Classification is below.

Working-model buckets:

- Target-specific instruction selection or printing, stays in AArch64:
  `dispatch_branch_fusion` branch/compare fusion, AArch64 condition suffixes,
  compare immediates, emitted-register operand spelling, and branch machine
  instruction construction; `dispatch_value_materialization` scalar/FP/global/
  pointer/va-list materialization into AArch64 registers; `dispatch_store_sources`
  store-local/store-global instruction selection, scalar conversions, global
  symbol address emission, and pending publication emission; `dispatch_entry_formals`
  entry formal load/store opcodes, AArch64 register views, register-bank
  checks, and formal publication instructions; `dispatch_dynamic_stack` helper
  call lowering and stack-home address materialization; `dispatch_calls`
  actual call lowering plus scalar call argument producer lowering; `calls_printing`
  call, call-boundary move, and ABI-binding instruction records/printers;
  `calls_byval_aggregates` chunk widths, scratch registers, lane load/store
  mnemonics, and AArch64 aggregate copy/lane emission; `calls_argument_sources`
  register-name/view parsing, AArch64 operand construction, sret/frame-slot/
  aggregate/indirect-callee/memory-return operands; `calls_moves` and
  `calls_preservation` machine-instruction construction for prepared move
  bundles and preserved values.

- Generic Prepared contract interpretation, candidate for `prealloc` or shared
  MIR support: `dispatch_lookup::prepared_named_value_id`,
  `dispatch_lookup::prepared_value_id`, and both
  `dispatch_lookup::find_value_home` overloads; the duplicate anonymous
  `prepared_value_id`/`find_value_home` helpers in `dispatch_entry_formals`;
  `dispatch_publication_common::find_frame_slot` and `find_stack_object`;
  `dispatch_publication::prepared_local_load_offset`,
  `local_slot_address_frame_offset`, and
  `local_aggregate_address_frame_offset`; `dispatch_edge_copies` prepared edge
  copy lookup/matching helpers such as `prepared_memory_access` and
  `prepared_memory_access_matches_instruction`; `calls_common` size/alignment
  and fixed-frame/variadic stack-offset interpretation helpers; `calls_effects`
  conversion from prepared clobbers/preserved values/operands into effect
  resources; `calls_preservation` prepared block dominance, move-bundle lookup,
  prior-preserved-value lookup, and later-use queries.

- Missing Prepared fact, candidate to compute earlier in `bir` or `prealloc`:
  repeated same-block producer searches in `dispatch_producers`,
  `dispatch_branch_fusion`, `dispatch_lookup`, and `dispatch_edge_copies`;
  join-parallel-copy source tests and named operand scans in `dispatch.cpp`;
  call-boundary source-to-destination recovery in `dispatch_calls`; local slot
  spelling/offset parsing in `dispatch_store_sources` and
  `dispatch_publication`; dynamic stack helper recognition by callee spelling
  in `dispatch_dynamic_stack`; byval register-lane size/source recovery in
  `calls_byval_aggregates`; register-name to view/size recovery in
  `calls_argument_sources`.

- Local publication/helper glue, candidate for later consolidation after the
  ownership boundary is clearer: `dispatch_publication_common` address string,
  relocation, scalar mnemonic/register-view helpers, and alias checks;
  `dispatch_publication` current-block entry publication bookkeeping,
  publication read/clobber checks, and instruction result extraction;
  `dispatch_edge_copies` select-chain labels and edge value publication
  emission; `dispatch_calls` address materialization reload sorting and call
  result-source recording; `calls_common::align_to`, register display/occupied
  view helpers, and diagnostic forwarding; common instruction wrapper helpers
  in `calls_moves`, `calls_preservation`, and `calls_printing`.

Recommended Step 2 first move:

- Move the generic Prepared value-home/name lookup responsibility out of
  AArch64 codegen: introduce shared/prealloc support for looking up a
  `PreparedValueId` from a `ValueNameId` and a `PreparedValueHome` from either
  id or name, then replace the AArch64-local helpers in `dispatch_lookup.cpp`
  and the duplicate anonymous copies in `dispatch_entry_formals.cpp`.
- Rationale: this is target-independent Prepared contract interpretation,
  already duplicated inside AArch64, and it does not require changing machine
  instruction selection, register spelling, tests, or backend expectations.
  AArch64-specific consumers such as `make_named_prepared_result_register`
  should remain in AArch64 because they parse/register-view target names.

## Suggested Next

Execute `plan.md` Step 2 by moving only the Prepared value-home/name lookup
helpers to shared/prealloc support and replacing the two AArch64-local copies.

## Watchouts

- Do not perform header-family consolidation or `.cpp` merging in this plan.
- Do not weaken tests, expectations, or backend contracts.
- Reject named-case shortcuts and target-specific special cases outside
  AArch64.
- Keep AArch64 register parsing, register-view selection, instruction records,
  and printer logic in AArch64. Only the target-independent Prepared lookup
  contract should move in the recommended first slice.

## Proof

Docs/audit-only packet; no build or test proof required, and no
`test_after.log` created.
