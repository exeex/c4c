# Plan Execution State

## Baseline
- 1862/1862 tests passing (2026-03-17)

## Overall Assessment

Phase 7 slice 1 is now implemented: specialization identity with hashing, O(1) dedup, and LLVM IR metadata.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass**
- **Truly deferred consteval is implemented**: consteval calls during deferred template instantiation are NOT evaluated eagerly during lowering. Instead, they create `PendingConstevalExpr` HIR nodes that the compile-time pass evaluates in a separate step.
- **Phase 6 materialization boundary**: consteval functions are lowered to HIR as declaration-only entities marked `consteval_only`, and the materialization pass excludes them from code emission. Template instantiations track their `template_origin` for provenance.
- **Phase 7 slice 1**: SpecializationKey has hash support (`SpecializationKeyHash`), `has_instance()` uses O(1) lookup via `instance_keys_` set, `spec_key` is set on `Function` during both eager and deferred lowering, LLVM IR emits `; spec-key:` and `; template-origin:` comments on instantiated functions.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **slice 1 complete** — specialization identity is stable, hashable, and emitted in LLVM IR

---

## What was done in this session (2026-03-17, session 5)

### Phase 7 slice 1: Specialization identity hashing, O(1) dedup, and LLVM metadata

**Goal**: Make specialization identity stable, hashable, and visible in emitted LLVM IR.

**Implementation changes**:

1. **`ir.hpp`**: Moved `SpecializationKey` and added `SpecializationKeyHash` before `Function` struct. Added `spec_key` field to `Function`.

2. **`ast_to_hir.cpp`**:
   - Added `spec_key` to `TemplateInstance` struct
   - Added `instance_keys_` (`unordered_set<string>`) for O(1) dedup in `has_instance()`
   - `record_instance()` now builds and stores `SpecializationKey` per instantiation
   - `spec_key` propagated to `Function` during both eager (Phase 2) and deferred (Phase 3) lowering

3. **`hir_emitter.cpp`**: Template-instantiated functions now emit `; template-origin:` and `; spec-key:` comments before the function definition in LLVM IR output.

4. **`hir_printer.cpp`**: Functions now show `key=...` annotation when spec_key is present.

### Test additions
- **`specialization_identity.cpp`**: Test case with 2 templates × multiple instantiations (add<int>, add<double>, mul<int>)
- **`cpp_hir_spec_key_identity`**: HIR test verifying `key=add<T=int>` appears
- **`cpp_hir_spec_key_distinct`**: HIR test verifying `key=add<T=double>` appears (distinct from int)
- **`cpp_llvm_spec_key_metadata`**: LLVM IR test verifying `spec-key: add<T=int>` appears
- **`cpp_positive_sema_specialization_identity_cpp`**: Runtime correctness test (auto-discovered)

### Exit criteria progress (Phase 7)
- [x] Define a stable specialization key — `SpecializationKey` struct with canonical string format
- [x] Include canonicalized template args in the key — `make_specialization_key()` uses `canonical_type_str()`
- [x] Ensure identity is deterministic across TUs — params sorted by declaration order, type strings canonical
- [ ] Keep encoding serializable for future link-time/JIT use — key is a string, but no cross-TU serialization yet

### Files changed
- `src/frontend/hir/ir.hpp` — moved SpecializationKey before Function, added SpecializationKeyHash, added spec_key to Function
- `src/frontend/hir/ast_to_hir.cpp` — O(1) dedup, spec_key on TemplateInstance and Function
- `src/codegen/llvm/hir_emitter.cpp` — LLVM IR metadata comments for template origin and spec key
- `src/frontend/hir/hir_printer.cpp` — key= annotation on functions
- `tests/internal/InternalTests.cmake` — 3 new tests
- `tests/internal/cpp/postive_case/specialization_identity.cpp` — new test case

---

## Added Testcases

The work now includes 23 HIR-oriented regression tests:

Previous 19 + 4 new:
- **`cpp_hir_spec_key_identity`** (NEW — proves spec_key appears in HIR)
- **`cpp_hir_spec_key_distinct`** (NEW — proves distinct instantiations have distinct keys)
- **`cpp_llvm_spec_key_metadata`** (NEW — proves spec-key emitted in LLVM IR)
- **`cpp_positive_sema_specialization_identity_cpp`** (NEW — runtime correctness)

---

## Recommended next milestone

Phase 7 slice 2: Cross-TU serialization format for specialization keys (e.g., LLVM named metadata or custom section), or begin Phase 5 remaining work (non-type template parameters).
