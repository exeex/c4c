Status: Active
Source Idea Path: ideas/open/164_sema_type_utils_static_eval_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Covered Callers

# Current Packet

## Just Finished

Completed `plan.md` Step 4 remaining covered-caller slice: converted the
static member declaration initializer fallback in
`src/frontend/hir/impl/expr/scalar_control.cpp` from the rendered
`enum_consts_` overload to `StaticEvalIntEnumLookupInput` populated from
lowerer structured enum maps.

Converted covered `static_eval_int` callers now consist of the non-template
`static constexpr` struct member initializer path in
`src/frontend/hir/hir_types.cpp` and the static member declaration initializer
fallback in `src/frontend/hir/impl/expr/scalar_control.cpp`. Remaining
rendered lookup is the no-metadata compatibility bridge overload plus the
intentional NTTP-specific rendered fallback in
`Lowerer::eval_const_int_with_nttp_bindings`.

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
  `Node::unqualified_text_id`, but rendered-compatibility only for enum
  constants today.
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

Have the supervisor decide whether Step 4 is complete or whether the
NTTP-specific compatibility fallback needs a separate plan-review packet.

## Watchouts

- `static_eval_int` now has no remaining HIR caller using the rendered-map
  overload except the compatibility bridge itself; `rg "static_eval_int\\("`
  shows only the two structured HIR callers and recursive evaluator calls.
- `Lowerer::eval_const_int_with_nttp_bindings` still has its own enum rendered
  fallback and was intentionally not changed in this packet.
- Local/block enum-scope conversion still needs a separate packet if the plan
  wants to cover mutable `enum_consts_` save/restore behavior beyond the global
  structured metadata mirrored by `ct_state_`.

## Proof

Ran the supervisor-selected proof exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_expr_scalar_control_helper|cpp_positive_sema_template_constexpr_member_runtime_cpp)$' > test_after.log`

Result: passed. `test_after.log` contains 4/4 passing tests.
