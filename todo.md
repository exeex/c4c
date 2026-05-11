Status: Active
Source Idea Path: ideas/open/164_sema_type_utils_static_eval_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Static Eval Callers

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory for `static_eval_int` without
implementation or test edits.

Inventory and classification:

- Declaration: `src/frontend/sema/type_utils.hpp:70`
  `long long static_eval_int(Node*, const std::unordered_map<std::string, long long>&)`.
  Classification: rendered-compatibility only. The public API accepts only a
  rendered string map and cannot express enum-domain identity or fail-closed
  structured misses.
- Definition: `src/frontend/sema/type_utils.cpp:1021`
  `static_eval_int`; enum constants are resolved by `enum_consts.find(n->name)`
  for `NK_VAR`.
  Classification: rendered-compatibility only today; implementation target for
  adding structured/TextId-aware lookup input while retaining this string map as
  the no-metadata bridge.
- Internal recursive calls: `src/frontend/sema/type_utils.cpp:1032`,
  `src/frontend/sema/type_utils.cpp:1067`,
  `src/frontend/sema/type_utils.cpp:1068`,
  `src/frontend/sema/type_utils.cpp:1069`,
  `src/frontend/sema/type_utils.cpp:1072`,
  `src/frontend/sema/type_utils.cpp:1073`.
  Classification: unrelated to enum constants as caller groups. They only
  propagate the evaluator input through casts, unary operators, and binary
  operators; enum lookup authority remains the `NK_VAR` branch.
- Wrapper/parallel evaluator:
  `src/frontend/hir/hir_types.cpp:465`
  `Lowerer::eval_const_int_with_nttp_bindings`.
  Classification: TextId capable for NTTP bindings via
  `NttpTextBindings` and `Node::unqualified_text_id`; rendered-compatibility
  only for enum constants today because it still falls back to
  `enum_consts_.find(n->name)`.
- External caller group:
  `src/frontend/hir/hir_types.cpp:2682`
  non-template `static constexpr` struct member evaluation during struct layout.
  Classification: structured-metadata capable for global enum constants. The
  lowerer owns `ct_state_` structured enum mirrors and
  `make_lowerer_consteval_env`/`LowererConstEvalStructuredMaps`; the initializer
  nodes carry `unqualified_text_id`/namespace metadata. It currently calls the
  rendered bridge when no NTTP bindings are present.
- External caller group:
  `src/frontend/hir/impl/expr/scalar_control.cpp:532`
  static member declaration initializer fallback while lowering a member
  reference.
  Classification: structured-metadata capable for global enum constants and
  TextId capable in the surrounding lowering context. The path has access to
  `ctx`, `module_`, `ct_state_`, and the same lowerer structured-map machinery,
  but the fallback call currently passes only `enum_consts_`.

No direct sema validation callers of `static_eval_int` were found by text
search. Sema validation already has separate structured enum value maps, so it
is not the first target for this runbook.

Narrow first implementation target: convert the non-template static constexpr
struct member initializer path in `src/frontend/hir/hir_types.cpp:2682` to a
structured-aware static integer evaluation input. This is the smallest direct
caller group: it is a single call site, it already sits beside
`eval_const_int_with_nttp_bindings` for the templated case, and the lowerer can
provide global structured enum mirrors without changing unrelated expression
lowering behavior.

## Suggested Next

Implement the structured lookup input for `static_eval_int`, then convert the
`src/frontend/hir/hir_types.cpp:2682` non-template static constexpr struct
member initializer call site first.

## Watchouts

- Do not treat raw `TextId` alone as sufficient when enum/domain scope matters.
- Do not let complete structured misses fall back to `enum_consts.find(n->name)`.
- Keep rendered string lookup only as an explicit no-metadata compatibility
  bridge.
- Do not reopen the full consteval interpreter or validate table ownership.
- `Lowerer::eval_const_int_with_nttp_bindings` already handles NTTP TextId
  bindings but not structured enum constants; avoid regressing that behavior
  when adding enum lookup metadata.
- `enum_consts_` has mutable save/restore behavior for block scopes in
  statement lowering, while `ct_state_` mirrors structured global enum
  constants. Local enum-scope conversion may need a separate packet after the
  global static-member initializer path.

## Proof

Inventory-only packet; no build or test proof required by the supervisor. No
`test_after.log` was written.
