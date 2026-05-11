Status: Active
Source Idea Path: ideas/open/164_sema_type_utils_static_eval_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Proof and Closure Readiness

# Current Packet

## Just Finished

Completed `plan.md` Step 6 final proof and closure-readiness ledger for idea
164.

Closure ledger:

- Structured enum lookup input exists for `static_eval_int`, with structured
  key lookup and TextId lookup taking authority over rendered-name lookup when
  metadata is complete.
- Covered callers converted: non-template `static constexpr` struct member
  initializer evaluation in `src/frontend/hir/hir_types.cpp`, static member
  declaration initializer fallback in
  `src/frontend/hir/impl/expr/scalar_control.cpp`, and
  `Lowerer::eval_const_int_with_nttp_bindings` including recursive evaluation
  and both NTTP-backed static member evaluation call sites.
- Step 5 collision coverage exists in
  `tests/frontend/cpp_hir_sema_consteval_type_utils_metadata_test.cpp` via
  `test_static_eval_int_keeps_same_spelled_enum_domains_distinct`, which proves
  same-spelled enum constants in distinct structured domains resolve to
  distinct values and do not collapse through shared TextId or rendered
  spelling.
- Retained rendered compatibility boundaries: the legacy
  `static_eval_int(Node*, const std::unordered_map<std::string, long long>&)`
  overload remains the no-metadata bridge, and the NTTP evaluator keeps
  rendered enum lookup only when no structured/TextId carrier can be built for
  the queried enum node.
- Removal conditions: remove rendered enum compatibility only after all
  producers/callers provide structured or TextId enum metadata, including any
  local/block enum-scope mirrors that currently depend on mutable
  `enum_consts_` save/restore behavior.
- Residual blocker: none for the covered Step 4/5/6 scope. Local/block
  enum-scope structured mirrors are a separate plan-owner decision if the
  source idea should be extended beyond the covered global/static-member paths.

## Step 1 Inventory Ledger

Inventory/proof packet completed before implementation; no build or test proof
was required and no `test_after.log` was written for that inventory-only slice.

- Declaration: `src/frontend/sema/type_utils.hpp`
  `static_eval_int(Node*, const std::unordered_map<std::string, long long>&)`.
  Classification before this slice: rendered-compatibility only; the public API
  could not express enum-domain identity or fail-closed structured misses.
- Definition: `src/frontend/sema/type_utils.cpp` `static_eval_int`; the
  `NK_VAR` branch resolved enum constants through `enum_consts.find(n->name)`.
  Classification before this slice: implementation target for adding
  structured/TextId-aware lookup input while retaining the string map as the
  no-metadata bridge.
- Internal recursive calls in `static_eval_int`: casts, unary operators, and
  binary operators only propagated the evaluator input. Classification: not
  separate enum lookup authorities; the `NK_VAR` branch owns enum resolution.
- Wrapper/parallel evaluator:
  `src/frontend/hir/hir_types.cpp`
  `Lowerer::eval_const_int_with_nttp_bindings`. Classification: TextId-capable
  for NTTP bindings through `NttpTextBindings` and
  `Node::unqualified_text_id`; converted in the current Step 4 packet so enum
  constants use structured TextId-based keys when complete.
- First selected external caller:
  `src/frontend/hir/hir_types.cpp` non-template `static constexpr` struct
  member evaluation during struct layout. Classification: structured-metadata
  capable for global enum constants through `ct_state_` and lowerer structured
  map helpers; converted in the current implementation packet.
- Remaining external caller target:
  `src/frontend/hir/impl/expr/scalar_control.cpp` static member declaration
  initializer fallback while lowering a member reference. Classification:
  structured-metadata capable for global enum constants in the surrounding
  lowerer context; converted in the current Step 4 packet.

## Suggested Next

Have the supervisor call the plan owner to decide whether idea 164 can close or
whether local/block enum-scope structured mirrors should split into a separate
idea.

## Watchouts

- This Step 6 packet changed only `todo.md` and refreshed `test_after.log`.
- No implementation or test files were touched during final proof.
- Local/block enum-scope conversion remains outside the covered closure scope
  unless the plan owner chooses to split or extend the work.

## Proof

Ran the supervisor-selected proof exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_expr_scalar_control_helper|cpp_hir_template_deferred_nttp_static_member_expr|cpp_hir_template_deferred_nttp_cast_static_member_expr|cpp_hir_template_alias_deferred_nttp_static_member|cpp_positive_sema_template_constexpr_member_runtime_cpp)$' > test_after.log`

Result: passed. `test_after.log` contains 7/7 passing tests.
