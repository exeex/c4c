Status: Active
Source Idea Path: ideas/open/665_aarch64_instruction_dispatch_internal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Selected AArch64 Rule

# Current Packet

## Just Finished

Step 3 added an explicit BIR source-carrier representation for AArch64
variadic HFA lane expansion in `src/backend/bir/lir_to_bir/calling.cpp` and
`src/backend/bir/bir.hpp`.

The required AST-backed lookup covered the relevant BIR call-argument source
surface before editing: `CallArgumentSourceRelationship` in `bir.hpp`,
`build_aapcs64_variadic_hfa_carrier_expansion` plus
`build_call_argument_source_relationships` in `calling.cpp`,
`route6_call_argument_source_record` /
`route6_call_argument_publication_source_record` in
`bir_route6_call_publication.cpp`, and
`render_call_argument_source_annotation` in `bir_printer.cpp`.

The BIR relationship now records each expanded HFA lane's aggregate carrier
name, lane index, and lane count, and the printer exposes those fields in
`call_arg_source` annotations. Identifiable AArch64 variadic HFA carriers that
cross the remaining FP-register boundary now lower into explicit frame-slot
lane source relationships so existing AArch64 pressure handling can mark the
whole HFA group stack-passed. Carriers that cannot be matched to aggregate
aliases/local leaf slots still reject instead of guessing.

Before this packet, row 284 passed and row 322 failed closed in semantic
`lir_to_bir` direct-call lowering for `stdarg`. After this packet, row 284
still passes and row 322 advances to prepared output snippet matching: the
prepared dump now reaches `call block_index=0 inst_index=460 ... callee=myprintf`
but still reports `arg index=8` with `source_value_id=2721` / `%t56.0`; the
expected prepared owner wants that stack slot tied to `source_value_id=2728` /
`%t58.48` while retaining `source_slot=#3138` and `source_stack_offset=8224`.
No CLI/expectation/regalloc/materializer guessing was introduced.

## Suggested Next

Continue Step 3 with a bounded prepared/prealloc publication packet. Consume
the new BIR aggregate-carrier lane metadata for stack-passed AArch64 variadic
HFA groups so row 322's prepared call argument source identity matches the
carrier publication expected by the route, while preserving the explicit lane
frame-slot placement facts already present in the prepared output.

## Watchouts

- Row 284 now passes in the delegated proof; preserve the AArch64
  entry-formal gate as target-specific and f128-only unless a later packet
  proves a wider prepared-formal rule is required.
- Row 322 is no longer fail-closed in `lir_to_bir`; it reaches prepared BIR and
  fails snippet matching on the downstream prepared call-argument publication
  source identity for the straddling HFA stack slots.
- Lifecycle decision: keep row 322 in Step 3 as an AArch64 BIR publication
  representation repair; do not move to Step 4 until this bounded packet is
  attempted or proves the owner is different.
- Do not repair row 322 by making `append_call_arg_move_resolution` reinterpret
  `arg_index` after BIR has already assigned the wrong value to that index.
- Do not repair row 322 through CLI text formatting, expectation edits,
  unsupported-marker changes, allowlist edits, timeout changes, runtime policy
  changes, baseline accounting, or named-case shortcuts.

## Proof

Ran exact delegated proof:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1
```

Result: build succeeded, row 284 `backend_aarch64_instruction_dispatch` passed,
and row 322
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
failed at prepared dump snippet matching after semantic BIR and prepared BIR
were produced. Overall delegated subset result: 1 of 2 tests passed, with
`test_after.log` preserved as the proof log.
