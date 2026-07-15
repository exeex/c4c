# Current Packet

Status: Active
Source Idea Path: ideas/open/827_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and select one next native parameter-use row

## Just Finished

- Step 1 traced one bounded next row: a native DirectScalar current-function
  parameter used unchanged as structured fixed direct-call argument 0. The
  value is the `LirOperand::ssa` ID returned by
  `emit_decl_ref_rval_operand` from `FnCtx::param_value_authorities`; its
  unique owner, parameter index, ABI type, and `DirectScalar` ABI are already
  published by the matching `LirFunction.native_body_parameter_definitions`
  entry. The consumer is `LirCallOp.structured_args[0]` emitted by the native
  call-target path, whose operand must be that value and whose `type_ref` must
  equal both the definition type and the direct callee's fixed parameter-0
  type. Step 2 must add an explicit call-argument authority tuple with role
  `FixedDirectCallArgument0`, rather than recovering any fact from argument
  text, names, signatures, or rendered operands.
- This is next and bounded because the existing native parameter definition,
  SSA operand identity, structured argument type, and fixed-callee relation
  meet in one producer/verifier seam, yet no `LirCallArg` carrier currently
  records the parameter-use owner/index/ABI/role. The earlier DirectScalar
  binary LHS/RHS, return, switch-selector, and truthiness-comparison rows are
  expressly excluded; the dirty binary-LHS change is diagnostic evidence only
  and was not used as a selection target.

## Suggested Next

- Step 2: publish and verify only `LirCallOp.structured_args[0]` authority for
  an unchanged native DirectScalar parameter at a fixed direct call. Require
  one matching current-function definition (valid value, current owner,
  index 0-or-greater as published, matching type, DirectScalar ABI), role
  `FixedDirectCallArgument0`, and consumer coherence with structured argument
  0 plus the callee's fixed parameter 0; reject missing, invalid, duplicate,
  foreign, owner/index/type/ABI/role, and consumer-incoherent forms. Do not
  admit later arguments, indirect/variadic/unspecified calls, pointer,
  spilled/load-derived, aggregate/vector, or any other parameter forms.

## Watchouts

- The selected relation is only direct, fixed, argument slot 0. `LirCallArg`
  currently has value identity and `type_ref`, but neither is authority for
  the parameter owner/index/ABI/role; all facts must be mirrored from the
  current function's native definition and checked structurally. Do not edit
  Raw-BIR/importer code or infer authority from presentation fields; all
  nonselected parameter forms remain fail closed.

## Proof

- No build/test ran for this read-only trace packet and `test_after.log` was
  not altered. Step 2 proof, subject to supervisor baseline preparation, is:
  `cmake --build --preset default` followed by
  `ctest --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'`.
  The focused producer target is `frontend_lir_call_type_ref`; 734's accepted
  `f7c33237e` backend 6/6 proof remains historical receiver evidence only.
