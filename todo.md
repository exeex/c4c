Status: Active
Source Idea Path: ideas/open/31_riscv_prepared_edge_publication_stack_source_policy_followup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Add Focused Positive And Negative Coverage

# Current Packet

## Just Finished

Completed Step 3 - Add Focused Positive And Negative Coverage.

Reviewed the focused large-offset RISC-V `StackSlot -> Register`
edge-publication coverage in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`; no
additional test edits were needed.

- Positive large-offset I32 coverage exists in
  `check_large_offset_stack_slot_to_register_loads_use_shared_lookup` with
  `li t6, 4096`, `add t6, sp, t6`, and `lw a1, 0(t6)`, plus publication
  provenance checks.
- Positive large-offset I64 coverage exists in the same helper with
  `li t6, 8192`, `add t6, sp, t6`, and `ld a1, 0(t6)`, plus stack provenance
  checks.
- Missing-publication negative coverage exists on the large-offset helper by
  clearing `publications_by_edge_destination` and requiring
  `MissingPublication`.
- Unsupported neighboring-form coverage exists in
  `check_stack_source_fail_closed_forms`, including large-offset subword stack
  sources, missing offset/size, aggregate-width sources, and non-move
  publications. Existing stack-destination fail-closed coverage also preserves
  the non-goal for source-to-stack destination large offsets.

## Suggested Next

Proceed to Step 4 validation/review of the RISC-V backend behavior using the
supervisor-selected backend proof and diff inspection.

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
- No separate I64 missing-publication negative was added because the shared
  publication lookup is width-independent and already covered on the
  large-offset path; positive I64 coverage proves the width-specific load
  rendering.
- Source-to-stack destination large offsets intentionally remain unsupported;
  this plan only covers concrete `StackSlot -> Register` loads.

## Proof

Proof command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_publication_plan_record|backend_)' > test_after.log 2>&1`

Result: passed. `test_after.log` records 163 matching backend tests passed,
0 failed.
