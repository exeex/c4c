Status: Active
Source Idea Path: ideas/open/451_stack_home_branch_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Stack-Home Branch Evidence

# Current Packet

## Just Finished

Completed Step 1 audit for idea 451.

Fresh `930930-1` probe artifacts were written under
`build/agent_state/451_step1_stack_home_branch_evidence/`. Prepared dump status
is `0`; object route status is `2` with
`unsupported_instruction_fragment`, matching the inherited 449 residual that
the module still fails before a clean branch-consumer decision.

Representative stack-home branch table:

| Function/block | Prepared branch fact | Stack-home operands/condition | Stack object identity | Offset / width | Freshness / clobber facts | Pointer provenance status | First owner |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `f.block_1` | `condition=%t2 compare=ult ptr %t1, %p.reg2` | `%t1` lhs stack slot; `%t2` condition stack slot; rhs `%p.reg2` is GPR `a4` | `%t1` object `#10` regalloc spill `ptr`; `%t2` object `#11` regalloc spill `i32` | `%t1` slot `#10` offset `80`, size `8`; `%t2` slot `#11` offset `88`, size `4` | No branch stack-load policy, freshness proof, or clobber-safety fact printed. | `%t1` comes from frame-slot load of `%lv.param.reg1`; range proven, but `layout_authority=unknown` and external-linkage formal provenance remains parked outside this idea. | Cleanest Step 2 contract candidate, but not target-consumable without explicit stack-home load/freshness authority. |
| `f.block_4` | `condition=%t8 compare=ult ptr %t7, %p.mr_HB` | `%t7` lhs stack slot; `%t8` condition stack slot; rhs `%p.mr_HB` is GPR `a2` | `%t7` object `#14` regalloc spill `ptr`; `%t8` object `#15` regalloc spill `i32` | `%t7` slot `#14` offset `112`, size `8`; `%t8` slot `#15` offset `120`, size `4` | No branch stack-load policy, freshness proof, or clobber-safety fact printed. | `%t7` is `inttoptr` of pointer-value memory load; related access has `layout_authority=unknown` and `range_verdict=unknown_compatible`. | Pointer-value/provenance blocker plus stack-home materialization gap; do not accept by stack loading alone. |
| `f.logic.end.14` | `condition=%t23 compare=ne i32 %t22, 0` | `%t22` select result stack slot; `%t23` condition stack slot | `%t22` object `#20` regalloc spill `i32`; `%t23` object `#21` regalloc spill `i32` | `%t22` slot `#20` offset `152`, size `4`; `%t23` slot `#21` offset `156`, size `4` | No branch stack-load policy, freshness proof, or clobber-safety fact printed. | `%t22` is select materialization over `%t18`/`0`; block-entry publication is `unsupported_destination_storage`. | Select-result/block-entry stack destination plus stack-home condition owner; not the first pointer-relational operand packet. |
| `f.block_7` | `condition=%t32 compare=ne ptr %t30, %t31` | none; GPR-compatible homes | not stack-home | not stack-home | not applicable | closed by existing GPR branch routes | Out of scope for idea 451. |

Classification: no stack-home branch row is target-consumable today. Stack
object ids, offsets, and widths are visible, but the prepared surface does not
publish branch-specific load policy, freshness, or clobber-safety authority.
`f.block_4` additionally needs pointer-value/provenance authority before a
branch consumer can soundly materialize `%t7`; `f.logic.end.14` belongs first
to select-result/block-entry stack-destination work.

Artifact:
`build/agent_state/451_step1_stack_home_branch_evidence/audit.md`.

## Suggested Next

Execute Step 2 from `plan.md`: define the stack-home branch materialization
contract.

Step 2 should state the accepted prepared facts for a future consumer:
stack object identity, slot offset/size/alignment, branch operand role, load
policy, freshness and clobber safety at the branch site, pointer provenance
requirements for pointer-derived operands, and fail-closed cases. If those
facts are not representable today, route to a producer/prepared metadata packet
instead of selecting RV64 implementation.

Suggested artifact directory:
`build/agent_state/451_step2_stack_home_branch_contract/`.

## Watchouts

- Do not select RV64 implementation from stack-slot homes alone.
- Do not weaken existing GPR-compatible branch publication predicates.
- Do not infer stack-home values, freshness, loads, operands, or conditions
  from raw BIR shape, stack-slot spelling, block order, filenames, function
  names, or one prepared dump layout.
- Keep pointer-value/provenance publication for `%t7`, select-result/block-entry
  stack destination work for `%t22/%t23`, instruction/storage owners, and
  unrelated residuals separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 proof:

```sh
git diff --check
```

Result: passed.
