Status: Active
Source Idea Path: ideas/open/192_hir_compile_time_rendered_registry_api_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Record Remaining Compatibility Surface

# Current Packet

## Just Finished

Completed Step 4 - Record Remaining Compatibility Surface.

Remaining rendered registry compatibility ledger:
- `CompileTimeState::find_template_def(std::string)`,
  `has_template_def(std::string)`, and `is_consteval_template(std::string)`
  remain public/reachable rendered template-function compatibility APIs.
  Current owners are HIR initial build and template retry plumbing:
  `collect_function_template_specializations`,
  `get_template_param_order_from_instances`, `record_template_seed`,
  `resolve_template_call_name`, `collect_template_instantiations`,
  `instantiate_deferred_template`, `TemplateInstantiationStep`, template
  deduction, and call-result inference. Removal condition: template call and
  retry metadata must carry the primary template declaration/key from initial
  discovery through deferred instantiation, so engine retry code no longer
  needs to reconstruct primary identity from `source_template` rendered text.
- `CompileTimeState::find_template_struct_def(std::string)` and
  `find_template_struct_specializations_no_metadata_compat(std::string)` remain
  rendered template-struct compatibility APIs. Current owners are pending
  template type canonicalization and no-owner specialization lookup fallback.
  Removal condition: pending template type work items and deferred NTTP
  parsing must always carry a structured primary owner key or `Node*`; only
  then can rendered primary-name mirrors stop being reachable.
- `CompileTimeState::find_consteval_def(std::string)` and
  `has_consteval_def(std::string)` remain public/reachable rendered consteval
  APIs. Current owners are pending consteval replay/evaluation by
  `PendingConstevalExpr::fn_name`, existing tests/no-metadata fixtures, and
  the address-of consteval diagnostic path in `scalar_control.cpp`. Removal
  condition: pending consteval records must carry the callee
  `ConstEvalStructuredNameKey` or declaration identity, and diagnostics must be
  able to report the user spelling without using the rendered map as semantic
  authority.
- `CompileTimeState::consteval_fn_defs()`, `enum_consts()`, and
  `const_int_bindings()` remain rendered maps passed into
  `evaluate_consteval_call` as compatibility mirrors. Current owner is the
  consteval evaluator ABI, which also receives TextId/structured maps.
  Removal condition: `ConstEvalEnv` and recursive consteval calls must complete
  migration to structured function/value-binding keys for all metadata-rich
  paths; rendered maps can then be diagnostics/no-metadata-only inputs or be
  removed from the evaluator signature.
- `CompileTimeState::find_enum_const(key, rendered_name)` and
  `find_const_int_binding(key, rendered_name)` remain structured-first public
  APIs with rendered fallback only when no complete value-binding key is
  available. Current owner is global enum/const-int consteval compatibility.
  Removal condition: every global enum constant and const-int producer/consumer
  must supply a complete `CompileTimeValueBindingKey`.

Address-of consteval classification:
- `Lowerer::lower_addr_expr` currently calls `has_consteval_def(name)` only to
  produce the "cannot take address of consteval function" diagnostic for an
  `NK_VAR` spelling. This can remain diagnostic/no-metadata compatibility for
  idea 192 because it does not lower or evaluate a consteval call and does not
  recover a semantic result after structured miss. It should be covered by a
  separate follow-up only when consteval reference nodes carry structured
  callee identity into address-of lowering.

Follow-up recommendation:
- Open a successor idea for carrying structured consteval identity through
  `PendingConstevalExpr` and recursive consteval evaluation. That is the
  clearest remaining metadata-rich surface after this route; template function
  and template-struct rendered compatibility are broader deferred-template
  carrier work and should not be folded into idea 192 closure.

## Suggested Next

Execute Step 5 by running or recording the supervisor-selected focused/broader
validation and preparing closure evidence for idea 192.

## Watchouts

Do not treat the address-of consteval diagnostic path as a blocker for idea 192
closure; it is diagnostic/no-metadata compatibility, not semantic call-lowering
authority. The likely follow-up should focus on `PendingConstevalExpr` and
recursive `evaluate_consteval_call` identity, not broad template-engine
redesign.

## Proof

No new command was required for this ledger-only packet. Existing Step 2
proof remains the relevant proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$|^frontend_hir_lookup_tests$' > test_after.log`

Result: PASS, `2/2` tests passed. Proof log: `test_after.log`.

Supervisor focused regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS, before `2/2`, after `2/2`, no new failures.
