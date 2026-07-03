Status: Active
Source Idea Path: ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Structured Unsupported-Instruction Context

# Current Packet

## Just Finished

Step 2 added a diagnostic-only generic fallback formatter for failed
`fragment_for_prepared_instruction` lowering in
`prepared_function_to_object_function`.

- The complete prepared traversal path and fallback block/instruction loop now
  keep the `unsupported_instruction_fragment` category prefix while appending
  function, block label, block index, instruction index, BIR instruction kind,
  and owner/value context.
- Focused backend object-emission tests now assert representative structured
  context for both the fallback block loop and prepared traversal loop without
  relying on whole-module rendered-text probes.
- No unsupported RV64 lowering behavior, unsupported markers, allowlists, or
  runtime comparison files were changed.

## Suggested Next

Run Step 3 against the nine representative unsupported RV64 cases and capture
the resulting enriched diagnostics for follow-up routing.

## Watchouts

- This plan is diagnostic-only; do not implement unsupported RV64 lowering
  operations discovered by the improved diagnostics.
- Do not add named-case matching for the nine representative source files.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- The atomic-operation admission rejection is outside the instruction-fragment
  fallback and already has a distinct owner (`function->atomic_operations`);
  changing it should be a separate decision, not part of the first generic
  fragment fallback patch.
- The null prepared traversal instruction fallback has no BIR instruction value
  to inspect; surfacing more context there likely requires threading event or
  block metadata separately.
- `prepared_consumer_category` diagnostics bypass the plain string rejection
  path and should not be conflated with `unsupported_instruction_fragment`.

## Proof

Proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$') > test_after.log 2>&1`

Result: passed.
Log path: `test_after.log`.
