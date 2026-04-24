Status: Active
Source Idea Path: ideas/open/03_bir-memory-coordinator-dispatch-split.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Split Scalar-Only Cases

# Current Packet

## Just Finished

Completed `Step 1: Split Scalar-Only Cases` by moving scalar compare/cast/binop
lowering into the private scalar-family handler in `scalar.cpp`. The memory
coordinator now keeps only pointer-address cast handling and pointer-address
subtraction before delegating generic scalar work.

## Suggested Next

Execute `Step 2: Split Alloca And Local-Slot Handling` from `plan.md`.

## Watchouts

- Do not create new `.hpp` files.
- Keep `lowering.hpp` as the complete private `BirFunctionLowerer` index.
- Keep memory state ownership on `BirFunctionLowerer`.
- Split by real instruction-family boundaries, not one file per narrow case.
- Preserve behavior and diagnostics; do not rewrite expectations as proof.
- `lower_scalar_or_local_memory_inst` still owns the address-int/provenance cast
  branches; those were intentionally left for the later address-int or memory
  families.

## Proof

`cmake --build --preset default --target c4c_backend` passed; proof log:
`test_after.log`. `git diff --check` passed.
