Status: Active
Source Idea Path: ideas/open/100_bir_runtime_intrinsic_placeholder_identity_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Placeholder Identity Producers And Consumers

# Current Packet

## Just Finished

Completed Step 1 inventory for runtime/intrinsic placeholder identity producers and prealloc raw callee-string consumers.

Producer inventory:

- `src/backend/bir/bir.hpp`: `CallInst` carries `callee`, `callee_link_name_id`, `callee_value`, `intrinsic`, and `inline_asm`. Commented contract says direct user/extern calls use `callee_link_name_id`; runtime/intrinsic placeholders keep it invalid and use `callee` as compatibility/display token. Classification: `runtime-placeholder-compat-string-exception` for current placeholder token storage; `needs-structured-placeholder-identity` if consumers need semantic decisions beyond display.
- `src/backend/bir/lir_to_bir/calling.cpp`: ordinary direct calls resolve `direct_callee_link_name_id` through the LIR link-name table, fall back only through the documented no-id direct-call bridge, and assign `lowered_call.callee_link_name_id`. Classification: `ordinary-link-name-authority`.
- `src/backend/bir/lir_to_bir/calling.cpp`: `lower_runtime_intrinsic_inst` synthesizes placeholder `CallInst`s for `llvm.va_start.p0`, `llvm.va_end.p0`, `llvm.va_copy.p0.p0`, `llvm.va_arg.<type>`, `llvm.va_arg.aggregate`, `llvm.inline_asm`, `llvm.stacksave`, `llvm.stackrestore`, `llvm.fabs.*`, and selected AArch64 semantic intrinsics. These generally leave `callee_link_name_id` invalid; some AArch64/fabs paths also populate `CallInst::intrinsic`. Classification: variadic/stack/inline-asm placeholders are `runtime-placeholder-compat-string-exception` today and `needs-structured-placeholder-identity` for prealloc consumers; AArch64/fabs intrinsic metadata is `runtime-placeholder-identity-authority` but still needs prealloc contract proof if shared.
- `src/backend/bir/bir_printer.cpp`: `render_call_target` prints `callee` unless the call is indirect, and explicitly treats `callee_link_name_id` as direct-call semantic identity. Classification: `ordinary-link-name-authority` plus display-only placeholder compatibility; `no-action` for Step 1.
- `src/backend/bir/bir_validate.cpp` and `src/backend/bir/lir_to_bir/module.cpp`: validation accepts non-indirect calls with a raw `callee` when `callee_link_name_id` is invalid, and module post-processing resolves linked callees by `callee_link_name_id` first, then raw `callee`. Classification: `needs-contract-proof` if a structured placeholder identity is added, because validation/module fallback must preserve ordinary direct-call authority.

Consumer inventory:

- `src/backend/prealloc/variadic.hpp`: `prepared_variadic_entry_helper_kind_for_callee` classifies `llvm.va_start.p0`, `llvm.va_copy.p0.p0`, `llvm.va_arg.aggregate`, and any `llvm.va_arg.` prefix directly from raw `callee`. Classification: `raw-string-semantic-authority`; strongest `needs-structured-placeholder-identity` candidate.
- `src/backend/prealloc/variadic_entry_plans.cpp`: `populate_aapcs64_variadic_entry_helper_operand_home_authority` and `populate_aapcs64_variadic_entry_helper_resource_authority` call `prepared_variadic_entry_helper_kind_for_callee(call->callee)` to create helper operand homes, scalar/aggregate va_arg access plans, and resource authority. Classification: `raw-string-semantic-authority`; `needs-contract-proof` for variadic helper behavior after any contract change.
- `src/backend/prealloc/call_plans.cpp`: `resolve_direct_callee` uses `callee_link_name_id` first for direct calls, rejects mismatched raw spelling, then falls back to raw `callee` when no link id exists. Classification: `ordinary-link-name-authority` for normal direct calls; raw fallback is a compatibility path that can accidentally treat placeholders as direct names, so `needs-contract-proof`.
- `src/backend/prealloc/call_plans.cpp`: `call_argument_allows_local_aggregate_address_publication` rejects calls whose raw `callee` starts with `llvm.` before allowing pointer argument publication. Classification: `raw-string-semantic-authority`; `needs-structured-placeholder-identity` or explicitly documented compatibility exception.
- `src/backend/prealloc/stack_layout/analysis.cpp`: `apply_aggregate_address_publication_hints` skips collecting published pointer values for calls whose raw `callee` starts with `llvm.`. Classification: `raw-string-semantic-authority`; likely shares the same contract decision as `call_argument_allows_local_aggregate_address_publication`.
- `src/backend/prealloc/dynamic_stack.cpp`: `is_dynamic_alloca_call`, `populate_dynamic_stack_plan`, and `dynamic_alloca_type_text` recognize `llvm.stacksave`, `llvm.stackrestore`, and `llvm.dynamic_alloca.*` from raw callee strings to build dynamic stack ops. Classification: `raw-string-semantic-authority`; likely acceptable only as `runtime-placeholder-compat-string-exception` unless Step 2 widens structured identity to stack-state placeholders.
- `src/backend/prealloc/frame_plan.cpp`: `populate_frame_plan` marks `plan.has_dynamic_stack` through raw `llvm.stacksave`, `llvm.stackrestore`, and `is_dynamic_alloca_call(call->callee)`. Classification: `raw-string-semantic-authority`; `needs-contract-proof` if dynamic stack identity remains string-only.
- `src/backend/prealloc/regalloc.cpp`: `function_has_dynamic_stack_operation` uses the same raw stack-state helper checks to reserve dynamic-stack-sensitive behavior. Classification: `raw-string-semantic-authority`; `needs-contract-proof` if retained.
- `src/backend/prealloc/frame_plan.cpp`: other `callee_saved` occurrences are register-save concepts, not call identity consumers. Classification: `no-action`.
- `src/backend/prealloc/regalloc.cpp`: runtime helper mapping includes `append_i128_runtime_helper_mappings` and `append_f128_runtime_helper_mappings`, but no raw callee-string predicate for runtime placeholders in this Step 1 target set. Classification: `no-action`.

## Suggested Next

Proceed to Step 2 by choosing whether the common contract is a new structured `CallInst` placeholder identity, a shared helper API over existing fields, or a narrow set of documented compatibility exceptions.

## Watchouts

- Preserve ordinary direct-call `callee_link_name_id` authority in `resolve_direct_callee`; do not add raw-name fallback for linked calls.
- Variadic helper classification and local aggregate address publication are the strongest shared contract candidates because both currently make semantic decisions from raw `llvm.` spelling.
- Dynamic stack checks are also raw-string semantic authority, but may be better retained as documented stack-state compatibility exceptions if Step 2 keeps the first implementation narrow.
- AArch64/fabs placeholders already carry `CallInst::intrinsic`; Step 2 should decide whether that identity is reusable or whether variadic/stack placeholders need their own kind.

## Proof

AST-backed inventory used `c4c-clang-tool`/`c4c-clang-tool-ccdb` for `CallInst`, `lower_runtime_intrinsic_inst`, variadic helper classification, dynamic stack, stack layout, frame plan, regalloc, and call planning symbols before targeted raw-text checks.

Delegated proof passed:

```sh
git diff --quiet -- src/backend/bir src/backend/prealloc && printf 'analysis-only Step 1 proof: no implementation changes under src/backend/bir or src/backend/prealloc\n' > test_after.log
```

Proof log: `test_after.log`.
