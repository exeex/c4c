Status: Active
Source Idea Path: ideas/open/649_pointer_global_local_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate The Publication Authority Boundary

# Current Packet

## Just Finished

Completed Step 2 boundary inspection for `%lv.l` using the Step 1 evidence and
AST-backed symbol/callee queries before opening implementation slices.

Boundary decision: the missing owner is RV64 object-route consumer/emission
admission, not a broad prepared-producer rewrite. The producer already emits
the key facts for the representative:

- `store_source function=foo block=block_3 inst=0 source=@f status=available
  intent=store_local_publication`.
- `address_materialization block=block_3 inst_index=0 kind=direct_global
  result=@f symbol=f policy=direct`.
- `%lv.l` is `object #1`, `frame_slot=#0`, offset 0, size 8, align 8, and the
  prepared memory accesses are base-plus-offset and proven in bounds.

Owned implementation surface for the next packet should stay narrow:

- Consumer/admission: `src/backend/mir/riscv/codegen/object_emission.cpp`,
  around `fragment_for_prepared_instruction(...)`,
  `prepared_memory_access_for_local_instruction(...)`,
  `prepared_store_source_publication_for_instruction(...)`, and the
  `local_memory_diagnostic` branch for `StoreLocalInst`/`LoadLocalInst`.
- Local emission helper surface:
  `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`, around
  `fragment_for_prepared_store_local(...)`,
  `append_rv64_materialize_or_move_store_value_local(...)`, and
  `fragment_for_prepared_load_local(...)`.
- Producer facts to consume, not broaden:
  `src/backend/prealloc/stack_layout/coordinator.cpp` already builds the direct
  frame-slot access and store-source publication inputs through
  `append_direct_frame_slot_accesses(...)`, `build_direct_frame_slot_access(...)`,
  and the existing publication planning path.

Focused positive test target shape: a small RV64 object-route case with a
global object address stored into a local pointer slot, then reloaded/used via
that slot, matching the representative shape `short *l = &f; *l = 0;`. The
positive should require the direct-global materialization for `@f` to be tied to
the exact `StoreLocalPublication` into `%lv.l`/frame slot #0 and should advance
RV64 object emission for the representative `pr57861.c` route.

Fail-closed behavior to preserve:

- Reject stale pointer values: do not accept if the selected publication source
  is not fresh at the local store/load use or if an intervening write invalidates
  the global/pointer identity.
- Reject missing global identity: require a direct global address
  materialization with a concrete symbol matching the published source value.
- Reject ambiguous publication order: require the publication/materialization
  to be at the selected instruction or otherwise ordered before the accepted
  use; do not search loosely across unrelated blocks/instructions.
- Reject mismatched slots: require the `StoreLocalPublication` destination
  access to be the exact prepared memory access/frame slot/offset/size/align
  being emitted.
- Reject scalar-only facts: scalar frame-slot layout authority and direct
  global-symbol scalar memory authority are not sufficient without the
  pointer/global local-publication fact.

## Suggested Next

Proceed to Step 3 with a narrow RV64 consumer/emission change that consumes the
existing `StoreLocalPublication` plus direct-global address materialization
facts for the exact `%lv.l` frame slot. If Step 3 ownership excludes
`prepared_local_memory_emit.cpp`, split the packet rather than broadening
`object_emission.cpp` or the producer.

## Watchouts

- Do not treat scalar frame-slot local-memory facts from idea 640 as
  pointer/global local-publication authority.
- Do not reopen direct global-symbol local-memory support from idea 631 or
  generic pointer freshness from idea 600 unless fresh evidence proves a new
  local-publication boundary.
- Do not use the `main` call-argument direct-global select-chain evidence as
  the owner for this idea; the representative owner is inside `foo` around
  `%lv.l`.
- Do not rewrite prepared provenance or mark all unknown local pointer slots as
  supported. The discovered positive route depends on exact publication,
  direct-global identity, slot identity, and ordering.
- Do not infer authority from source spelling, final assembly order,
  diagnostics, testcase identity, local/global names, or stack-slot shape.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

No build or CTest proof was required for this diagnostic-only packet, and
`test_after.log` was not overwritten.

Evidence source: Step 1 summary at
`build/agent_state/649_step1_pointer_global_local_evidence/summary.md`.

AST-backed inspection used:

- `c4c-clang-tool-ccdb list-symbols` on
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `src/backend/prealloc/stack_layout/coordinator.cpp`, and
  `src/backend/prealloc/prepared_contract_verifier.cpp`.
- `c4c-clang-tool-ccdb function-callees` for
  `build_direct_frame_slot_access`,
  `publish_pointer_loaded_from_global_local_memory_authority`,
  `append_direct_frame_slot_accesses`,
  `fragment_for_prepared_instruction`,
  `fragment_for_prepared_store_local`, and
  `fragment_for_prepared_load_local`.
- Targeted source slices were then read only around the symbols listed above.
