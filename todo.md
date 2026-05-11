Status: Active
Source Idea Path: ideas/open/164_sema_type_utils_static_eval_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert Covered Callers

# Current Packet

## Just Finished

Completed `plan.md` Step 4 NTTP evaluator slice: converted
`Lowerer::eval_const_int_with_nttp_bindings` so enum constants use a structured
`ConstEvalStructuredNameKey` map when the queried node has complete metadata,
returning closed on structured misses instead of falling through to rendered
enum lookup.

Preserved NTTP binding priority: the evaluator still checks
`NttpTextBindings` first and explicit rendered NTTP bindings second, before
enum lookup. Both NTTP-backed static member evaluation sites now pass refreshed
lowerer structured enum maps through the evaluator and its recursive calls.

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

Have the supervisor decide whether Step 4 is complete or whether local/block
enum-scope structured mirrors need a separate plan-review packet.

## Watchouts

- `Lowerer::eval_const_int_with_nttp_bindings` still allows rendered enum
  lookup only when no structured/TextId carrier can be built for the queried
  node; complete structured misses fail closed.
- Explicit rendered NTTP bindings remain compatibility authority after
  `NttpTextBindings`, as required by the packet.
- Local/block enum-scope conversion still needs a separate packet if the plan
  wants to cover mutable `enum_consts_` save/restore behavior beyond the global
  structured metadata mirrored by `ct_state_`.

## Proof

Ran the supervisor-selected proof exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_template_deferred_nttp_static_member_expr|cpp_hir_template_deferred_nttp_cast_static_member_expr|cpp_hir_template_alias_deferred_nttp_static_member|cpp_positive_sema_template_constexpr_member_runtime_cpp)$' > test_after.log`

Result: passed. `test_after.log` contains 6/6 passing tests.
