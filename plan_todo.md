# Plan Execution State

## Baseline
- 1864/1864 tests passing (2026-03-17)

## Overall Assessment

Phase 7 slice 2 is now implemented: LLVM named metadata for cross-TU specialization identity serialization.

Current state:
- HIR preserves template/consteval metadata and exposes debug visibility
- **Nested template instantiation is owned by the HIR compile-time reduction pass**
- **Truly deferred consteval is implemented**: consteval calls during deferred template instantiation are NOT evaluated eagerly during lowering. Instead, they create `PendingConstevalExpr` HIR nodes that the compile-time pass evaluates in a separate step.
- **Phase 6 materialization boundary**: consteval functions are lowered to HIR as declaration-only entities marked `consteval_only`, and the materialization pass excludes them from code emission. Template instantiations track their `template_origin` for provenance.
- **Phase 7 slice 1**: SpecializationKey has hash support (`SpecializationKeyHash`), `has_instance()` uses O(1) lookup via `instance_keys_` set, `spec_key` is set on `Function` during both eager and deferred lowering, LLVM IR emits `; spec-key:` and `; template-origin:` comments on instantiated functions.
- **Phase 7 slice 2**: Specialization keys emitted as LLVM named metadata (`!c4c.specializations`) for machine-readable cross-TU serialization. Each entry is `!{!"spec-key", !"template-origin", !"mangled-name"}`.

Summary:
- Phase 1: complete
- Phases 2-4: partially complete, with good observability and metadata preservation
- Phase 5: **substantially complete** — deferred template instantiation works; truly deferred consteval evaluation works via PendingConstevalExpr
- Phase 6: **complete** — materialization boundary separates compile-time entities from emitted code
- Phase 7: **complete** — specialization identity is stable, hashable, serializable via LLVM named metadata

---

## What was done in this session (2026-03-17, session 6)

### Phase 7 slice 2: LLVM named metadata for cross-TU specialization identity

**Goal**: Make specialization keys machine-readable via LLVM named metadata, enabling cross-TU dedup and future link-time/JIT reuse.

**Implementation changes**:

1. **`hir_emitter.hpp`**: Added `SpecEntry` struct and `spec_entries_` vector to collect specialization metadata during function emission.

2. **`hir_emitter.cpp`**:
   - During function emission, when a template instantiation has a spec_key, collect it into `spec_entries_`
   - At end of `emit()`, output LLVM named metadata:
     - Individual entries: `!N = !{!"spec-key", !"template-origin", !"mangled-name"}`
     - Named metadata node: `!c4c.specializations = !{!0, !1, ...}`

**Output example**:
```llvm
!0 = !{!"add<T=int>", !"add", !"add_i"}
!1 = !{!"add<T=double>", !"add", !"add_d"}
!2 = !{!"mul<T=int>", !"mul", !"mul_i"}
!c4c.specializations = !{!0, !1, !2}
```

### Test additions
- **`cpp_llvm_spec_key_named_metadata`**: Verifies `!c4c.specializations` appears in LLVM IR
- **`cpp_llvm_spec_key_named_metadata_entry`**: Verifies `!"add<T=int>"` appears as metadata string

### Exit criteria progress (Phase 7)
- [x] Define a stable specialization key — `SpecializationKey` struct with canonical string format
- [x] Include canonicalized template args in the key — `make_specialization_key()` uses `canonical_type_str()`
- [x] Ensure identity is deterministic across TUs — params sorted by declaration order, type strings canonical
- [x] Keep encoding serializable for future link-time/JIT use — LLVM named metadata `!c4c.specializations`

### Files changed
- `src/codegen/llvm/hir_emitter.hpp` — added SpecEntry struct and spec_entries_ vector
- `src/codegen/llvm/hir_emitter.cpp` — collect spec entries during emission, emit named metadata at module end
- `tests/internal/InternalTests.cmake` — 2 new tests

---

## Added Testcases

The work now includes 25 HIR-oriented regression tests:

Previous 23 + 2 new:
- **`cpp_llvm_spec_key_named_metadata`** (NEW — proves !c4c.specializations appears)
- **`cpp_llvm_spec_key_named_metadata_entry`** (NEW — proves metadata entry strings present)

---

## Recommended next milestone

Phase 7 is now complete. Potential next work:
- Phase 5 remaining: non-type template parameters
- Phase 2-4 remaining: deeper template/consteval HIR preservation
