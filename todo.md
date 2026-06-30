Status: Active
Source Idea Path: ideas/open/448_array_aggregate_global_layout_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Array/Aggregate Layout Authority Contract

# Current Packet

## Just Finished

Completed Step 2: defined the array/aggregate global layout authority contract
and selected the first bounded implementation packet.

Contract summary:

| scope | decision |
| --- | --- |
| Narrow accepted class | Integer-array globals only: semantic `GlobalSymbol` identity, known definition, non-TLS/non-string, producer-owned element stride, element count, complete storage size, linear addressability, complete object extent, valid access width/alignment, and `ProvenInBounds` requested range. |
| Representative fit | `930930-1 @mem+792` is a coherent integer-array candidate: source declares `ptr_t mem[100]`, prepared access is offset 792, width 8, align 8, and range is proven in bounds. It is not accepted yet because durable integer-array layout authority is not published. |
| Authority kind | Use existing non-scalar predicate-compatible authority, likely `ByteStorageAggregate`, unless Step 3 adds a more precise enum. Do not use `ScalarLayout` for this plan. |
| Broader aggregates | Not selected for Step 3. Structured aggregate support needs semantic structured type identity, complete field layout, field/byte-lane offset ownership, padding/packing handling, and durable prepared facts. |
| Rejected shapes | Unknown or opaque authority, scalar globals, raw-only unresolved globals, extern/declaration-only globals, TLS globals, strings, pointer arrays without explicit layout facts, rendered-text-only aggregates, missing extent, out-of-range access, pointer-value/frame-slot/direct-global return rows, and immediate-source encoding. |

Artifacts:

- `build/agent_state/448_step2_non_scalar_layout_contract/contract.md`

## Suggested Next

Execute Step 3: Implement Or Route First Non-Scalar Authority Packet.

Selected packet: `Publish Integer-Array Global Layout Authority`.

Proposed implementation:

- add durable integer-array layout metadata to `bir::Global` or an equivalent
  prepared-accessible producer carrier: authority bit, element size/stride,
  element count, and complete storage size;
- populate it only from the integer-array lowering branch in
  `src/backend/bir/lir_to_bir/globals.cpp`;
- publish non-scalar layout authority in
  `src/backend/prealloc/stack_layout/coordinator.cpp` only for semantic
  in-bounds integer-array global accesses compatible with a single element;
- add focused BIR/prepared tests for accepted integer-array access and
  fail-closed scalar, aggregate, string, extern/TLS, raw-only unresolved,
  missing-extent, out-of-range, incompatible offset/width, pointer-value, and
  direct-global/address-only boundaries.

Proposed owned files:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/globals.cpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/448_step3_integer_array_global_layout_authority/*`

## Watchouts

- Keep this plan limited to array/aggregate global layout authority.
- Do not infer non-scalar layout authority in RV64 from raw BIR, symbol names,
  object labels, representative filenames, offsets, or dump shape.
- Do not broaden Step 3 into structured aggregate layout; split if aggregate
  support is needed.
- Do not claim scalar `layout_authority=scalar_layout` rows as progress for
  this non-scalar plan.
- Keep immediate global-store source encoding with
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Keep direct-global return/select-chain, pointer-value memory,
  terminator/select publication, and local/frame-slot residuals outside this
  idea.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 2 contract-only check:

```sh
git diff --check
```
