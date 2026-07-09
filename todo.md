Status: Active
Source Idea Path: ideas/open/625_prepared_stack_slot_preservation_source_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate prepared preserve source authority production

# Current Packet

## Just Finished

Completed Step 2 authority inspection for prepared stack-slot preserve source
publication. Used `c4c-clang-tool-ccdb` on
`src/backend/prealloc/call_plans.cpp` before narrow text reads; AST lookup
confirmed `build_call_preserved_values(...)` as the producer and
`populate_call_plans(...)` as the ordinary call-plan caller.

Concrete producer target:

- `src/backend/prealloc/call_plans.cpp:1601`
  `build_call_preserved_values(...)`, specifically the StackSlot preserve path
  that calls `make_preservation_value_source_endpoint(...)` at lines
  1686-1691 and `make_preservation_destination_endpoint(...)` at lines
  1692-1693.
- `src/backend/prealloc/call_plans.cpp:717`
  `make_preservation_value_source_endpoint(...)` is the narrow endpoint
  construction helper. It already knows how to publish concrete register homes
  when `prefer_value_home` is true, but stack-slot preserves currently pass
  `prefer_value_home=false`; that leaves ordinary ABI-home rows as
  `preservation_source=register:value#N`.
- `src/backend/prealloc/calls.hpp:790` / `:809` already provide the carrier:
  `PreparedCallBoundaryEffectEndpoint preservation_source` and
  `preservation_destination` on `PreparedCallPreservedValue`. No new RV64
  inference carrier is needed for this route.

Exact facts to publish for complete caller-saved stack-slot reuse preserves:

- Source endpoint: `encoding=Register`, `storage_kind=Register`,
  `value_id`, `value_name`, `register_name`, `register_bank`,
  `contiguous_width`, `occupied_register_names`, and `register_placement` when
  the prepared value home or storage publication proves a concrete register.
- Destination endpoint: keep the existing stack-slot facts from
  `make_preservation_destination_endpoint(...)`: `slot_id`,
  `stack_offset_bytes`, `stack_size_bytes`, `stack_align_bytes`, and
  `spill_slot_placement`.
- Preserve association: keep `preservation_reason=
  caller_saved_clobber_reuse_stack_preservation`, value identity, route
  `StackSlot`, and the current callsite association through
  `PreparedCallPlan::block_index` / `instruction_index`.

Expected Step 3 producer-level proof rows:

- Extend `check_stack_cross_call_preservation_contract()` in
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp` to
  require a concrete register source endpoint for a stack-slot preserve before
  RV64 object emission.
- Reuse the Step 1 corpus rows as diagnostic confirmation after the producer
  test goes green: `20020529-1.c` should expose `%p.b` from `a1`,
  `20000412-4.c` should expose `%p.j` from `a1` and `%p.width` from `a3`, and
  `pr51933.c` should expose `%p.y` from `a1` when the row reaches prepared
  dumps.

Rows that remain fail-closed or outside this owner:

- Any stack-slot preserve whose source endpoint cannot be proven from prepared
  home/storage publication remains fail-closed as `register:value#N`; RV64 must
  not recover it from ABI parameter index, function storage summaries, source
  filename, or final assembly shape.
- `20000412-4.c` still has a downstream
  `rv64_prepared_move_bundle_consumer` residual for an ambiguous move-bundle
  path; that is not the producer authority target.
- `20020529-1.c` still has an ordinary same-module call/result ABI residual
  after preserve publication; that belongs to the call ABI/result owner.
- `pr51933.c` still stops on `unsupported_inline_asm_fragment`; inline asm
  carrier policy remains a non-goal.

## Suggested Next

Execute Step 3 from `plan.md`: publish producer-side concrete source endpoint
facts for complete caller-saved stack-slot preserves in the prepared call-plan
surface, then add a focused producer-level contract before touching RV64
consumers.

## Watchouts

- The source-name route must be producer-side prepared authority. Do not add an
  RV64 fallback that guesses `a1`/`a3` from parameter position or target ABI
  layout.
- Prefer extending `make_preservation_value_source_endpoint(...)` or its call
  site over adding a parallel carrier; the existing
  `PreparedCallBoundaryEffectEndpoint` fields are already printed and consumed.
- Keep stack-slot source preserves such as
  `preservation_source=stack_slot:slot#1100:value#2199` separate from the
  caller-saved register-source route.
- Do not broaden into move-bundle ambiguity, ordinary call/result ABI policy,
  inline asm policy, expectation rewrites, unsupported marker changes,
  allowlists, timeouts, runtime mismatch, or accounting.

## Proof

Ran exactly the supervisor-delegated Step 2 proof command from `/workspaces/c4c`.
`test_after.log` contains the `cmake --build build --target c4cll` proof plus
the appended authority summary. Artifacts are in
`build/agent_state/625_step2_preserve_source_authority/`.
