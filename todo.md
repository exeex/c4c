Status: Active
Source Idea Path: ideas/open/164_sema_type_utils_static_eval_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 2/3/4
Current Step Title: Add structured enum lookup input, prefer structured lookup/fail closed for covered complete metadata, and convert the first covered caller

# Current Packet

## Just Finished

Completed `plan.md` Steps 2/3/4 first slice: added
`StaticEvalIntEnumLookupInput` for `static_eval_int`, including structured key
and TextId enum maps plus the rendered string bridge; updated recursive
evaluation to propagate that input; and made covered structured/TextId misses
return closed instead of falling through to `enum_consts.find(n->name)`.

Converted the non-template `static constexpr` struct member initializer path in
`src/frontend/hir/hir_types.cpp` to build lowerer structured enum maps from
`ct_state_` and call the new `static_eval_int` input. The legacy
`static_eval_int(Node*, const std::unordered_map<std::string, long long>&)`
overload remains as the documented no-metadata compatibility bridge.

Added focused coverage in
`tests/frontend/cpp_hir_sema_consteval_type_utils_metadata_test.cpp` for
structured enum preference over stale rendered names, structured miss
fail-closed behavior, and TextId enum lookup/miss behavior.

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
  lowerer context, but still not converted in this packet.

## Suggested Next

Convert the next covered caller, likely the static member declaration
initializer fallback in `src/frontend/hir/impl/expr/scalar_control.cpp`, to the
new input while preserving NTTP-specific behavior in
`Lowerer::eval_const_int_with_nttp_bindings`.

## Watchouts

- The new `static_eval_int` input treats any provided structured/TextId map as
  authoritative when the queried node has matching metadata; only provide those
  maps for domains the caller intends to cover.
- `Lowerer::eval_const_int_with_nttp_bindings` still has its own enum rendered
  fallback and was intentionally not changed in this packet.
- Local/block enum-scope conversion still needs a separate packet because
  `enum_consts_` has mutable save/restore behavior while `ct_state_` mirrors
  structured global enum constants.

## Proof

Ran the supervisor-selected proof exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_positive_sema_template_constexpr_member_runtime_cpp)$' > test_after.log`

Result: passed. `test_after.log` contains 3/3 passing tests.
