# Current Packet

Status: Active
Source Idea Path: ideas/open/119_aarch64_calls_before_call_move_bundle_local_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Local Owner Surface In Place

## Just Finished

Step 1 completed: characterized the existing `lower_before_call_move` cluster
before code movement.

Cluster map:

- Direct caller: `lower_before_call_moves`, which finds the BeforeCall
  `PreparedMoveBundle`, emits prepared outgoing-stack reservation/base setup
  when `PreparedCallPlan::outgoing_stack_argument_area` is present, plans
  call-boundary effects, handles preservation-home population first, and then
  calls `lower_before_call_move` for each BeforeCall explicit move.
- `lower_before_call_move` consumes `PreparedCallPlan`, `PreparedMoveBundle`,
  `PreparedCallBoundaryEffectPlan`, `PreparedMoveResolution`, indexed prepared
  value homes, call-boundary classification, ABI bindings, prepared argument
  source selections, prepared f128 carriers, prepared addressing, and prepared
  outgoing-stack-area bytes.
- Register argument routes use `classify_prepared_call_boundary_move`,
  `find_indexed_prepared_value_home`,
  `make_register_operand_from_prepared_authority`, scalar register-view
  helpers, and `make_call_boundary_move_instruction` to produce GPR/FPR/Q
  call-boundary move records.
- Prior-preserved, local-frame-address, and byval routes consume prepared
  source selections through `make_prior_preserved_call_argument_source`,
  `make_selected_call_argument_source`,
  `make_byval_register_lane_prepared_source`, and
  `validate_aggregate_register_lane_publication`; missing selected authority is
  diagnosed instead of rediscovered locally.
- Frame-slot-to-register routes consume prepared frame-slot value selections
  and destination ABI bindings for scalar FPR and f128/Q-register moves.
- Frame-slot-to-stack and aggregate stack-copy routes consume prepared source
  frame slots, destination stack offsets, selected aggregate address sources,
  and `PreparedCallPlan::outgoing_stack_argument_area` through
  `outgoing_stack_argument_bytes` plus
  `source_memory_after_outgoing_stack_reservation`.
- Immediate scalar arguments are adjacent to the cluster in
  `lower_before_call_immediate_binding`; that helper consumes indexed prepared
  immediate call arguments or falls back to the call plan argument list, then
  materializes scalar GPR/FPR immediates or scalar integer stack immediates.
- f128 routes consume prepared full-width register carriers, prepared constant
  carriers, module-level carrier fallback lookup, `q_register_operand`,
  `materialize_f128_stack_call_argument_source`,
  `make_prepared_f128_carrier_transport_record`, and
  `make_f128_transport_instruction`.

Local-owner boundary:

- Must stay AArch64-local: x16 outgoing-stack-base setup/addressing, stack
  pointer adjustment records, register spelling and register-view selection,
  Q-register rendering, target machine-record construction, inline-asm
  materialization carried by call-boundary records, memory operand spelling,
  and target diagnostics for missing/incomplete prepared authority.
- Must remain prepared/shared authority: call planning, move bundles,
  call-boundary effect ordering, ABI bindings, value homes, source selections,
  f128 carrier selection/completeness, indexed immediate argument publication,
  frame-slot destinations, and outgoing-stack-area sizing.

Focused proof subset candidates:

- `ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure`
  covers direct register moves, prior-preserved register/stack call arguments,
  local frame-address/byval routes, scalar stack moves, immediate register and
  stack argument materialization, f128 Q-register moves, f128 frame-slot to
  Q-register, f128 frame-slot to outgoing stack slot, and outgoing stack base
  print checks through named internal fixtures.
- `ctest --test-dir build -R '^backend_aarch64_return_lowering$' --output-on-failure`
  covers module-build scalar integer and FP immediate materialization before
  direct calls.
- `ctest --test-dir build -R '^backend_prepare_frame_stack_call_contract$' --output-on-failure`
  covers shared prepared outgoing-stack-area and selected frame-slot source
  contracts consumed by the AArch64 stack argument lowering path.
- `ctest --test-dir build -R '^backend_prealloc_call_boundary_classification$' --output-on-failure`
  covers shared call-boundary classification and ABI binding availability that
  `lower_before_call_move` depends on.
- Optional record-shape smoke: `ctest --test-dir build -R '^backend_aarch64_target_instruction_records$' --output-on-failure`
  for call-boundary record payload/printing invariants, and
  `ctest --test-dir build -R '^backend_aarch64_call_boundary_owner$' --output-on-failure`
  for smaller call-boundary owner record behavior.

Missing coverage / watch list:

- The strongest single executable subset is test-binary level, not fixture
  level; several named routes live inside `backend_aarch64_instruction_dispatch`.
- No new coverage gap was found for register, stack, immediate, FP/f128, or
  outgoing-stack routes, but Step 4 should re-run the focused subset after code
  movement and record any route that becomes unprovable.

## Suggested Next

Execute Step 2 by extracting an in-place AArch64-local owner surface for the
before-call move cluster. Keep the owner fed by explicit prepared inputs and
avoid moving or recomputing shared authority.

## Watchouts

- Keep `lower_before_call_immediate_binding` in view: immediate scalar
  arguments are adjacent to, but not directly inside, `lower_before_call_move`.
- Preserve the caller-side ordering in `lower_before_call_moves`: outgoing
  stack reservation/base setup, preservation-home population, explicit moves,
  then immediate ABI bindings.
- Do not reselect stack homes, f128 carriers, immediate publication, or
  outgoing stack area authority.
- Do not claim progress through helper renames, line-count reduction, or a
  broad file split while the old before-call authority remains in place.
- Keep code movement target-local unless a narrow declaration adjustment is
  required.

## Proof

No build or test command was required. This was a docs/analysis-only packet and
only `todo.md` changed, so no `test_after.log` was generated.
