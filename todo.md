# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Complete functions, signatures, CFG and local objects

## Just Finished

- Plan Step 4's first bounded packet now imports producer-valid direct integer,
  enum-normalized, and floating return signatures through the existing typed
  `FunctionSignature` return slot.
- Optional typed mirrors reconcile exactly, producer `inner_rank == -1` and
  compatibility `0` normalize locally, target-shaped `long`/`long double`
  carriers survive Raw/Foundation/Canonical publication, and malformed or
  residual declarator facts reject transactionally.

## Suggested Next

- Add the next bounded Plan Step 4 ordinary-instruction receipt packet centered
  on the shared `UnsupportedOrdinaryInstruction function='main' block='entry'`
  boundary from three real producer probes.

## Watchouts

- `riscv64_zero_aggregate_global_storage.c` now reaches `InvalidVoidReturn`;
  non-void `LirRet` still has only text identity and needs a separately scoped
  producer-authority decision rather than signature-text parsing.
- Pointers/references/function pointers, aggregate/vector/complex/VRM/va-list
  returns, parameters, variadics, stack objects, and hoisted allocas remain
  closed. Scalar mirrors are optional but exact when present.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
- `global_store.c`, `defined_pointer_global_pointer.c`, and
  `defined_global_array.c` now stop at `UnsupportedOrdinaryInstruction`;
  `riscv64_zero_aggregate_global_storage.c` stops at `InvalidVoidReturn`.
