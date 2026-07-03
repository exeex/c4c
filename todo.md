# Current Packet

Status: Active
Source Idea Path: ideas/open/568_rv64_pointer_result_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Prepared Pointer-Result Materialization

## Just Finished

Completed Step 3 (`Implement Prepared Pointer-Result Materialization`) by
teaching RV64 prepared object emission to lower pointer-result frame-slot address
adds without testcase-shaped matching.

- Added an encoded-object helper that claims `bir.add ptr` when the current
  block/instruction has exactly one frame-slot address-materialization fact for
  the pointer LHS/base value.
- The helper validates default-address-space, non-TLS, non-negative frame-slot
  facts, requires a GPR home for the pointer base, materializes the integer byte
  offset from a prepared register, stack-slot, or coherent rematerializable
  home, adds it to the base GPR, and publishes the pointer result to its
  prepared GPR or frame-slot destination.
- The Step 2 positive coverage now builds and verifies the stack load, dynamic
  pointer add, and frame-slot result publication. The malformed fact/home cases
  still reject through the unsupported lowering path.

## Suggested Next

Execute Step 4 representative-progress validation: run the supervisor-selected
representative source or runtime case that originally motivated the pointer
result frame-slot address materialization repair, and decide whether this
source idea now has enough coverage to close or needs another focused packet.

## Watchouts

- The new object helper intentionally matches the prepared address fact against
  the pointer LHS/base value, not the `bir.add ptr` result. Keep that boundary
  intact when adding representative coverage.
- The helper currently supports the semantic Step 3 shape for `Add` with a
  pointer LHS and integer RHS. Do not widen into unrelated pointer arithmetic or
  expectation rewrites without a new packet.
- Do not match `src/20001026-1.c`, old pinned temporary names, raw instruction
  text, or diagnostic strings in follow-up work.
- Leave `review/557_step13_vector_local_memory_review.md` untouched.

## Proof

Proof command run:

`set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed. Build completed, `backend_riscv_object_emission` passed, and
all `^backend_` tests passed (`345/345`).

Proof log: `test_after.log`.
