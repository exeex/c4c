Status: Active
Source Idea Path: ideas/open/461_rv64_move_bundle_coordinate_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Coordinate Diagnostic Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 461: defined the coordinate diagnostic contract under
`build/agent_state/461_step2_coordinate_diagnostic_contract/contract.md`.

Accepted diagnostic shape:

| Field | Requirement |
| --- | --- |
| Event and phase | Print traversal event kind plus prepared move-bundle phase. |
| Function/block/instruction coordinate | Print function identity, block index, prepared block label when available, and instruction index or explicit non-applicable state. |
| Move identity | Print move count and move source/destination value ids, destination kind/storage, and prepared move reason. |
| Parallel-copy context | Print predecessor/successor or source parallel-copy identity when present. |
| Authority status | Print whether explicit select-edge suppression authority matched and whether cast-dependency stack-publication authority matched. |
| Fragment status | State that the bundle reached generic RV64 move-bundle fragment materialization and failed; refine to move-level guard reason later only if needed. |

Rejected shapes: broad `unsupported_move_bundle_target_shape` alone,
testcase/source-name diagnostics, raw BIR text, value-id-only ownership
classification, and any diagnostic that changes lowering or consumes stale
`load_from_stack_slot missing_stack_freshness`.

## Suggested Next

Execute Step 3: add event-site coordinate diagnostics for generic RV64 prepared
move-bundle fragment failure. Owned files should be
`src/backend/mir/riscv/codegen/object_emission.cpp`,
`tests/backend/mir/backend_riscv_object_emission_test.cpp`, `todo.md`,
`test_after.log`, and
`build/agent_state/461_step3_coordinate_diagnostic_packet/**`.

Exact Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- This is diagnostics/probe support, not semantic lowering.
- Do not add generic stack-to-register or register-to-register move support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen ideas 456, 458, or 459 without coordinate-bearing evidence.
- Do not infer ownership from value ids, block indexes, instruction indexes,
  raw BIR shape, filenames, function names, or one prepared dump alone.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Step 2 proof:

```sh
git diff --check
```

Result: passed.
