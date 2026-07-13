# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Complete functions, signatures, CFG and local objects

## Just Finished

- Plan Step 3 is complete: globals, string-pool state, extern declarations and
  indexes, symbols/link identities, initializer/link topology, specialization
  metadata, and intrinsic requirement flags have typed receiving and verifier
  coverage.
- Real producer probes now clear module/global receipt and first stop at
  `UnsupportedReturnType function='main'`, the explicit Plan Step 4 boundary.

## Suggested Next

- Add one bounded Plan Step 4 packet for producer-valid typed function return
  and signature receipt, centered on the current `UnsupportedReturnType`
  boundary, with neighboring positive and malformed/parity coverage.

## Watchouts

- Keep the first Step 4 packet limited to return/signature receipt; parameters,
  arbitrary block order, CFG edges, stack objects, and hoisted allocas remain
  later bounded Step 4 families.
- Preserve typed LIR authority and module-transactional failure; do not infer
  signatures from names, rendered text, or testcase identity.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
- Producer probes `global_store.c`, `defined_pointer_global_pointer.c`,
  `defined_global_array.c`, and `riscv64_zero_aggregate_global_storage.c` all
  clear module/global receipt and stop at the Step 4 return-type boundary.
