# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now imports the ten existing typed `LirModule` intrinsic
  requirement flags into one exact new-BIR module record.
- All-false, each independent bit, and mixed combinations survive builder/view,
  Foundation, Raw BIR, and Canonical BIR without inferred implications.

## Suggested Next

- Audit the remaining producer-emitted global `TypeSpec` families and make the
  Step 3 checkpoint decision if no additional coherent family remains.

## Watchouts

- Intrinsic requirements remain validation-only module facts; this packet does
  not infer declarations, synthesize operations, or merge independent flags.
- `ModuleBuilder::set_intrinsic_requirements` is set-once when used; an omitted
  builder assignment retains the exact all-false default.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
