Status: Active
Source Idea Path: ideas/open/360_lir_bir_vararg_va_arg_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current Vararg Representation

# Current Packet

## Just Finished

Completed Step 1 audit of the current frontend/LIR/BIR/prepared vararg
representation.

- Frontend/parser/HIR keeps variadic identity as function/type metadata:
  parser nodes carry `variadic`, HIR functions carry `attrs.variadic`, HIR
  function pointer signatures carry variadic flags, and `VaArgExpr` represents
  `__builtin_va_arg(ap, type)`.
- LIR carries function-entry identity in `LirFunction::signature_is_variadic`
  plus fixed `signature_params`/`signature_param_type_refs`; BIR lowering turns
  that into `bir::Function::is_variadic` and a synthetic `bir::Param` with
  `is_varargs=true` after the fixed parameters.
- LIR carries `va_start`, `va_end`, and `va_copy` as typed operations
  `LirVaStartOp`, `LirVaEndOp`, and `LirVaCopyOp`; BIR lowers them to runtime
  placeholder calls `llvm.va_start.p0`, `llvm.va_end.p0`, and
  `llvm.va_copy.p0.p0`. Prepared helper classification currently recognizes
  `va_start`, `va_copy`, and `llvm.va_arg.*`; it does not classify `va_end`.
- LIR carries semantic `va_arg` only when target lowering emits `LirVaArgOp`.
  BIR lowers scalar forms to `llvm.va_arg.<type>` calls with
  `va_arg_payload_abi`; aggregate forms become `llvm.va_arg.aggregate` with a
  destination payload pointer, payload ABI, and optional AAPCS64 HFA lane facts.
  Some target paths still expand `va_arg` before BIR as raw loads/stores/GEPs:
  RV64 single-slot scalar/pointer payloads are directly advanced and loaded in
  LIR, and AArch64/AMD64 have legacy inline lowering paths unless the semantic
  form is selected.
- Prepared output already exposes call-site variadic identity through call
  plans, including direct extern variadic wrapper classification and
  `variadic_fpr_arg_register_count`.
- Prepared vararg entry/access plans currently exist only for AAPCS64-defined
  functions: `PreparedVariadicEntryPlanFunction` records fixed named parameter
  counts, named GP/FP register counts, register-save and overflow-area storage,
  `va_list_layout`, helper resources, helper operand homes, scalar
  `va_arg` source/progression fields, aggregate/HFA `va_arg` access plans, and
  missing required facts. Prepared printer dumps these under
  `--- prepared-variadic-entry-plans ---`.
- Value homes/liveness feed the AAPCS64 helper operand homes through
  `PreparedValueHome` lookup. For RV64 there is no equivalent prepared entry
  plan, `va_list` layout, fixed-argument home boundary, or prepared scalar/
  aggregate `va_arg` access plan exposed to object emission.
- First missing contract boundary: after BIR records `function.is_variadic`
  and placeholder `va_*` calls, prepared planning only publishes explicit
  entry and `va_arg` consumption plans for AAPCS64. RV64 object emission sees
  `function.is_variadic` but receives no target-independent prepared contract
  naming fixed argument homes, vararg state layout, or `va_arg` consumption
  plans; it therefore rejects variadic functions at prepared-object admission.

Representative RV64 object-route probes were classification-only:
`src/920908-1.c` and `src/20030914-2.c` both fail before object emission with
`unsupported_function_admission: variadic functions are not supported by the
RV64 object route`. `src/920908-1.c` includes aggregate `va_arg`; `src/20030914-2.c`
is a variadic function without `va_*` use, so the current blocker is the
function-entry/admission contract before per-helper lowering.

## Suggested Next

Start Step 2 by defining the shared prepared contract for non-AAPCS64 variadic
entry: fixed argument home boundary, `va_list`/overflow-area state, and the
target hook or unsupported diagnostic shape for `va_start` and `va_arg`.

## Watchouts

Do not bypass the missing prepared contract by teaching RV64 object emission to
infer vararg state from source-shaped BIR or torture-case names. Also preserve
the existing AAPCS64 prepared publication behavior: it already has scalar and
aggregate/HFA `va_arg` access-plan facts, while `va_end` is presently just a BIR
placeholder call with no prepared helper classification.

## Proof

Ran the delegated proof command exactly and preserved `test_after.log`.

- `cmake --build --preset default`: no work to do.
- CTest subset passed 5/5:
  `backend_cli_dump_bir_00204_stdarg_semantic_handoff`,
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`,
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`,
  `backend_codegen_route_x86_64_variadic_sum2_observe_semantic_bir`, and
  `backend_codegen_route_x86_64_variadic_va_copy_accumulate_observe_semantic_bir`.
- RV64 allowlist probe classified both delegated cases as current failures at
  prepared-object admission:
  `src/920908-1.c` and `src/20030914-2.c`.

Proof log: `test_after.log`.
