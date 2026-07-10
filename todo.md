Status: Active
Source Idea Path: ideas/open/665_aarch64_instruction_dispatch_internal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The Selected AArch64 Rule

# Current Packet

## Just Finished

Step 3 repaired the selected AArch64 variadic HFA carrier expansion enough to
stop publishing the stale split-register/overflow lane order from
`src/backend/bir/lir_to_bir/calling.cpp`.

The required AST-backed lookup resolved
`append_aarch64_variadic_hfa_carrier_arg_lanes` at `calling.cpp:1593` and
`build_call_argument_source_relationships` at `calling.cpp:1814`; both are
local function-object definitions in the same lowering routine, so direct
callee queries do not apply.

The code now tracks the per-call AArch64 variadic FP register-lane count while
lowering arguments. `build_aapcs64_variadic_hfa_carrier_expansion` rejects an
expanded HFA carrier when it would straddle the remaining FP argument registers
instead of appending lanes into `lowered_args` and letting
`build_call_argument_source_relationships` mirror the stale order. This is a
general AAPCS64 HFA pressure rule, not a row-specific remap.

Before this packet, row 284 passed and row 322 failed after semantic BIR was
published with arg index 8 as `%t56.0` / `source_value_id=2721` and arg index
15 as `%t58.48` / `source_value_id=2728`. After this packet, row 284 still
passes and row 322 fails closed in semantic `lir_to_bir` direct-call lowering
before the bad BIR call/source relationship is emitted. No
CLI/expectation/regalloc/materializer guessing was introduced.

## Suggested Next

Continue Step 3 inside active idea 665. Row 322 remains in-scope because the
new first failure is the AArch64 prepared-BIR publication owner named by the
source idea: semantic `lir_to_bir` direct-call lowering now lacks a precise BIR
source representation for an AArch64 variadic HFA carrier that straddles the FP
register boundary and overflows to stack.

Delegate a bounded BIR-representation implementation packet before moving to
Step 4. The packet should model the straddling carrier's publication/source
relationship explicitly in BIR lowering, preserve the fail-closed diagnostic
for unsupported shapes, and only then re-open prepared/prealloc consumers. Do
not split or reroute row 322 unless fresh evidence proves the missing
representation is generic prepared/CLI exposure rather than AArch64
publication.

## Watchouts

- Row 284 now passes in the delegated proof; preserve the AArch64
  entry-formal gate as target-specific and f128-only unless a later packet
  proves a wider prepared-formal rule is required.
- Row 322 is now fail-closed inside the BIR lowering owner rather than emitting
  stale `call.args` and `arg_sources`.
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
failed closed during semantic `lir_to_bir` direct-call lowering for `stdarg`.
Overall delegated subset result: 1 of 2 tests passed, with `test_after.log`
preserved as the proof log.
