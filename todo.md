Status: Active
Source Idea Path: ideas/open/27_riscv_prepared_edge_publication_stack_destination_support.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Handoff or Close

# Current Packet

## Just Finished

Completed Step 5 handoff/closure decision for focused RISC-V
`Register -> StackSlot` stack-destination edge-publication consumption.

Supported behavior is exactly:

- the publication is available through `find_unique_indexed_prepared_edge_publication`
- the source home is a concrete register
- the destination home is `StackSlot` with concrete `offset_bytes`
- the destination stack slot has `size_bytes == 4`

The target-local emission is `sw <src>, <offset>(sp)` through shared
`edge_publications` authority. Existing register-destination consumers for
register, immediate, stack-source, and pointer-base source homes remain
supported.

Validation and review evidence is sufficient for closure:

- focused RISC-V prepared edge-publication subset: PASS, 5/5 selected tests
- matching focused regression guard: PASS, 5/5 before and 5/5 after, no new failures
- broader backend bucket: PASS, 163/163 backend tests
- accepted full-suite baseline: PASS, 3411/3411 tests
- reviewer report `review/idea27_riscv_stack_destination_edge_publication_review.md`:
  no blocking findings

## Suggested Next

Recommend closing idea 27 if the supervisor accepts the scoped support and
evidence above. Follow-up stack-destination source families should remain
separate packets or ideas.

## Watchouts

The new stack-destination support is intentionally narrow: only
`Register -> StackSlot` with concrete 4-byte destination facts is accepted.
`RematerializableImmediate -> StackSlot`, `StackSlot -> StackSlot`,
`PointerBasePlusOffset -> StackSlot`, malformed stack destinations, non-move
publications, and local rediscovery remain fail-closed. The implementation
continues to rely on shared `edge_publications` authority and does not scan
predecessor/successor block shape.

## Proof

Proof mode: `docs/handoff-only`.

No tests were run for this handoff note, and no new `test_after.log` was
created.

Previously accepted focused proof command:

```text
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1
```

Result: PASS, 5/5 selected tests passed.

Supervisor ran the matching regression guard against the focused
`test_before.log` and focused `test_after.log` with non-decreasing pass count
allowed because this packet extended an existing CTest binary. Result: PASS
with 5/5 before and 5/5 after, no new failures.

Supervisor then ran broader backend validation:

```text
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1
```

Result: PASS, 163/163 backend tests passed. This backend run is broader
validation only, not the matching before/after regression comparison for the
focused packet.
