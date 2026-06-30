Status: Active
Source Idea Path: ideas/open/446_global_memory_layout_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Global Layout Authority Contract

# Current Packet

## Just Finished

Completed Step 2: defined the prepared global layout authority contract and
selected the first bounded producer/prepared packet.

Accepted global layout authority requires:

| required fact | contract |
| --- | --- |
| Global identity | Prepared `GlobalSymbol` address with semantic symbol/`LinkNameId` resolution to a `bir::Global`; raw spelling alone is compatibility-only unless the existing resolver proves the object. |
| Object status/addressability | Known global object plus accepted materialization policy and `can_use_base_plus_offset=true`; unresolved/declaration-only or unsupported TLS/GOT policy shapes remain false by default. |
| Complete extent | `object_extent.size_known=true`, `completeness=Complete`, and nonzero `size_bytes` from producer global object facts. |
| Access facts | Prepared offset, width, alignment, address space, nonzero `size_bytes`, nonzero `align_bytes`, and `align_bytes <= size_bytes`. |
| Range proof | Requested range is available, non-overflowed, matches the prepared offset/width, has an available end, and is `ProvenInBounds` against the complete object extent. |
| Layout authority | Non-unknown and non-opaque: accepted predicate kinds are `StructuredLayout`, `ScalarLayout`, `ByteStorageAggregate`, and `RenderedTypeFallback`; the first packet should publish only the narrow kind justified by producer facts. |
| Store publication coupling | Store-global records still need idea 439 source authority: explicit producer/value home for named values or explicit immediate-source encoding for immediates. |

Current representative disposition:

| bucket | rows | disposition |
| --- | --- | --- |
| Candidate for first packet | `20041112-1` load/store rows for `@global+0`, width 4, align 4, range `proven_in_bounds` | Coherent simple scalar global candidate once producer publishes complete extent and `ScalarLayout` authority. Not currently accepted because `layout_authority=unknown`. |
| Missing/incoherent authority | `930930-1` store to `@mem+792`, width 8, align 8, range `proven_in_bounds` | Still lacks explicit layout authority and printed extent/completeness; keep fail-closed until a broader array/aggregate-global packet proves source facts. |
| Out of scope | `20000622-1` local/frame-slot row; `20041112-1` direct-global return/address-only row; pointer-value memory | Not global layout-authority publication targets. |
| Separate idea | immediate global-store source encoding | Belongs to `ideas/open/447_immediate_global_store_source_encoding.md`, not this plan. |

Artifacts:

- `build/agent_state/446_step2_global_layout_authority_contract/contract.md`

## Suggested Next

Execute Step 3: Implement Or Route First Layout Authority Packet.

Selected packet: `Publish Scalar Global Layout Authority`.

Use existing global producer facts from `bir::Global`/global lowering and the
prepared global resolver to publish complete object extent plus `ScalarLayout`
for simple scalar global objects only when the global has nonzero size/alignment,
is addressable, resolves semantically, and the access range is proven in bounds.
Add focused producer/prepared tests for accepted scalar `@global+0` load/store
authority and fail-closed missing symbol, unresolved/raw-only symbol, unknown
layout, missing extent, out-of-range, pointer-value memory, address-only/direct
global return, and immediate-source encoding boundaries. If current producer
facts cannot distinguish simple scalar globals from arrays/aggregates or
declaration-only objects, stop and report that metadata gap instead of editing
RV64 lowering.

Proposed owned files for Step 3:

- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/446_step3_scalar_global_layout_authority/*`

## Watchouts

- Keep this plan limited to global layout authority publication.
- Do not fold immediate store-source encoding into this plan; that belongs to
  `ideas/open/447_immediate_global_store_source_encoding.md`.
- Do not infer global layout, object extent, addressability, offset meaning,
  or symbol identity in RV64 from raw BIR, symbol spelling, object labels,
  representative filenames, or dump shape.
- Keep `layout_authority=unknown` fail-closed until producer facts are
  explicit.
- Do not accept `OpaqueCompatibility` as publication authority for this plan.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 2 contract-only check:

```sh
git diff --check
```
