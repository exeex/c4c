Status: Active
Source Idea Path: ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Parameter Home Shape

# Current Packet

## Just Finished

Step 2 from `plan.md` is complete as an evidence-only classification of the
`src/20001017-1.c` parameter/call ABI home shape.

- Rejected call:
  `unsupported_call_abi: RV64 object route requires supported ordinary same-module call ABI/result lowering; function=main; block=entry; block_index=0; instruction_index=0; callee=bug; args=13; planned_args=13; result=none`.
- Prepared caller shape from
  `build/agent_state/635_step1_20001017-1.prepared.txt`: `main` has
  `call block_index=0 inst_index=0 wrapper_kind=same_module callee=bug
  outgoing_stack_argument_area=40`.
- Accepted-by-shape register side:
  - arg0: GPR register `%lv.C.0`/`t0` to `a0`, with explicit
    local-frame-address materialization and freshness publication.
  - arg1-4: GPR immediates `66`, `1`, `2`, `3` to `a1`-`a4`.
  - arg5: FPR immediate `0x4010000000000000` to `fa5`.
  - arg6: GPR register `%lv.A.0`/`s1` to `a6`, with explicit
    local-frame-address materialization and freshness publication.
  - arg7: GPR immediate `5` to `a7`.
- Unsupported prepared stack-argument side:
  - arg8: GPR register `%lv.B.0`/`s2` to `dest_stack_offset=0`,
    `dest_stack_size=8`.
  - arg9: GPR immediate `6` to `dest_stack_offset=8`,
    `dest_stack_size=8`.
  - arg10: FPR immediate `0x401C000000000000` to
    `dest_stack_offset=16`, `dest_stack_size=8`.
  - arg11: GPR register `%lv.C.0`/`t0` to `dest_stack_offset=24`,
    `dest_stack_size=8`, with explicit local-frame-address materialization
    and freshness publication.
  - arg12: GPR immediate `8` to `dest_stack_offset=32`,
    `dest_stack_size=8`.
- Prepared callee formal homes from
  `build/agent_state/635_step4_src_20001017-1.c_prepared_dump.txt` are
  explicit, not missing: `%p.B` is `kind=stack_slot slot_id=9 offset=32`,
  `%p.fdB` is `kind=stack_slot slot_id=10 offset=40`, `%p.b` is
  `kind=stack_slot slot_id=12 offset=48`, `%p.C` is `kind=stack_slot
  slot_id=13 offset=56`, and `%p.fdC` is `kind=stack_slot slot_id=11
  offset=44`.
- Storage/frame facts match the callee homes: `%p.B`, `%p.fdB`, `%p.b`,
  `%p.C`, and `%p.fdC` are `encoding=frame_slot` with stack offsets `32`,
  `40`, `48`, `56`, and `44`; `bug` has `frame_size=64` and `main` has
  `frame_size=24`.
- Classification against closed work: this is no longer the old producer
  missing-home/offsetless stack-parameter gap from ideas 424/512. Idea 512
  says stack-passed homes must be explicit, and the current prepared dumps do
  publish explicit caller stack offsets plus callee stack-slot homes. Idea 374
  covers scalar formal stack-slot home admission; that owner is not the current
  first stop. The active owner is RV64 object-route ordinary-call consumption
  for scalar stack-destination call arguments.

## Suggested Next

Step 3 should implement one semantic RV64 object-route ordinary same-module
call ABI path for explicit scalar stack-destination call arguments. The slice
should consume `outgoing_stack_argument_area=40`, emit the needed stack
adjustment, store scalar GPR/immediate/FPR argument values to prepared
`dest_stack_offset`/`dest_stack_size` locations for args 8-12, perform the
direct call, then restore `sp`. It should be general over explicit prepared
stack call-argument facts, not named to `src/20001017-1.c` or argument indexes
8-12, and should keep missing, ambiguous, aggregate, dynamic, unsupported bank,
or malformed stack destinations fail-closed.

## Watchouts

- Candidate owners ruled out: branch stack clobber-safety is not first owner;
  the old producer-contract missing stack-parameter homes from idea 512 are
  now published in prepared facts; non-register formal home admission from idea
  374 is not the current rejection.
- Candidate owner active: RV64 object emission for prepared ordinary
  same-module call ABI lowering with scalar stack arguments.
- The object-route call fragment currently returns `std::nullopt` when an
  argument lacks `destination_register_bank`; the first unsupported prepared
  argument by that path is arg8. The final diagnostic remains call-level
  `unsupported_call_abi`, so Step 3 should add focused diagnostics/tests rather
  than relying on the broad call-level text.
- Do not reconstruct stack argument placement from ABI formulas, source syntax,
  or final assembly. The only admissible authority here is the prepared call
  plan, move bundle, value homes, and outgoing stack argument area.

## Proof

No proof rerun; supervisor explicitly marked this as an evidence-only
classification packet. Existing evidence artifacts inspected:

- `test_before.log`
- `build/rv64_gcc_c_torture_backend/src_20001017-1.c/case.log`
- `build/agent_state/635_step1_20001017-1.prepared.txt`
- `build/agent_state/635_step4_src_20001017-1.c_prepared_dump.txt`
- `build/agent_state/424_step2_infrastructure_classification/classification_summary.md`
- `ideas/closed/374_rv64_object_route_non_register_param_homes.md`
- `ideas/closed/512_stack_passed_parameter_home_publication.md`
