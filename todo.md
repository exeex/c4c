Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Consume Prepared Typed Stack And Store-Source Publication Facts

# Current Packet

## Just Finished

Started Step 4 by mapping the typed stack-source and store-source publication
decisions visible in `dispatch_publication.cpp` to existing prepared facts.

| `dispatch_publication.cpp` decision | Existing prepared fact or plan | Current use | Migration note |
| --- | --- | --- | --- |
| `lower_local_slot_address_publication` / `emit_local_slot_address_publication_to_register_impl` computes a local frame-slot address for a pointer add/sub publication. | `PreparedAddressMaterializationLookups` via `prepared_frame_address_offset_for_value`. | Already consumes prepared address-materialization facts for the source offset; target-local register selection, add/sub offset arithmetic, and store-back emission remain local. | Not a typed stack-source publication consumer; no Step 4 migration target here unless the source idea is widened into address-materialization cleanup. |
| `lower_missing_fused_compare_operand_publication` falls back through `plan_prepared_scalar_publication` and `emit_prepared_value_home_publication_to_register` when a needed operand lives in a stack slot. | `PreparedScalarPublicationPlan` plus `PreparedValueHome`; not `PreparedTypedStackSourcePublication`. | Consumes scalar publication/home facts, then emits target-local stack-slot load syntax. | Possible narrow follow-up only if a matching `PreparedEdgePublication` is available at this call site; today this helper has producer/value-home facts but not the typed stack-source publication fact required by `prepare_same_width_i32_stack_source_publication`. |
| `plan_fixed_formal_store_local_publication` gathers store-local source and destination inputs, then calls `plan_prepared_fixed_formal_store_source_publication`. | `PreparedStoreSourcePublicationPlan`, `PreparedFixedFormalStoreSourcePublication`, and `prepared_store_source_publication_available`. | Already consumes prepared store-source publication authority before `lower_fixed_formal_store_local_publication` emits the AArch64 fixed-formal `str*`. | No first migration target in this helper; residue is target-local mnemonic/register/address conversion. |
| `lower_fixed_formal_store_local_publication` checks prepared destination frame-slot/base-plus-offset facts and uses `store_source.source_value_name`, `destination_stack_offset_bytes`, and `destination_byte_offset`. | The `store_source` member returned by `PreparedFixedFormalStoreSourcePublication`. | Already depends on the prepared plan for source identity and destination memory authority. | Keep as store-source plan reuse; broader store-source publication helpers live outside `dispatch_publication.cpp`. |
| No `dispatch_publication.cpp` call to `prepare_same_width_i32_stack_source_publication` or direct `PreparedTypedStackSourcePublication` use exists. | Shared fact exists in `prepared_lookups.*`; AArch64 has no local consumer in this file. | Not consumed in `dispatch_publication.cpp`. | Blocker for an in-file typed stack-source migration: the only typed fact constructor takes a `PreparedEdgePublication*`, but the visible `dispatch_publication.cpp` candidates do not currently carry that publication fact. |

First narrow migration target/blocker: there is no concrete typed stack-source
publication migration inside `dispatch_publication.cpp` without first threading
or locating the relevant `PreparedEdgePublication` at the AArch64 publication
emission site. The first actionable target is therefore to audit the edge
publication emission owner that already has `PreparedEdgePublication` in hand
and decide whether an AArch64 same-width I32 stack-source load should consume
`prepare_same_width_i32_stack_source_publication` there, mirroring the existing
shared fact contract. Within `dispatch_publication.cpp`, the store-source path
appears already migrated to prepared plan usage.

## Suggested Next

Delegate the first Step 4 implementation packet to the edge publication owner
that has `PreparedEdgePublication` available, or have the supervisor/plan owner
confirm that `dispatch_publication.cpp` should be extended to receive that fact
before adding an in-file typed stack-source consumer. Keep the fixed-formal
store-local publication helper as a proof point for existing prepared
store-source plan reuse unless a narrower missing store-source case is named.

## Watchouts

- `PreparedTypedStackSourcePublication` is a typed edge-publication fact, not a
  generic `PreparedValueHome` stack-slot load plan; do not replace scalar
  publication/home consumers with it unless the local decision has the
  corresponding `PreparedEdgePublication`.
- `lower_fixed_formal_store_local_publication` should remain target-local for
  `strb`/`strh`/`str` selection, register view resizing, and AArch64 address
  rendering; those are not prepared authority decisions.
- `memory.cpp` has additional prepared store-source publication consumers, but
  this packet only mapped the Step 4 decisions visible in
  `dispatch_publication.cpp`.

## Proof

Command: `git diff --check -- todo.md`

Result: passed; no `test_after.log` was written for this documentation-only
audit packet.
