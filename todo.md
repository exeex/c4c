Status: Active
Source Idea Path: ideas/open/438_prepared_pointer_value_memory_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Producer Coverage

# Current Packet

## Just Finished

Completed Step 2: defined the prepared authority contract for pointer-value
memory before selecting any implementation packet.

Minimum prepared facts for a target-consumable pointer-value memory access:

| fact group | required facts |
| --- | --- |
| Pointer ownership | `address.base_kind=PointerValue`, `pointer_value_name` names the dereferenced runtime pointer, and `address.provenance.base_identity.kind` identifies a concrete owner such as local slot, global symbol, formal/byval/sret parameter, string constant, or an explicitly accepted unknown-runtime compatibility owner. Bare `Unknown` or `PointerValue` identity is not enough by itself. |
| Layout authority | `address.provenance.layout_authority` is stronger than `Unknown`: structured layout, scalar layout, byte-storage aggregate, rendered-type fallback from the structured-layout route, or a deliberately accepted opaque-compatibility route. |
| Object extent | bounded-object routes publish known extent: `size_known=true`, enough `size_bytes` for the requested access, and `completeness=Complete` unless the packet is explicitly an opaque compatibility case. |
| Range verdict | `requested_range` matches `address.byte_offset` and `address.size_bytes` without overflow, and `range_verdict=ProvenInBounds`. `UnknownCompatible` remains fail-closed for target consumers unless a later packet defines a precise opaque-compatibility exception. |
| Base-plus-offset validity | `can_use_base_plus_offset=true`, target-representable offset, scalar size/alignment match, and an authoritative home for the pointer value. This is necessary but not sufficient without provenance/layout/range proof. |

Current representative `930930-1` remains rejected under this contract: its
four pointer-value memory records have `layout_authority=unknown` and
`range_verdict=unknown_compatible`, so RV64 must not reconstruct pointee
ownership or range from pointer names, stack slots, or dump shape.

Rejected adjacent shapes:

- `layout_authority=unknown`, even with `can_use_base_plus_offset=true`.
- `range_verdict=unknown_compatible` or `ProvenOutOfBounds`, except for a
  future explicitly tested opaque-compatibility route.
- Missing `pointer_value_name`, missing pointer value home, unsupported address
  space, volatile access, non-representable offset, unsupported width, or
  size/alignment mismatch.
- Global memory publication, direct-global return/select, terminator/select,
  store-source publication, selected object-data emission, and RV64-side
  provenance inference.

Required focused tests for Step 3:

- producer/prepared contract coverage that rejects or classifies
  `layout_authority=unknown` plus `range_verdict=unknown_compatible` as not
  target-consumable;
- a coherent producer fixture, if existing helpers can express it, proving
  concrete base identity, layout authority, object extent, requested range,
  `ProvenInBounds`, and base-plus-offset facts are preserved;
- retention of existing RV64 manually authored pointer-value consumer fixtures
  as low-level consumer tests only, not proof that `930930-1` has producer
  authority.

Disposition: this source idea can proceed directly to a producer/contract
coverage packet. No split is needed yet because the required contract maps to
existing `PreparedAddress::provenance` fields and the producer helpers in
`memory/provenance.cpp`.

Artifacts:

- `build/agent_state/438_step2_pointer_value_authority_contract/contract.md`

## Suggested Next

Execute Step 3: Implement Or Route Producer Coverage.

Recommended packet:

- Add or tighten focused producer/prepared contract tests for pointer-value
  memory authority.
- Preserve fail-closed behavior for unknown layout/range authority and add a
  coherent explicit-authority fixture if the existing BIR/prepared helpers can
  express it.
- Implement only the smallest producer/verifier repair needed by those tests,
  or route a separate producer split if the fixture surface cannot express the
  coherent authority contract.

## Watchouts

- Do not infer pointer-value memory ownership, layout, range, or provenance in
  RV64 from raw pointer values, integer casts, filenames, function names,
  object labels, or dump shape.
- Keep `layout_authority=unknown` and `range_verdict=unknown_compatible`
  fail-closed until producer facts prove stronger authority.
- Do not fold store-source/global-memory publication, direct-global
  return/select-chain, or terminator/select publication work into this plan.
- Do not reopen pointer-cast movement from idea 429 or selected global
  object-data support from idea 433.
- Existing RV64 pointer-value tests are manually-authored consumer fixtures;
  they are not proof that `930930-1` has producer authority.
- Treat `OpaqueCompatibility` as a named producer-owned exception only after
  focused coverage exists; do not let it silently admit arbitrary
  `UnknownCompatible` pointer-value accesses.

## Proof

Step 2 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
