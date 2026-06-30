Status: Active
Source Idea Path: ideas/open/441_terminator_select_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Terminator/Select Publication Evidence

# Current Packet

## Just Finished

Completed Step 1: audited terminator/select publication evidence from the 433
Step 4 residual disposition and the 440 Step 6 handoff, with supporting notes
under `build/agent_state/441_step1_terminator_select_audit/`.

Bucket classification:

| Bucket | Representative evidence | Authority state / first owner |
| --- | --- | --- |
| First coherent fused pointer branch candidate | `20041112-1 bar.entry`: `%t6 = bir.ne ptr %t2, %t5` feeds `bir.cond_br i32 %t6, block_4, block_5`; prepared `branch_condition entry kind=fused_compare condition=%t6 compare=ne ptr %t2, %t5`; `%t2`, `%t5`, and `%t6` have register homes. | Coherent candidate, but missing an explicit terminator/select publication contract tying branch condition, compare operands, homes, and target labels to RV64 consumption. |
| Closed direct-global return | `20041112-1 foo.block_1`: `bir.ret ptr @global`, prepared `@global` register home, and matching before-return ABI move. | Complete in idea 440; do not reopen here. |
| Pointer relational branch candidates | `20010329-1 main.entry`: `%t8 = bir.uge ptr %t5, %t7`; prepared fused branch condition exists with register homes. `930930-1` has `ult` pointer branch rows. | Candidate but broader than the first packet because pointer ordering predicate policy must be defined first. |
| Select materialization feeding branches | `20010329-1` `%t22/%t36/%t50`, `20000622-1` `%t13/%t24`, and `930930-1` `%t22` have `select_chain ... root_is_select=yes` and branch conditions comparing select results to zero. | Follow-up select-publication contract/consumer work; not the first packet. |
| Unsupported select/storage and mixed producer residuals | `930930-1` has `block_entry_publication ... unsupported_destination_storage`, pointer-base-plus-offset homes, pointer-value memory, and local/store-source publication gaps. `20000622-1` currently reports `unsupported_instruction_fragment`, not isolated terminator failure. | Out of first packet; split or follow up after explicit select/storage publication facts exist. |
| Completed global/store/direct-global prerequisites | Prior global layout/source and direct-global return facts remain visible in dumps. | Out of scope for idea 441 Step 1. |

## Suggested Next

Execute Step 2: define the terminator/select publication contract. The first
bounded implementation target should be the `20041112-1`-class fused pointer
`eq/ne` branch publication packet, restricted to prepared `branch_condition`
facts with authoritative GPR-compatible compare operand homes, coherent
condition/result homes, and stable true/false target labels. Keep pointer
relational predicates and select-result publication as follow-ups unless the
contract deliberately includes them.

## Watchouts

- Keep this plan limited to terminator/select publication authority.
- Do not reopen direct-global return authority; closed idea 440 routes only the
  remaining `bar.entry` fused pointer compare branch here.
- Do not infer branch conditions, select results, compare operands, or
  terminator homes from raw BIR shape, block order, filenames, function names,
  or one dump layout.
- Do not include pointer relational predicates (`ult/uge/ule`) or select-chain
  materialization in the first implementation packet until the contract states
  their authority and target predicate policy.
- Keep missing or incoherent publication authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 classification validation:

```sh
git diff --check
```

Result: passed.
