# Plan Execution State

## Baseline
- Test suite: **1798/1798** (2026-03-16, after Phase B)

## Completed
- Phase A: Lock Current Behavior With Tests — existing tests cover fn-ptr return cases; XFAIL guard exists for nested raw syntax
- Phase B: Fill `ResolvedTypeTable` For Locals — `record_local_decl_types()` recursively walks function bodies and records canonical types for all NK_DECL nodes

## Active Item
- **Phase C: Add Expression Canonical Types**

## Next Slice
- Extend sema/type resolution to annotate expression nodes with canonical type
- Thread expression canonical types to lowering/codegen consumers
- Indirect calls through expression forms can read canonical callable signatures directly

## Blockers
- None

## Deferred
- Phase D: Legacy fallback removal
- Cleanup item 1: Parser gap for nested fn-returning-fn-ptr declarators
