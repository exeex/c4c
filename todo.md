Status: Active
Source Idea Path: ideas/open/470_branch_stack_load_policy_freshness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Policy Freshness Packet

# Current Packet

## Just Finished

Completed Step 3 as a routed/blocker packet. No producer implementation was
selected because current prepared facts do not prove branch-site
`load_from_stack_slot` policy, slot freshness, or clobber safety for real
records. The existing carrier can represent those facts, but no safe producer
currently owns them.

Implementation decision:

| Candidate | Result |
| --- | --- |
| Scalar populated `missing_policy` row (`f.logic.end.14` `%t23`) | Cannot become available yet; missing explicit branch-site load policy, freshness, and clobber-safety producer facts. |
| `f.logic.end.14` `%t22` | Remains fail-closed; select-result stack destination is a separate owner. |
| `f.block_1` / `f.block_4` rows | Remain fail-closed at `unsupported_terminator`; branch-site relationship acceptance is a separate prerequisite. |
| Pointer operands `%t1` / `%t7` | Remain fail-closed/separate; pointer status and provenance are not proven. |

Exact blocker:

- Missing producer/prepared branch-site load-safety facts: whether the slot may
  be loaded at the branch site, whether it still contains the current condition
  value, and whether no intervening store/call/helper/publication/move/stack
  write clobbers it.
- Setting `load_from_stack_slot` from stack homes, frame slots, object ids,
  prepared branch shape, or testcase evidence alone would be raw-shape
  inference and is rejected.

Artifact:
`build/agent_state/470_step3_policy_freshness_population/blocker.md`.

## Suggested Next

Execute Step 4: Residual Disposition And Close Readiness. Decide whether idea
470 should close/split to a narrower branch-site freshness/clobber-safety
producer initiative, or remain active only if one exact in-scope metadata
packet can produce those facts without raw-shape inference.

## Watchouts

- Do not edit RV64 target lowering.
- Do not set `load_from_stack_slot` unless freshness and clobber safety are
  also explicitly proven.
- Preserve `unsupported_terminator` rows as unavailable unless a separate
  prerequisite packet proves the branch-site relationship.
- The carrier capacity exists; the blocker is missing producer facts, not the
  enum/record shape.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not accept unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Do not infer load authority from stack homes, offsets, object ids, raw BIR,
  block labels, function names, or testcase shape.
- Keep pointer-value/provenance and select-result stack-destination boundaries
  separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Delegated proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` reports all 327 backend tests passed.
