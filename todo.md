Status: Active
Source Idea Path: ideas/open/441_terminator_select_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4: re-probed terminator/select representatives after the Step 3
fused pointer `eq/ne` branch publication consumer and classified residual
ownership.

Residual table:

| Row | Current result | Classification |
| --- | --- | --- |
| `20041112-1` | RV64 object emission succeeds; `20041112-1.object.err` is empty and `20041112-1.o` is produced. | The former `bar.entry` fused pointer `ne` branch owner is resolved by Step 3. |
| `20010329-1` | `unsupported_terminator_fragment` | First visible terminator owner is pointer relational fused compare: `compare=uge ptr %t5, %t7`; select-result branch rows remain later follow-up candidates. |
| `20000622-1` | `unsupported_instruction_fragment` | Not isolated to terminator consumption; select-result publication rows exist, but the first current failure is instruction-side. |
| `930930-1` | `unsupported_instruction_fragment` | First owners remain pointer-value/local publication and unsupported instruction/storage paths; pointer relational and select-derived branches are separate follow-ups. |

Accepted/closed by this idea packet:

- Prepared fused pointer `eq/ne` branches with coherent prepared branch
  condition, target labels, condition home, and GPR-compatible operand homes.
- Representative `20041112-1 bar.entry`
  `branch_condition entry kind=fused_compare condition=%t6 compare=ne ptr %t2, %t5`
  now emits in RV64 object route.

Remaining possible idea-441 follow-ups, only if selected as new packets:

- pointer relational fused pointer branch predicate policy/consumer
  (`uge/ult/...`);
- select-result publication into branch conditions (`root_is_select=yes`
  feeding `ne i32 <select>, 0`).

Out of scope / first-owned elsewhere:

- direct-global return authority, closed by idea 440;
- direct-global select-chain/global store-source rows that no longer block
  `20041112-1`;
- pointer-value memory, local/global store-source publication, unsupported
  instruction fragments, stack-slot/unsupported destination storage,
  expectation/allowlist changes, and pass/fail accounting.

Artifacts:

- `build/agent_state/441_step4_residual_disposition/classification.md`
- `build/agent_state/441_step4_residual_disposition/20041112-1.prepared.out`
- `build/agent_state/441_step4_residual_disposition/20041112-1.object.err`
- `build/agent_state/441_step4_residual_disposition/20010329-1.object.err`
- `build/agent_state/441_step4_residual_disposition/20000622-1.object.err`
- `build/agent_state/441_step4_residual_disposition/930930-1.object.err`

## Suggested Next

Plan-owner close-readiness review. Recommendation: close idea 441 if its scope
is the selected fused pointer `eq/ne` branch publication packet; otherwise keep
active only by explicitly selecting one new exact packet, either pointer
relational fused pointer branch policy/consumer or select-result publication
into branch conditions.

## Watchouts

- No remaining residual is the same fused pointer `eq/ne` publication gap.
- Do not treat `20010329-1` pointer relational predicates as a small extension
  of Step 3; they need a separate predicate policy.
- Do not treat `root_is_select=yes` rows as branch publication without a
  select-result authority contract.
- Keep implementation, expectations, allowlists, `test_before.log`,
  `test_baseline.new.log`, and `review/` untouched in this residual packet.

## Proof

Step 4 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. `test_after.log` contains the backend subset proof.
