Status: Active
Source Idea Path: ideas/open/31_riscv_prepared_edge_publication_stack_source_policy_followup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define And Implement The Selected Target-Local Policy

# Current Packet

## Just Finished

Completed Step 2 - Define And Implement The Selected Target-Local Policy.

Implemented large-offset concrete RISC-V `StackSlot -> Register`
edge-publication loads in `src/backend/mir/riscv/codegen/emit.cpp`:

- Shared `edge_publications` lookup remains the only semantic authority:
  emission still starts from `find_unique_indexed_prepared_edge_publication`
  and an available prepared `Move`.
- Stack-source acceptance now records concrete 4-byte and 8-byte stack sources
  without rejecting large offsets in the source-home phase.
- Register-destination emission keeps signed-12-bit offsets on direct
  `lw <dst>, <offset>(sp)` and `ld <dst>, <offset>(sp)`.
- Large concrete offsets now emit target-local address materialization through
  `t6`: `li t6, <offset>`, `add t6, sp, t6`, then `lw` or `ld` from `0(t6)`.
- Unsupported neighboring forms remain fail-closed: missing offset/size,
  subword width, aggregate width, non-move publications, pointer-base
  stack-destinations, and stack-destination large offsets are not broadened.

Added focused coverage in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` for
large-offset size-4 and size-8 stack-source publications, including shared
publication authority preservation and the no-publication fail-closed check.

## Suggested Next

Proceed to Step 3/4 review by checking whether the focused large-offset
coverage is sufficient for close-readiness, or add any supervisor-requested
negative coverage for scratch/register alias policy before lifecycle review.

## Watchouts

- Shared prepared `edge_publications` must remain the only semantic authority for edge moves.
- Do not broaden pointer-base register-destination or source-to-stack destination policy in this plan.
- Preserve existing concrete-offset 4-byte `lw` and 8-byte `ld` support.
- Treat fixture-name matching, expectation-only changes, and predecessor/successor block scans as route failures.
- `PreparedValueHome` currently records stack offset/size but not signedness,
  scalar type, or register bank, so sub-word, unsigned 32-bit, and F32 policies
  need additional authority before implementation.
- The large-offset stack-source helper reserves `t6` as target-local address
  scratch for one edge-publication sequence; no broader scratch allocator or
  register-liveness model was introduced in this packet.
- Source-to-stack destination large offsets intentionally remain unsupported;
  this packet only covers concrete `StackSlot -> Register` loads.

## Proof

Proof command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_publication_plan_record|backend_)' > test_after.log 2>&1`

Result: passed. `test_after.log` records 163 matching backend tests passed,
0 failed.
