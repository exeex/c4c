Status: Active
Source Idea Path: ideas/open/558_bir_call_metadata_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Direct-Call Metadata Publication

# Current Packet

## Just Finished

Step 2 - Repair Direct-Call Metadata Publication completed for typed
`ptr null` pointer operands in metadata-rich direct calls.

`lower_call_pointer_arg_value` now admits non-SSA, non-global pointer operands
through existing `lower_value(..., TypeKind::Ptr, ...)` materialization, so
typed null pointer call arguments publish immediate null BIR values instead of
failing the direct-call semantic family. Focused coverage
`expect_metadata_rich_direct_call_null_pointer_argument_publishes_immediate_source`
now exercises a structured direct `(i32, ptr)` call with `ptr null` and checks
callee LinkNameId/signature identity, `arg_types[1] == Ptr`, pointer ABI
metadata, immediate null value facts, and an `Immediate` call-argument source
relationship.

The selected `src/20000412-2.c` row no longer fails in direct-call semantic
admission. It now reaches the object route and fails downstream as
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering`.

## Suggested Next

Inspect the next call-metadata representative boundary. The `src/20000412-2.c`
seed has moved past direct-call producer admission, so Step 2 should either
advance to the next direct-call metadata seed or hand back to plan-owner review
if the remaining Step 2 scope is exhausted.

## Watchouts

Reject downstream RV64/MIR call inference, generic local-memory routing,
runtime/intrinsic repairs, expectation rewrites, unsupported-marker changes,
allowlist edits, runtime-comparison changes, and named-case shortcuts. The
runbook must cover call-return metadata before claiming the source idea is
complete. Treat the current RV64 object-route failure for `src/20000412-2.c` as
downstream unless the supervisor explicitly opens an object-lowering packet.

## Proof

Proof log: `test_after.log`.

Commands:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed `345/345`.
- `ALLOWLIST=build/agent_state/558_step1_20000412.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
  rechecked the selected row. The command exits nonzero because the row still
  fails, but the failure moved from direct-call semantic producer admission to
  downstream RV64 object lowering.
- `git diff --check` passed.
