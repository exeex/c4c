Status: Active
Source Idea Path: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Extend Structured Lookup Proof Across HIR Edge Paths

# Current Packet

## Just Finished

Completed Step 4 struct owner-key proof extension. `frontend_hir_lookup_tests`
now covers stale rendered struct member and method lookup maps disagreeing with
the structured owner-key mirrors, and asserts those owner-key parity paths record
mismatches instead of letting rendered spelling silently count as authoritative.

## Suggested Next

Next coherent Step 4 packet: either extend the same owner-key proof to another
existing mirror such as method link-name or return-type lookup, or ask for plan
review if the remaining Step 4 edge paths now look exhausted enough for
acceptance routing.

## Watchouts

The new test deliberately uses the existing owner-key parity mirrors and does
not change production lookup authority; it proves detection rather than a full
owner-key replacement. Future packets should keep that distinction explicit and
avoid weakening legacy fallback expectations.

## Proof

Supervisor-selected proof ran exactly:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1
```

Result: passed. `test_after.log` contains `frontend_hir_lookup_tests`: 1 passed
/ 0 failed after rebuilding `frontend_hir_lookup_tests`.
