# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.3
Current Step Title: Receive the checked ffs Cttz-to-Add-one result

## Just Finished

- Step 7.3 complete: received only the native i32 `Cttz` result followed by
  its immediate-one i32 Add. The importer, Raw-BIR builder, and verifier keep
  the ordered source identity and require the Add lhs to be that admitted i32
  Cttz result; zero comparison and Select remain transactional failures.

## Suggested Next

- Supervisor-select the next bounded receiver row; do not fold the still-
  unsupported ffs zero comparison or Select into this completed Add-one slice.

## Watchouts

- Cttz Add-one is one narrow i32 provenance family: missing, invalid,
  duplicate, cross-owner, wrong-provenance, wrong-opcode/type/immediate, and
  malformed-linkage forms fail closed. Other intrinsic arithmetic, operands,
  immediates, widths, and the following comparison/Select have not been
  received.

## Proof

- Step 7.3 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`.
