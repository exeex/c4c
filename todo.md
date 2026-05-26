Status: Active
Source Idea Path: ideas/open/31_riscv_prepared_edge_publication_stack_source_policy_followup.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate RISC-V Backend Behavior

# Current Packet

## Just Finished

Completed Step 4 - Validate RISC-V Backend Behavior.

Inspected the committed implementation slice against the source idea reviewer
reject signals and reran the supervisor-selected backend proof.

- No fixture-name, value-id, stack-slot-id, offset, or test-name special-casing
  was found in the implementation path. The implementation accepts 4-byte and
  8-byte `StackSlot -> Register` homes generically and only branches on whether
  the prepared stack offset fits the signed 12-bit load immediate.
- No predecessor/successor block scan or RISC-V-local edge-copy fact table was
  added. The path still starts from
  `find_unique_indexed_prepared_edge_publication`; clearing
  `publications_by_edge_destination` still yields `MissingPublication`.
- No expectation downgrade was found. The test change replaced the old
  large-offset I64 rejection with the selected large-offset policy, while
  keeping large-offset subword sources fail-closed.
- Existing 4-byte `lw` and 8-byte `ld` concrete-offset support remains covered
  by the focused stack-source tests and the backend proof.
- Unsupported neighboring forms still fail closed: missing offset/size,
  subword widths, aggregate widths, non-move publications, pointer-base
  broadening outside this plan, and source-to-stack destination large offsets.

## Suggested Next

Proceed to Step 5 close-or-follow-up decision. The current large-offset
`StackSlot -> Register` slice is validation-clean; remaining candidate forms
still need separate policy authority before implementation.

## Watchouts

- `PreparedValueHome` currently records stack offset/size but not signedness,
  scalar type, or register bank, so sub-word, unsigned 32-bit, and F32 policies
  need additional authority before implementation.
- The large-offset stack-source helper reserves `t6` as target-local address
  scratch for one edge-publication sequence; no broader scratch allocator or
  register-liveness model was introduced in this packet.
- Source-to-stack destination large offsets intentionally remain unsupported;
  this plan only covers concrete `StackSlot -> Register` loads.

## Proof

Proof command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_publication_plan_record|backend_)' > test_after.log 2>&1`

Result: passed. `test_after.log` records 163 matching backend tests passed,
0 failed.
