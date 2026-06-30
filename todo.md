Status: Active
Source Idea Path: ideas/open/447_immediate_global_store_source_encoding.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Immediate Source Encoding Contract

# Current Packet

## Just Finished

Completed Step 2: defined the immediate global-store source encoding contract
for idea 447 and recorded supporting notes under
`build/agent_state/447_step2_immediate_source_contract/`.

Accepted immediate-source contract facts:

- Source value is a `StoreGlobalInst` immediate and the prepared plan preserves
  the exact immediate type plus payload/bit pattern in `source_value`.
- Source producer is explicit:
  `source_producer_kind=PreparedEdgePublicationSourceProducerKind::Immediate`.
  No block/inst producer reference is required for an immediate literal.
- Publication intent is
  `PreparedStoreSourcePublicationIntent::StoreGlobalPublication`.
- Destination access is a semantic global-symbol access with symbol identity,
  offset, width, alignment, base-plus-offset eligibility, layout authority, and
  range proof satisfying `prepared_global_symbol_memory_has_publication_authority`.
- Missing source value, `source=<none>`, `source_producer=unknown`, or
  incoherent destination facts remain fail-closed.

Rejected adjacent shapes:

| shape | disposition |
| --- | --- |
| implicit immediate with `source_producer=unknown` | rejected until producer publishes `Immediate` |
| named-value global sources (`%t4`, `%t10`, `%t1`) | out of scope for this packet; use existing named source-producer/value-home authority |
| local/frame-slot store-source rows | out of scope |
| pointer-value memory and pointer stores | out of scope |
| direct-global return/select-chain rows | out of scope |
| missing/incoherent global layout authority | rejected; layout authority belongs to ideas 446/448 |
| RV64 target-side literal recovery | rejected as target-side inference/overfit |

The bounded candidate remains `20041112-1 main.entry.0`: destination/layout
facts are coherent (`scalar_layout`, `proven_in_bounds`), while the immediate
`i32 1` source is still `source=<none>; source_producer=unknown`.

## Suggested Next

Execute Step 3: implement the first immediate-source publication packet in the
prepared producer path. Suggested owned files/tests:

- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp` only if a small helper/API
  declaration is needed
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` if populated
  store-source metadata coverage is clearer there
- `tests/backend/bir/backend_prepared_printer_test.cpp` if the prepared dump
  contract needs `source_producer=immediate` coverage
- `todo.md`
- `test_after.log`
- `build/agent_state/447_step3_immediate_source_publication/*`

The implementation should set
`PreparedStoreSourcePublicationPlan::source_producer_kind` to `Immediate` for
immediate-valued `StoreGlobalInst` rows only when the destination access is
otherwise coherent, preserve the immediate value/type already held in
`plan.source_value`, and leave named sources plus unknown/incoherent shapes on
the existing fail-closed paths.

Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Keep this plan limited to immediate global-store source encoding.
- Do not reopen scalar or integer-array global layout authority; those are
  closed by ideas 446 and 448.
- Do not infer immediate values or source publication in RV64 from raw BIR
  store shape, testcase names, block labels, symbol names, or one dump layout.
- Keep `source=<none>` and `source_producer=unknown` fail-closed until
  producer facts are explicit.
- Keep non-immediate global stores (`source=%t4`, `%t10`, `%t1`), local stores,
  pointer-value memory, direct-global return/select-chain rows, and target
  lowering outside Step 2.
- Preserve idea 439 store-publication predicates; do not weaken
  `prepared_store_global_publication_has_authority` to accept unknown sources.
- Existing authority logic already accepts explicit `Immediate` and rejects
  implicit immediate; Step 3 should populate the producer fact rather than
  change the target or predicate semantics.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 2 contract-only validation:

```sh
git diff --check
```
