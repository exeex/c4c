Status: Active
Source Idea Path: ideas/open/164_sema_type_utils_static_eval_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Collision Coverage

# Current Packet

## Just Finished

Completed `plan.md` Step 5 collision coverage slice: added
`test_static_eval_int_keeps_same_spelled_enum_domains_distinct` to
`tests/frontend/cpp_hir_sema_consteval_type_utils_metadata_test.cpp`.

The new test uses two enum constants with the same rendered spelling and
`TextId` base name in distinct structured qualifier domains, verifies that each
domain resolves to its own value, and verifies that a structured miss does not
recover through the shared TextId or rendered spelling fallback.

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

Have the supervisor decide whether Step 5 coverage is sufficient or whether
additional collision coverage is needed for local/block enum-scope mirrors.

## Watchouts

- This packet changed only the focused metadata unit test and `todo.md`.
- The new collision case intentionally supplies wrong TextId/rendered fallback
  values so a collapse to base spelling would fail the test.
- Local/block enum-scope conversion remains outside this packet.

## Proof

Ran the supervisor-selected proof exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_sema_consteval_type_utils_structured_metadata$' > test_after.log`

Result: passed. `test_after.log` contains 1/1 passing tests.
