# `stmt_emitter.cpp` Split Refactor

Status: Open
Last Updated: 2026-04-01

## Goal

Split `src/codegen/lir/stmt_emitter.cpp` (5,077 lines) into multiple focused implementation files while keeping `src/codegen/lir/stmt_emitter.hpp` as the single declaration surface for `StmtEmitter`.

## Why this plan

`src/codegen/lir/stmt_emitter.cpp` is carrying several distinct responsibilities in one translation unit:

- block / terminator emission
- string/global/type helpers
- lvalue lowering
- rvalue type inference
- expression payload lowering
- call / builtin / vararg lowering
- statement lowering

The header already provides a useful "one place to see everything" overview. The real problem is implementation density: contributors must load unrelated emission paths together, and AI agents have to scan a large monolith to edit one narrow lowering family.

## Refactor Objective

Adopt the same high-level shape proposed for `ast_to_hir`:

- keep `stmt_emitter.hpp` as the canonical declaration surface
- split `stmt_emitter.cpp` into multiple `stmt_emitter_*.cpp` files by responsibility
- preserve `StmtEmitter` as the owning class for the lowering API
- keep behavior unchanged; this is a structural refactor only

## Header Strategy

Do **not** split `stmt_emitter.hpp` as the first move.

The current header is large, but it is still valuable because it lets an agent inspect one file and understand:

- which helper/model structs exist
- what the `StmtEmitter` surface area is
- how responsibilities are grouped today

If cross-file private helpers become noisy during the split, introduce one small private helper header such as `stmt_emitter_internal.hpp`, but keep `stmt_emitter.hpp` as the primary summary view.

## Proposed Split

| New File | Content | Estimated Lines |
|----------|---------|-----------------|
| `stmt_emitter_core.cpp` | constructor, module wiring, block/terminator helpers, temp/label generation, string pool, global/object lookup, core utility helpers | ~500 |
| `stmt_emitter_types.cpp` | coercion, boolean conversion, field/type helpers, field-chain lookup, GEP/type-shape helpers, expression type resolution | ~1000 |
| `stmt_emitter_lvalue.cpp` | lvalue lowering, member/index address formation, assignable lvalue helpers, bitfield load/store, assignment helpers | ~900 |
| `stmt_emitter_call.cpp` | call target resolution, argument preparation, builtin lowering, direct/indirect calls, variadic lowering, ABI-specific vararg helpers | ~1400 |
| `stmt_emitter_expr.cpp` | `emit_rval_expr`, `emit_rval_id`, literal/declref/unary/binary/cast/ternary/member/index/sizeof payload emitters | ~900 |
| `stmt_emitter_stmt.cpp` | `emit_stmt`, per-statement dispatch, non-control-flow statements, control-flow statements, switch-label handling | ~500 |

Exact ownership can shift slightly during execution, but the split should follow semantic families rather than equal line counts.

## Preferred Extraction Order

1. Extract `stmt_emitter_stmt.cpp`
2. Extract `stmt_emitter_call.cpp`
3. Extract `stmt_emitter_lvalue.cpp`
4. Extract `stmt_emitter_expr.cpp`
5. Extract `stmt_emitter_types.cpp`
6. Reduce or remove the original `stmt_emitter.cpp`

This order front-loads the cleanest seams first and avoids starting with the most cross-coupled expression/type helpers.

## Constraints

- Pure structural refactor: no intended logic changes
- `stmt_emitter.hpp` remains the single public declaration file
- All new implementation files must be added to the LIR/codegen build rules
- Preserve current lowering order and emitted instruction behavior as much as possible
- Keep private helper free functions file-local unless cross-file sharing is clearly justified
- Prefer one optional internal helper header over proliferating many tiny headers
- Existing tests must pass unchanged after the split

## Acceptance Criteria

- [ ] `src/codegen/lir/stmt_emitter.hpp` remains the canonical declaration surface for `StmtEmitter`
- [ ] `src/codegen/lir/stmt_emitter.cpp` is removed or reduced to a thin forwarding/coordinator file
- [ ] statement emission logic lives in a dedicated `stmt_emitter_stmt.cpp`
- [ ] call / builtin / vararg lowering lives in a dedicated `stmt_emitter_call.cpp`
- [ ] lvalue and expression lowering are moved into focused implementation files
- [ ] `cmake --build build -j8` succeeds
- [ ] `ctest --test-dir build -j 8` shows no regressions

## Guardrails

- Do not mix semantic fixes into the split unless needed to preserve pre-refactor behavior
- Avoid re-grouping methods in the header unless needed to keep declarations readable
- Keep platform ABI helpers close to the call/vararg implementation instead of scattering them
- If a helper is only used by one implementation file, keep it local to that file

## Good First Patch

Move statement-dispatch and statement-kind implementations into `stmt_emitter_stmt.cpp` while leaving expression and call helpers in place. This yields a meaningful size reduction with relatively low coupling risk.
