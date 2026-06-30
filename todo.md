Status: Active
Source Idea Path: ideas/open/466_representative_select_carrier_alias_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Representative Authority Collection

# Current Packet

## Just Finished

Completed Step 1 audit for idea 466. The representative `20010329-1`
authority record is not probe-visible, and current evidence cannot soundly
distinguish not-produced from produced-but-hidden because the collector drops
unavailable statuses and the prepared printer has no carrier-alias authority
print hook. There is no concrete evidence yet for a produced
RV64-mismatched record.

Audit table:

| Question | Evidence | Classification |
| --- | --- | --- |
| Representative structural route | Prepared dump has `%t46 = bir.ule ptr %t42, %t45`, duplicate `%t50.phi.sel0/%t50.phi.sel1`, final `%t50`, and `logic.rhs.end.40 -> logic.end.41` edge transfer `%t46 -> %t50`. | Candidate shape exists; raw shape remains non-authoritative. |
| Operand consumability | `%t42` home `s1`, `%t45` home `s2`, `%t46/%t50` home `t0`. | Not the first blocker. |
| Prepared printed record | Fresh dump has no `carrier_alias` / `select_carrier` text. `prepared_printer.cpp` has no carrier-alias append hook. | Record is not probe-visible. |
| Collector behavior | `collect_prepared_select_carrier_alias_authorities` emits only available records and silently skips unavailable statuses. | Missing versus hidden cannot be resolved from dumps. |
| RV64 matcher | RV64 requires exact function, edge, destination, source, source producer kind/block/instruction match. | Mismatch remains possible but unproven without record fields. |
| Debugger probe | `build/c4cll` exposes non-debug symbols but not enough local type/debug info to inspect the returned record vector. | Non-editing runtime inspection was insufficient. |

Artifact:
`build/agent_state/466_step1_representative_authority_collection_audit/audit.md`.

## Suggested Next

Execute Step 2: define the representative authority evidence contract. The
next exact packet should require a focused collector/printer/probe surface that
reports available carrier-alias records and unavailable per-publication
statuses for the real-style `%t46 -> %t50` route, including function, edge,
destination/source ids and names, source producer kind/block/instruction,
carrier aliases, source-use closure, and rejection status.

## Watchouts

- Do not classify the representative as solved from raw duplicate-carrier
  select shape.
- Do not claim the record is missing or hidden until a focused evidence surface
  can observe record count and/or rejection status.
- Do not route back to RV64 consumer work unless the record is proven present
  with fields that RV64 mismatches.
- Do not make RV64 ULE rematerialization changes until representative
  authority is proven present and matchable.
- Do not infer aliases from `%*.phi.sel*` spelling, raw select shape, value
  ids, block labels, function names, testcase names, or dump order.
- Do not implement plain `%t46 -> %t50` copies or same-register no-ops.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 proof:

```sh
git diff --check
```

Result: passed.
