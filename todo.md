Status: Active
Source Idea Path: ideas/open/467_unsupported_carrier_alias_planner_rejection.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Planner Rejection Packet

# Current Packet

## Just Finished

Completed Step 3 as a blocker-only routing packet for idea 467.

The uncommitted implementation/test attempt was rejected and removed from the
tracked source tree. The attempted `const_cast` name-table mutation is not
accepted progress, and the const-correct rewrite exposed a caller/API conflict
instead of a bounded acceptance-ready repair.

Blocker: carrier-alias name publication is semantically a mutation of
`PreparedNameTables`, but the current representative object route reaches the
authority collector through const prepared-module consumers. A scratch-copy
const collector can publish alias names for diagnostic records, but RV64 and
other original-module consumers then see a different name table and cannot
reliably match the same synthesized carrier alias identity.

The original prepared rejection remains classified:

| Row | Classification |
| --- | --- |
| `20010329-1` `%t46 -> %t50` | Two semantic carrier-alias candidates exist, but accepting them requires a const-correct shared identity publication design. |
| Prior `const_cast` implementation | Rejected; hidden mutation through `const PreparedNameTables&` is not acceptance-ready. |
| Const-correct scratch-copy attempt | Routed; fixes diagnostic prepared evidence in a copy but does not provide consistent identity to original-module consumers. |

Tracked implementation and test files from the attempt were restored. The
diagnostic artifacts are preserved under
`build/agent_state/467_step3_carrier_alias_acceptance/`.

## Suggested Next

Route a follow-up lifecycle decision before any implementation:

- either select an explicit mutable carrier-alias name-publication stage that
  runs before RV64/object consumers receive the prepared module; or
- select a consumer-side API design that matches authorized carrier aliases
  without requiring scratch-copy names to exist in the original prepared name
  table.

Do not resume the rejected `const_cast` approach.

## Watchouts

- No implementation/test changes remain from Step 3.
- Do not treat unavailable carrier-alias evidence rows as authority.
- Do not infer aliases from `%*.phi.sel*` spelling, testcase names, dump order,
  value ids, block labels, or raw shape alone.
- Do not make RV64 lowering changes in this idea without a separately owned
  packet.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 3 blocker-only proof:

```sh
git diff --check
```

Result: passed.
