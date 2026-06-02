# AArch64 Calls Stack And Frame-Slot Operand Owner

## Goal

Extract or consolidate a narrow AArch64-local owner for stack, frame-slot,
sret, aggregate-source, and prior stack-preserved memory operands used by call
lowering, while preserving prepared-source authority and existing call-boundary
behavior.

## Why This Exists

The calls-owner audit identified a stable local cluster inside
`src/backend/mir/aarch64/codegen/calls.cpp`: frame-slot address legality,
selected frame-slot sources, local-frame and sret memory sources, stack call
argument destinations, aggregate argument sources, endpoint stack storage, and
prior stack-preserved value sources. These helpers render already-selected
prepared endpoints and homes into AArch64 memory operands and address
materialization details. That boundary is narrower than before/after call move
lowering and does not need to rederive prepared call plans, move bundles,
publication producers, result facts, or preservation facts.

## In Scope

- Audit and group the AArch64 calls-side helpers that build stack/frame-slot
  memory operands and address materialization lines:
  `call_boundary_frame_slot_direct_offset_is_encodable`,
  `materialize_call_boundary_frame_slot_address_lines`, `stack_copy_address`,
  `make_selected_frame_slot_source`, `make_sret_memory_return_address_source`,
  `make_selected_local_frame_address_source`,
  `make_stack_call_argument_destination`,
  `make_aggregate_call_argument_source`,
  `make_prior_preserved_call_argument_source`,
  `endpoint_has_stack_storage`, `make_frame_slot_memory_from_endpoint`,
  `find_prior_stack_preserved_value_before_instruction`, and
  `make_prior_stack_preserved_value_source`.
- Create a local helper owner or tighten the existing helper boundary only
  where the API consumes prepared endpoints, value homes, frame plans, and
  selected source facts.
- Preserve AArch64 address legality checks, frame-slot offset handling,
  memory operand shape, diagnostics, and emitted call-boundary/source records.
- Keep before-call, after-call, scalar producer bridge, and late publication
  control flow in their current calls-side owners unless a separate idea proves
  a narrower boundary.

## Out Of Scope

- Moving source-selection authority, prepared call plans, move bundles,
  publication producers, after-call result facts, preservation facts, or
  aggregate transport facts out of their existing owners.
- Turning stack/frame-slot operand rendering into a shared BIR/prealloc owner
  without a separate missing target-neutral fact.
- Rewriting ordinary call-boundary record construction, call instruction
  records, aggregate byval lane publication, indirect callee materialization,
  or scalar producer dispatch bridging.
- Changing unsupported diagnostics, selection status, assembly output, or
  frame layout behavior as part of a cleanup.
- Treating line-count reduction in `calls.cpp` as acceptance without a clearer
  operand-owner API and focused proof.

## Acceptance And Proof Expectations

- The resulting boundary is a narrow AArch64-local operand owner that consumes
  existing prepared endpoints and homes rather than selecting or rederiving
  them.
- Existing call-boundary moves, outgoing stack arguments, sret memory return
  sources, aggregate argument sources, prior stack-preserved values, and
  frame-slot address materialization keep equivalent records, diagnostics, and
  assembly.
- Focused proof covers stack call arguments, frame-slot call-boundary sources,
  sret/result-related memory sources, aggregate stack sources, prior
  stack-preserved call values, and at least one rejection or non-encodable
  frame-slot path when available.
- Build proof covers the AArch64 backend targets affected by `calls.cpp` and
  any new local helper files.
- If helper APIs or call-boundary record construction paths change, use
  matching before/after logs for the focused test subset.

## Reviewer Reject Signals

- The implementation reconstructs prepared call plans, move bundles,
  publication producers, result facts, preservation facts, or aggregate
  transport facts under a new stack/frame-slot helper name.
- A shared BIR/prealloc helper is introduced for AArch64 frame-slot addressing
  or memory operand rendering without a concrete target-neutral fact and
  separate evidence route.
- The slice claims progress through helper renames, file movement, or
  expectation rewrites while the same mixed calls-side ownership remains.
- Unsupported expectations, diagnostics, selection status, frame layout, or
  assembly output are weakened without explicit approval.
- The route sweeps in before-call move ordering, after-call republication,
  aggregate byval lane publication, indirect callee materialization, or scalar
  producer bridging beyond the stack/frame-slot operand boundary.
- Proof covers only one named stack-argument case while frame-slot sources,
  sret/local-frame sources, aggregate sources, prior stack-preserved values,
  and non-encodable/rejection behavior remain unexamined.

## Closure Note

Closed after implementation in:

- `c6876e26a` (`Introduce AArch64 stack frame-slot operand owner`)
- `9f917bc5a` (`Route stack frame-slot memory return storage`)

The route introduced the local `StackFrameSlotCallOperandOwner` in
`src/backend/mir/aarch64/codegen/calls.cpp`. The owner consumes prepared facts
and renders stack/frame-slot memory operands or existing call-boundary operand
records without selecting or rederiving prepared call plans, move bundles,
publication producers, result facts, preservation facts, or aggregate transport
facts.

Routed in-scope stack/frame-slot operand helpers through the owner, including:

- selected frame-slot sources
- sret and selected local-frame address sources
- stack call argument destinations
- aggregate stack-copy sources
- endpoint stack storage
- prior stack-preserved sources
- memory-return frame-slot storage

Intentionally outside this route:

- aggregate byval lane publication and transport chunk/lane validation
- immediate scalar stack argument publication
- after-call result publication
- generic prepared value and return moves
- after-call preserved-value publication and republication
- source-selection, call-plan presence checks, and call-record construction
  decisions

Separate open follow-ups remain:

- `ideas/open/94_aarch64_calls_f128_carrier_operand_owner.md`
- `ideas/open/95_aarch64_calls_immediate_scalar_argument_publication_owner.md`

Those follow-ups were not absorbed into this route.

Proof:

- Focused before/after scope passed 8/8 before and 8/8 after.
- Close-time regression guard passed with non-decreasing pass-count mode:

```bash
cmake --build --preset default
ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_memory_operand_contract|backend_aarch64_prepared_memory_operand_records|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor)$' --output-on-failure > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

The supervisor also reported broader validation on June 2, 2026:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

That broader backend scope passed 169/169.
