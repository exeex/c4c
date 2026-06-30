Status: Active
Source Idea Path: ideas/open/427_rv64_scalar_select_join_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Generalize To Join-Transfer Carrier Rows

# Current Packet

## Just Finished

Step 3 implemented the next coherent prepared join-transfer carrier case:
published select-carrier edge-copy bundles whose carrier result is stack-homed
now skip predecessor publication moves and fall through to the existing
join-block scalar select materialization path. Register-homed published
carriers still use the existing predecessor-copy route, while non-stack,
missing-home, cycle, malformed, or unsupported publication shapes remain
fail-closed.

AST-backed inspection was used before the edit. The direct select and
join-transfer helper queries identified the existing published edge-copy gate,
edge-source producer path, and `fragment_for_prepared_select` scalar
materialization path. Focused backend coverage now proves a stack-home
published join-transfer carrier distinct from the Step 2 ordinary select shape:
the object route emits the select in the join block, stores the selected value
to the prepared stack slot, reloads it for return, and keeps local select
relocations.

## Suggested Next

Execute Step 4: prove nearby same-feature coverage across representative
select/join rows, starting with another non-stack join-transfer or ordinary
select shape that still stays inside scalar select/join materialization.

## Watchouts

- Published join-transfer carriers with register destinations remain on the
  predecessor-copy path; the new fallback is only for stack-homed carrier
  results that can be materialized by the existing join select.
- The stack fallback intentionally requires simple predecessor terminator
  publication bundles with complete select-publication facts. Mixed, cycle, or
  non-select bundles should continue to reject instead of being silently
  broadened.
- Do not edit expectations, allowlists, unsupported markers, runtime comparison, or pass/fail accounting.
- Keep F128, call-adjacent publication, aggregate ABI, pointer provenance, and global memory repair out of this route.
- Reject filename-shaped fixes for representative gcc_torture rows.

## Proof

Ran the delegated proof command:
`(cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: passed, including the focused prepared-object stack-home published
join-transfer carrier assertion plus the existing ordinary select and
published-carrier tests. `test_after.log` is the canonical proof log for this
Step 3 code packet. `git diff --check` also passed.
