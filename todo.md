Status: Active
Source Idea Path: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Extend Structured Lookup Proof Across HIR Edge Paths

# Current Packet

## Just Finished

Completed Step 4 direct-call proof extension. `frontend_hir_lookup_tests` now
covers a namespace-qualified `Module::resolve_direct_call_callee()` disagreement
where `DeclRef::NamespaceQualifier` plus `TextId` resolve the structured callee
even when the rendered name points at a different legacy-index function.

## Suggested Next

Next coherent Step 4 packet: choose another HIR edge path with existing
structured metadata, such as member/method owner keys or link-name-backed method
return lookup, and add the same stale-rendered-name authority proof without
changing resolver implementation.

## Watchouts

Fallback behavior remains covered by the existing declaration, operator, and
range-for cases; this packet only added a new structured-authority edge. Keep
future Step 4 tests focused on paths where structured metadata already exists,
and split missing metadata propagation into a separate idea instead of expanding
this runbook.

## Proof

Supervisor-selected proof ran exactly:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1
```

Result: passed. `test_after.log` contains `frontend_hir_lookup_tests`: 1 passed
/ 0 failed after rebuilding `frontend_hir_lookup_tests`.
