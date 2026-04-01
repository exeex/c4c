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
- use a draft-first extraction flow so multiple agents can stage candidate split files in parallel before the original monolith is cleaned up

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

## Preferred Execution Mode

Prefer the same workflow that is now proving faster for the `ast_to_hir` split:

1. Keep `src/codegen/lir/stmt_emitter.cpp`, `src/codegen/lir/stmt_emitter.hpp`, and build wiring untouched during the first draft pass.
2. Let multiple agents create unintegrated staging files such as `stmt_emitter_stmt.cpp`, `stmt_emitter_call.cpp`, `stmt_emitter_lvalue.cpp`, `stmt_emitter_expr.cpp`, `stmt_emitter_types.cpp`, and `stmt_emitter_core.cpp` in parallel.
3. Treat those first-pass files as ownership proposals, not finished integration work.
4. Review the staged files for overlap, duplicated helpers, and cross-file dependency direction before any build wiring is changed.
5. Once the first-pass boundaries look plausible, wire the staged files into the build earlier rather than trying to finish all ownership cleanup by inspection alone.
6. Use compiler and linker diagnostics as the main ownership-reconciliation tool:
   - compiler errors expose missing declaration surface and bad cross-file dependency assumptions
   - duplicate-definition linker errors expose which `StmtEmitter::...` bodies still need to be disabled or removed from `stmt_emitter.cpp`
   - undefined-reference linker errors expose helpers that were disabled too aggressively or still have no split-file owner
7. Prefer batch ownership cleanup driven by those diagnostics over one-function-at-a-time manual peeling.
8. Reduce or remove the original `stmt_emitter.cpp` only after the staged files have converged and compile cleanly together.

This mode intentionally front-loads cheap parallel extraction and postpones the risky ownership cleanup until the target file boundaries are visible.

## Preferred Integration Order

Once the draft-only staging files exist and their boundaries have been reviewed:

1. Normalize overlap between draft files and assign one owner per helper cluster.
2. Wire several draft files into the build as soon as their boundaries are coherent enough to generate useful diagnostics.
3. Let compiler/linker output produce the first concrete ownership-removal list instead of trying to predict every overlap manually.
4. Batch-disable or batch-delete the matching bodies from `stmt_emitter.cpp` by semantic cluster.
5. Rebuild and repeat until the duplicate-definition list collapses to the next unresolved cluster.
6. Remove or reduce the monolith only after the staged files have replaced it cleanly.

This keeps the parallel speedup from the draft pass while preserving a conservative integration path.

## Constraints

- Pure structural refactor: no intended logic changes
- `stmt_emitter.hpp` remains the single public declaration file
- All new implementation files must be added to the LIR/codegen build rules
- Preserve current lowering order and emitted instruction behavior as much as possible
- Keep private helper free functions file-local unless cross-file sharing is clearly justified
- Prefer one optional internal helper header over proliferating many tiny headers
- During the draft-only staging pass, do not edit `stmt_emitter.cpp` just to "keep up" with the extracted files; let the staged files settle first
- During the very first draft-only pass, do not change CMake or other build wiring yet
- After the first draft boundaries stabilize, prefer early build wiring plus diagnostic-driven cleanup over prolonged manual ownership review
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
- If parallel draft extraction creates duplicate helper ownership, resolve the duplication before wiring those files into the build instead of trying to force both versions forward
- Once files are wired into the build, treat duplicate-definition and undefined-reference errors as the fastest ownership map available; use them aggressively to drive batch cleanup
- Planning state should describe the current phase explicitly: draft-only extraction, overlap cleanup, build wiring, or monolith cleanup

## Good First Patch

Create unintegrated draft staging files for `stmt_emitter_stmt.cpp`, `stmt_emitter_call.cpp`, and `stmt_emitter_expr.cpp` in parallel while keeping the original `stmt_emitter.cpp` untouched. Then review overlaps before selecting the first file to wire into the build.
