Status: Active
Source Idea Path: ideas/open/27_riscv_prepared_edge_publication_stack_destination_support.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate the Stack Destination Slice

# Current Packet

## Just Finished

Completed Step 2 and Step 3 preparation for focused RISC-V
`Register -> StackSlot` edge-publication consumption.

The RISC-V prepared edge-publication helper now accepts a shared lookup-backed
move when:

- the publication is available through `find_unique_indexed_prepared_edge_publication`
- the source home is a concrete register
- the destination home is `StackSlot` with concrete `offset_bytes`
- the destination stack slot has `size_bytes == 4`

The target-local emission is `sw <src>, <offset>(sp)`. Existing
register-destination consumers for register, immediate, stack-source, and
pointer-base source homes remain supported.

Focused tests now cover:

- positive shared-authority `Register -> StackSlot` emission
- local rediscovery prevention after the shared edge-publication index is
  removed
- malformed stack destinations without offset or with non-4-byte size
- unsupported stack-destination source homes: immediate, stack-source, and
  pointer-base
- non-move publications remaining fail closed

## Suggested Next

Request Step 3 reviewer scrutiny, then run Step 4 validation if review reports
no blocking route-quality findings.

## Watchouts

The new stack-destination support is intentionally narrow: only
`Register -> StackSlot` with concrete 4-byte destination facts is accepted.
`RematerializableImmediate -> StackSlot`, `StackSlot -> StackSlot`, and
`PointerBasePlusOffset -> StackSlot` remain fail-closed. The implementation
continues to rely on shared `edge_publications` authority and does not scan
predecessor/successor block shape.

## Proof

Delegated proof command:

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
