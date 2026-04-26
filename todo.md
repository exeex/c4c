# Current Packet

Status: Active
Source Idea Path: ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Assembler Handoff And Document Retained Boundaries

## Just Finished

`plan.md` Step 4 prepared-consumer cleanup landed through the liveness,
stack/call-frame, dynamic-stack, out-of-SSA, and regalloc consumer packets.

Completed work:

- `src/backend/prealloc/out_of_ssa.cpp`: prepared out-of-SSA phi/materialized
  join handling prefers id-backed prepared block names before raw block spelling.
- `src/backend/prealloc/prealloc.cpp`: stack/call-frame and dynamic-stack
  prepared consumers prefer id-backed prepared block lookup while retaining raw
  spelling as a narrow invalid/unresolved-id compatibility fallback.
- `src/backend/prealloc/regalloc.cpp`: select-materialized join move resolution
  resolves the current block from the existing BIR `BlockLabelId` through
  prepared names first, and falls back to raw block spelling only when the id is
  invalid or unresolved.
- Focused backend tests for phi materialization, authoritative join ownership,
  structured context, prepared printing, frame/stack call contracts, dynamic
  stack restore CLI coverage, and prepared liveness passed in the executor
  packets.

## Suggested Next

Step 5 handoff proof is currently blocked in this checkout. Enabling
`C4C_ENABLE_X86_BACKEND_TESTS=ON` makes `backend_x86_handoff_boundary_test`
compile against x86 handoff headers that are not present here:

- `src/backend/mir/x86/codegen/api/x86_codegen_api.hpp`
- `src/backend/mir/x86/codegen/abi/x86_target_abi.hpp`

The normal backend build configuration has been restored with
`C4C_ENABLE_X86_BACKEND_TESTS=OFF`. Next action belongs to the supervisor:
either restore/provide the missing x86 handoff test infrastructure for the
Step 5 proof, or delegate a non-x86 assembler/backend handoff proof path that
still satisfies the runbook's id-consumption requirement.

## Watchouts

- Preserve BIR dump spelling unless a contract change is explicitly approved.
- Treat invalid or unresolved `BlockLabelId` values as proof gaps, not as a
  reason to add broader raw string matching.
- Do not expand into MIR migration unless the supervisor opens a separate
  initiative.
- Avoid testcase-overfit cleanup that only makes one known case pass.
- Do not remove raw label spelling fields yet. They are still the compatibility
  payload for printer output, unresolved-id fallback, and existing downstream
  code not covered by this packet.
- `clang-format` is not installed in this container, so this packet was kept
  manually formatted.
- Do not treat the unavailable `backend_x86_handoff_boundary_test` as proof
  that assembler handoff is complete. It is an infrastructure blocker for
  Step 5, not a completed proof.

## Proof

Step 4 focused proofs passed in executor packets.

Latest Step 4 proof command:
`( cmake -S . -B build-backend -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON && cmake --build build-backend --target backend_prepare_liveness_test && ctest --test-dir build-backend -j --output-on-failure -R 'backend_prepare_liveness' ) > test_after.log 2>&1`

Proof log: `test_after.log`

Step 5 proof status: blocked before compile because the optional x86 handoff
test target depends on absent x86 codegen headers in this checkout. No
assembler/x86 handoff acceptance proof is recorded yet.
