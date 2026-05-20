Status: Active
Source Idea Path: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Selected Address And Writeback Propagation

# Current Packet

## Just Finished

Step 3 accepted slice summary for idea 348 indexed aggregate selected-address
and writeback propagation.

Focused coverage now exists for pointer-value split-offset memory operands:
one store where the selected pointer address contributes offset 24 and the
store/member contributes offset 4, and one load where the selected pointer
address contributes offset 18 and the load/member contributes offset 6. These
prove that the prepared AArch64 memory layer preserves selected-address plus
member-offset semantics instead of accepting only a single precombined offset.

Implementation completed in the accepted slice:

- Pointer-indirect prepared address construction combines the selected
  `MemoryAddress::PointerValue` offset with the load/store instruction byte
  offset.
- Fixed aggregate slice families with sparse byte-offset-style suffixes keep
  those semantic offsets in stack-slot assignment instead of being packed by
  home-slot order. This repairs the local scalarized aggregate case where a
  direct leaf store and pointer-derived read disagreed on the same byte.
- Prepared memory validation accepts both structured offset encodings currently
  produced by the backend: `MemoryAddress.byte_offset` already selected as the
  authoritative offset, and `MemoryAddress.byte_offset + instruction offset`
  for selected-plus-member offsets. This fixed the
  `backend_aarch64_instruction_dispatch` regression without weakening the new
  pointer-value split-offset coverage.

External representative status improved: `00130` and `00187` now pass. The
remaining red representatives are unchanged and are outside the accepted
repair boundary for this slice: `00176`, `00181`, `00182`, and `00195`.

Supervisor validation recorded for acceptance:

- Regression guard PASS: delegated subset improved from 3/9 passing to 5/9
  passing, with no new failures.
- Broader backend validation PASS: `ctest --test-dir build -j
  --output-on-failure -R '^backend_'` passed 141/141.

## Suggested Next

Decide whether the remaining scalar-select/store-publication boundary behind
`00176`, `00181`, `00182`, and `00195` belongs in idea 348 Step 3 or needs a
lifecycle split into a separate focused initiative.

## Watchouts

- Keep implementation scoped to active focused idea 348, not the parked
  umbrella inventory idea.
- Do not special-case c-testsuite filenames, source symbols, byte offsets,
  temporary registers, or emitted instruction sequences.
- The remaining red external tests in `test_after.log` are the same known
  non-regression set after this packet: `00176`, `00181`, `00182`, and `00195`.
  `00130` and `00187` still pass.
- The validation rule intentionally preserves both current structured offset
  encodings: already-selected offset and selected-plus-member offset. The new
  pointer-value split-offset tests still prove the combined-offset case.

## Proof

Executor proof was previously run exactly into `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R 'backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract)|c_testsuite_aarch64_backend_src_001(30|76|81|82|87|95)_c' | tee test_after.log
```

Supervisor already ran regression guard and broader backend validation after
that proof. No command was required for this todo-only tightening packet.
