Status: Active
Source Idea Path: ideas/open/469_branch_stack_load_authority_metadata.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Branch Stack-Load Metadata Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 469.

Decision: the first missing prepared metadata field is explicit branch-site
stack-load policy for the stack-home branch value and role. Existing facts
already expose branch identity, operand roles, stack homes, stack slot ids,
offsets, object size/alignment, and object matches. They do not expose a record
authorizing a branch-site load; freshness and clobber-safety are also absent
and must be part of the same future authority surface.

Representative audit table:

| Row | Branch identity | Operand role | Value home / object | Current load/freshness/clobber evidence | Pointer status | First missing field |
| --- | --- | --- | --- | --- | --- | --- |
| `f.block_1` `%t2` | `condition=%t2 compare=ult ptr %t1, %p.reg2` | condition | stack slot `#11` offset `88`, object `#11` `i32` size `4` align `4` | No branch-stack-load policy, freshness, or clobber-safety record. | scalar condition | `branch_stack_load_policy` for condition role |
| `f.block_1` `%t1` | same branch | lhs | stack slot `#10` offset `80`, object `#10` `ptr` size `8` align `8` | No branch-stack-load policy, freshness, or clobber-safety record. | frame-slot load range proven but `layout_authority=unknown`; external-linkage formal caveat remains | `branch_stack_load_policy` for lhs role; pointer status must be recorded before available authority |
| `f.block_1` `%p.reg2` | same branch | rhs | register `a4` | Stack-load metadata not needed. | GPR formal pointer | none |
| `f.block_4` `%t8/%t7` | `condition=%t8 compare=ult ptr %t7, %p.mr_HB` | condition/lhs | `%t8` slot `#15`; `%t7` slot `#14` | No branch-stack-load policy/freshness/clobber records. | `%t7` pointer-value memory has `layout_authority=unknown`, `range_verdict=unknown_compatible` | `branch_stack_load_policy`, then pointer-provenance status blocks availability |
| `f.logic.end.14` `%t23/%t22` | `condition=%t23 compare=ne i32 %t22, 0` | condition/lhs | `%t23` slot `#21`; `%t22` slot `#20` | No branch-stack-load policy/freshness/clobber records; `%t22` block-entry publication is `unsupported_destination_storage`. | select-result stack destination | `branch_stack_load_policy`, plus select-result/block-entry owner remains separate |

Existing surface classification:

- `PreparedBranchCondition` supplies branch identity and operand roles but no
  branch stack-load authority.
- `PreparedValueHome` and stack layout supply slot/object facts but no
  freshness or branch-site load semantics.
- `PreparedFusedPointerBranchPublication` intentionally accepts only
  GPR-compatible branch homes; stack homes remain unsupported there.
- `PreparedDependencyOperandAuthority` has `load_from_stack_slot`,
  `missing_stack_freshness`, and `missing_stack_clobber_safety`, but it is
  scoped to select-edge dependency operands, not ordinary branch-site
  condition/lhs/rhs loads.

Artifact:
`build/agent_state/469_step1_branch_stack_load_metadata_audit/audit.md`.

## Suggested Next

Execute Step 2 from `plan.md`: define the branch-stack-load authority contract.

Suggested artifact directory:
`build/agent_state/469_step2_branch_stack_load_authority_contract/`.

Step 2 should define available/unavailable record fields and fail-closed
statuses for a future `PreparedBranchStackLoadAuthority`-style surface. The
first implementation packet should not weaken
`PreparedFusedPointerBranchPublication`; it should add separate metadata.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission before available metadata exists.
- Do not weaken GPR-compatible branch predicates.
- Treat `PreparedDependencyOperandAuthority` as precedent for statuses, not as
  the branch-site authority record itself.
- Do not infer branch loads, freshness, operands, or conditions from raw BIR,
  stack-slot spelling, block order, filenames, function names, or one prepared
  dump.
- Keep pointer-value/provenance repair and select-result/block-entry
  stack-destination repair separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 proof:

```sh
git diff --check
```

Result: passed.
