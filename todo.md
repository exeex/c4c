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

Lifecycle decision: continue inside idea 348 Step 3. Do not split or switch
owners yet.

Rationale: the remaining red representatives still match the active idea's
indexed aggregate selected-address/writeback scope, based on the existing
first-bad-fact evidence from the Step 1 localization and the current
`test_after.log`:

- `00176` still shows global indexed swap output mostly unchanged, consistent
  with dynamic `array[index]` stores selecting values but not publishing them
  through the selected element addresses.
- `00181` still segfaults in pointer-parameter/global tower mutation, matching
  the recorded pointer-backed indexed aggregate scan/store boundary.
- `00182` still loses dynamic buffer writes, matching indexed buffer element
  publication through a selected slot.
- `00195` still prints zeroed struct fields, matching indexed struct-member
  stores where the selected element/member value is not published.

The accepted slice repaired one sub-boundary: selected pointer/local offsets
are now preserved well enough for `00130` and `00187` to pass. The remaining
boundary is narrower but not a separate durable initiative yet: scalar
selection or temporary publication appears to be the next handoff in the same
dynamic selected aggregate operation. Keep the next executor packet scoped to
that handoff in the shared indexed aggregate path, and only request a split if
fresh evidence proves the remaining failures are scalar publication for
non-address-exposed locals, scalar casts, direct calls, returns, local
conversions, or another owner explicitly excluded by idea 348.

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
