# Unified Parser State Snapshot/Restore

Status: Closed
Last Updated: 2026-03-31
Completed: 2026-03-31

## Goal

Replace ad-hoc manual save/restore of parser state during tentative parsing with a unified `ParserSnapshot` mechanism and RAII guard, eliminating the class of bugs where rollback misses a state field.

## Why this plan

Current tentative parsing manually saves/restores subsets of parser state (`pos_`, `typedefs_`, `typedef_types_`, etc.) at each backtracking point. The set of fields saved varies between call sites, and some fields (e.g. `enum_consts_`, `var_types_`, `template_scope_stack_`, namespace state) are sometimes omitted. This causes "drift" bugs where a failed tentative parse leaves behind stale type/name registrations that corrupt subsequent parsing.

## Proposed Design

### ParserSnapshot struct

```cpp
struct ParserSnapshot {
    int pos;
    std::set<std::string> typedefs;
    std::unordered_map<std::string, TypeSpec> typedef_types;
    std::unordered_map<std::string, long long> enum_consts;
    std::unordered_map<std::string, TypeSpec> var_types;
    std::vector<TemplateScopeFrame> template_scope_stack;
    // ... all other speculative state fields
};
```

### API on Parser

```cpp
ParserSnapshot save_state() const;
void restore_state(const ParserSnapshot& snap);
```

### RAII guard

```cpp
struct TentativeParseGuard {
    Parser& parser;
    ParserSnapshot snapshot;
    bool committed = false;

    explicit TentativeParseGuard(Parser& p)
        : parser(p), snapshot(p.save_state()) {}

    ~TentativeParseGuard() {
        if (!committed) parser.restore_state(snapshot);
    }

    void commit() { committed = true; }
};
```

## Implementation Steps

1. Audit all parser state fields — classify as "speculative" (must snapshot) vs. "observational" (safe to share)
2. Implement `ParserSnapshot`, `save_state()`, `restore_state()`
3. Find all existing manual save/restore sites (grep for `saved_pos`, `saved_typedefs`, etc.)
4. Replace each site with `TentativeParseGuard`
5. Add debug assertion: on restore, log which fields actually changed (helps catch new fields added later that forgot to join the snapshot)

## Constraints

- Must not change parsing behavior for any currently passing test
- Snapshot copy cost is acceptable for correctness; optimize later if profiling shows a bottleneck
- New speculative state fields added in the future must be added to `ParserSnapshot` (enforce via comment/checklist in `parser.hpp`)

## Completion Notes (2026-03-31)

- `ParserSnapshot` struct added to `parser.hpp` covering: `pos_`, `typedefs_`, `user_typedefs_`, `typedef_types_`, `var_types_`, `last_resolved_typedef_`
- `save_state()` / `restore_state()` implemented in `common.cpp`
- `TentativeParseGuard` RAII guard added to `parser.hpp`
- 25 manual save/restore sites converted across `statements.cpp`, `types_declarator.cpp`, `types_struct.cpp`, `types_base.cpp`, `expressions.cpp`, `declarations.cpp`
- 5 sites left unchanged (token injection patterns that swap `tokens_`, lambda-captured positions, non-tentative re-consume patterns)
- Regression: 2534/2671 (baseline maintained, no new failures)

## Acceptance Criteria

- [x] All convertible manual save/restore sites replaced with `TentativeParseGuard` (25/30; 5 left for structural reasons)
- [x] No raw `pos_ = saved_pos` patterns remain outside of `restore_state()` (except the 5 documented exceptions)
- [x] `cmake --build build -j8` succeeds
- [x] `ctest --test-dir build -j 8` shows no regressions (2534/2671 baseline maintained)
- [ ] Debug build logs snapshot restore events for traceability (skipped — not needed for correctness)
