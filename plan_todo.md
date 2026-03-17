# Plan Execution State

## Baseline
- 1870/1870 tests passing (2026-03-17)

## Overall Assessment

Phase 5 slice 7 is now implemented: template type parameter substitution in function signatures and local variable declarations.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass**
- **Truly deferred consteval is implemented**: consteval calls during deferred template instantiation are NOT evaluated eagerly during lowering. Instead, they create `PendingConstevalExpr` HIR nodes that the compile-time pass evaluates in a separate step.
- **Phase 6 materialization boundary**: consteval functions are lowered to HIR as declaration-only entities marked `consteval_only`, and the materialization pass excludes them from code emission. Template instantiations track their `template_origin` for provenance.
- **Phase 7**: Specialization identity is stable, hashable, serializable via LLVM named metadata.
- **Phase 5 slice 3**: Non-type template parameters (NTTP) — `template<int N>` etc. supported end-to-end.
- **Phase 5 slice 4**: Mixed type + NTTP params (`template<typename T, int N>`) and consteval NTTP functions supported end-to-end.
- **Phase 5 slice 5**: NTTP forwarding in deferred chains — `template<int N> f() { return g<N>(); }` where g is another template or consteval template.
- **Phase 5 slice 6**: Mixed type+NTTP forwarding in deferred chains works end-to-end.
- **Phase 5 slice 7 (NEW)**: Template type parameter substitution in function signatures and locals. Previously, `TB_TYPEDEF` template params fell through to `i32` in codegen (via `llvm_base()` default case). Now `lower_function()` correctly substitutes template type params in return types, parameter types, and local variable types. Pointer/array modifiers from declarators are preserved (only base+tag are substituted).

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr; NTTP support added; mixed type+NTTP params work; consteval with NTTP works; NTTP forwarding in deferred chains works; mixed type+NTTP forwarding works; **type parameter substitution in signatures and locals works**
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 11)

### Phase 5 slice 7: Template type parameter substitution in function signatures and locals

**Root cause**: `lower_function()` in `ast_to_hir.cpp` set `fn.return_type`, parameter types, and local variable types directly from AST `TypeSpec` without substituting template type bindings. Since `TB_TYPEDEF` falls through to `i32` in `llvm_base()` (the default case in the switch), template instantiations with non-int types (long, short, char) silently produced wrong LLVM types.

**Fix**: Three substitution points added in `ast_to_hir.cpp`:
1. **Return type** (line ~1146): Check if `fn_node->type.base == TB_TYPEDEF` and tag matches a template binding → substitute `base` and `tag` from the concrete type
2. **Parameter types** (line ~1215): Same check on each `p->type`
3. **Local variable types** (line ~2097): Same check on `n->type` using `ctx.tpl_bindings`

**Key design decision**: Only `base` and `tag` fields are substituted; declarator modifiers (`ptr_level`, `array_rank`, etc.) are preserved from the original TypeSpec. This ensures `T*` correctly becomes `long*` (not just `long`).

### Test additions
- **`template_type_subst.cpp`**: Tests type substitution across return types, parameters, locals, and pointer parameters with long, short, and char types. Verifies no truncation/overflow for 64-bit values.

### Files changed
- `src/frontend/hir/ast_to_hir.cpp` (3 substitution points)
- `tests/internal/cpp/postive_case/template_type_subst.cpp` (new)

---

## Recommended next milestone

Potential next work:
- `static_cast` support in consteval interpreter (currently fails for deferred consteval with static_cast)
- Template return type substitution for more complex types (struct, fn_ptr)
- Phase 2-4 remaining: deeper template/consteval HIR preservation
- Template class/struct support
