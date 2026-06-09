Status: Active
Source Idea Path: ideas/open/132_aarch64_dispatch_lookup_thin_helper_surface_trim.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build Lookup Surface Inventory

# Current Packet

## Just Finished

Step 1: Build Lookup Surface Inventory completed for `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`.

`dispatch_lookup.hpp` public declaration inventory:

| Declaration | Definition | Direct caller evidence | Include users | Classification |
| --- | --- | --- | --- | --- |
| `make_named_prepared_result_register(const module::BlockLoweringContext&, const bir::Value&)` | `dispatch_lookup.cpp:77`; AST `list-symbols` confirms non-static namespace definition. | Raw call sites: `memory_store_retargeting.cpp:127`, `globals.cpp:929`, `globals.cpp:1147`, `comparison.cpp:1979`, `alu.cpp:3837`, `memory.cpp:2514`, `memory.cpp:4534`, `calls.cpp:6149`, `calls.cpp:6348`, `calls.cpp:6362`, `calls.cpp:6377`, `calls.cpp:6429`. AST caller checks confirmed callers in `memory_store_retargeting.cpp:116`, `globals.cpp:900`, `globals.cpp:1138`, `comparison.cpp:1887`, `alu.cpp:3815`, `calls.cpp:6125`, and `calls.cpp:6304`; `memory.cpp` raw evidence includes one class-member caller and one free-function/lambda-shaped caller. | Included by `memory_store_retargeting.cpp:4`, `globals.cpp:5`, `comparison.cpp:2`, `memory.cpp:10`, `alu.cpp:6`, `calls.cpp:4`, and `dispatch_lookup.cpp:1`. | Retained: real cross-file hook used by six non-dispatch lowering files. |
| `emitted_scalar_value_available(const module::BlockLoweringContext&, const bir::Value&, const BlockScalarLoweringState&)` | `dispatch_lookup.cpp:120`; AST `list-symbols` confirms non-static namespace definition. | Raw call sites: `calls.cpp:6341`, `calls.cpp:6420`. AST caller check confirmed `calls.cpp:6304` (`materialize_scalar_call_argument_value`). | Same current header include set; only `calls.cpp` uses this declaration directly. | Retained: real cross-file hook consumed by calls lowering. |
| `is_scalar_call_argument_producer_opcode(bir::BinaryOpcode)` | `dispatch_lookup.cpp:27`; AST `list-symbols` confirms non-static namespace definition. | Whole-repo `rg` found only declaration, definition, `plan.md`, `todo.md`, and historical idea text; no active `src/` caller. `function-callers` in `dispatch_lookup.cpp` reports target not called in this translation unit. | Header is included by the six consumer files plus `dispatch_lookup.cpp`, but no active consumer references this name. | Removal candidate: no direct caller evidence; remove declaration and dead definition if Step 2 confirms no generated/indirect dependency. |
| `find_same_block_scalar_producer(const module::BlockLoweringContext&, const bir::Value&, std::size_t)` | `dispatch_lookup.cpp:129`; AST `list-symbols` confirms non-static namespace definition. | Whole-repo `rg` found only declaration, definition, one closed-idea historical note, `plan.md`, and no active `src/` caller. `function-callers` in `dispatch_lookup.cpp` reports target not called in this translation unit. | Header is included by the six consumer files plus `dispatch_lookup.cpp`, but no active consumer references this name. | Removal candidate: stale same-block lookup surface with no active direct caller; deletion may also strand `prepared_call_argument_source_producer_materialization`, which is otherwise only used by this definition. |
| `has_same_block_load_local_producer(const module::BlockLoweringContext&, const bir::Value&, std::size_t)` | `dispatch_lookup.cpp:145`; AST `list-symbols` confirms non-static namespace definition. | Whole-repo `rg` found only declaration and definition in active source; no active caller. No local caller appears in `dispatch_lookup.cpp`. | Header is included by the six consumer files plus `dispatch_lookup.cpp`, but no active consumer references this name. | Removal candidate: stale same-block load-local lookup surface with no active direct caller. |

Inventory notes:
- `dispatch_lookup.hpp` declares only the five public functions above; no public types are declared there.
- No public declaration currently has local-only call evidence that would make privatization alone the obvious Step 2 action.
- No clearer owner was proven in this evidence-only packet, so no declaration is classified as a rehome candidate yet.
- The consumer include set is `alu.cpp`, `calls.cpp`, `comparison.cpp`, `globals.cpp`, `memory.cpp`, and `memory_store_retargeting.cpp`; `dispatch_lookup.cpp` includes its own header for definitions.

## Suggested Next

Execute Step 2 from `plan.md`: remove the three no-direct-caller lookup declarations from `dispatch_lookup.hpp` and their dead definitions from `dispatch_lookup.cpp`, preserving `make_named_prepared_result_register` and `emitted_scalar_value_available` as public hooks.

## Watchouts

- Keep `make_named_prepared_result_register` and `emitted_scalar_value_available` public unless a same-slice replacement owner is proven and implemented.
- `prepared_call_argument_source_producer_materialization` appears to become dead if `find_same_block_scalar_producer` is removed; Step 2 should confirm and remove or privatize consistently in the same slice.
- Do not turn file-count reduction into the acceptance argument.
- Do not reintroduce BIR-name rescans, same-block named-case matching, or weakened backend expectations.

## Proof

Evidence-only packet per delegated proof. No implementation files were edited and no build/tests were run. AST/source evidence commands used:

- `c4c-clang-tool function-signatures src/backend/mir/aarch64/codegen/dispatch_lookup.hpp -- --std=c++20 -I/workspaces/c4c/src` returned the five public declarations, with a non-blocking transitive include diagnostic for `ast.hpp`.
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_lookup.cpp build/compile_commands.json` confirmed the five corresponding definitions.
- `rg` over active source and whole repo recorded direct call sites and include users.
- Targeted `c4c-clang-tool-ccdb function-callers` checks confirmed retained-hook callers in `calls.cpp`, `globals.cpp`, `alu.cpp`, `comparison.cpp`, `memory_store_retargeting.cpp`, and no local callers inside `dispatch_lookup.cpp`.

No `test_after.log` was produced because the delegated packet explicitly required no build/test proof for this inventory-only slice.
