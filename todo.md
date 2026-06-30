Status: Active
Source Idea Path: ideas/open/449_pointer_relational_branch_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Pointer Relational Branch Evidence

# Current Packet

## Just Finished

Completed Step 1: audited pointer relational fused branch evidence for idea
449 and recorded the classification under
`build/agent_state/449_step1_pointer_relational_audit/classification.md`.

Bucket table:

| Row | Predicate / prepared fact | Homes | Authority state | First missing fact |
| --- | --- | --- | --- | --- |
| `20010329-1 main.entry` | `%t8 = bir.uge ptr %t5, %t7`; `branch_condition entry kind=fused_compare condition=%t8 compare=uge ptr %t5, %t7 true=logic.rhs.11 false=logic.skip.12` | `%t5` register `t0`, `%t7` register `s2`, `%t8` register `s1` | Structurally coherent first candidate, but not target-consumable yet. | Explicit pointer relational policy: allowed predicates, unsigned/signed ordering semantics, provenance/null requirements, and accepted operand forms. |
| `20010329-1 logic.end.14` | Pointer relational `uge ptr` appears inside `%t22 = bir.select ...`; branch fact is `ne i32 %t22, 0`. | `%t22` register `t0`, `%t23` register `s1` | Not a direct pointer relational branch packet. | Select-result publication; route to idea 450. |
| `20010329-1 logic.end.27` | Pointer relational `%t32 = bir.uge ptr %t28, %t31` feeds select `%t36`; branch fact is `ne i32 %t36, 0`. | Relational operands/results have GPR homes. | Not a direct pointer relational branch packet. | Select-result publication / relational-select policy, not direct fused branch consumption. |
| `930930-1 f.block_1` | `%t2 = bir.ult ptr %t1, %p.reg2`; `branch_condition block_1 ... compare=ult ptr %t1, %p.reg2` | `%t1` stack slot, `%p.reg2` register `a4`, `%t2` stack slot | Not first-packet target; object route currently fails earlier with `unsupported_instruction_fragment`. | Stack-home materialization/publication plus relational policy/provenance. |
| `930930-1 f.block_4` | `%t8 = bir.ult ptr %t7, %p.mr_HB`; `branch_condition block_4 ... compare=ult ptr %t7, %p.mr_HB` | `%t7` stack slot, `%p.mr_HB` register `a2`, `%t8` stack slot | Not first-packet target; `%t7` comes from pointer-value memory with unknown authority and route fails earlier. | Pointer-value/provenance and stack-home materialization first; relational policy later. |
| `930930-1 f.block_7` | `compare=ne ptr %t30, %t31` | pointer-base-plus-offset register homes and `%t32` register | Pointer equality/inequality branch, not relational. | Closed/out of scope via idea 441. |

Accepted target-consumable pointer relational rows today: none.

First coherent structural candidate: `20010329-1 main.entry`, because it has a
prepared fused pointer relational branch, stable labels, and GPR homes for both
operands and the condition. It is blocked from implementation by missing
predicate policy, not by missing prepared branch-condition facts.

Implementation blocker before code:

- no accepted policy yet for which pointer relational predicates are allowed
  (`uge`, `ult`, etc.);
- no documented ordering semantics, such as RV64 unsigned address ordering;
- no accepted provenance/null requirements for integer-address `inttoptr`
  values or formal pointers;
- no finalized operand-form boundary beyond the likely GPR-home candidate.

## Suggested Next

Execute Step 2: define the pointer relational predicate policy before any RV64
consumer implementation.

Step 2 should state:

- accepted/rejected predicates (`uge`, `ult`, and any excluded relational
  variants);
- pointer ordering semantics, including whether RV64 unsigned address ordering
  is the selected target behavior;
- null and non-null immediate pointer handling;
- provenance requirements for `inttoptr`, formals, pointer-value memory, and
  same-object/unknown-compatible pointers;
- required prepared branch-condition, label, condition-home, and operand-home
  facts;
- exact owned files/tests and the future implementation proof command if a
  consumer packet becomes coherent.

## Watchouts

- Keep this plan limited to pointer relational fused branch predicates.
- Do not reopen pointer `eq/ne` branch publication; closed idea 441 owns that
  route.
- Do not fold select-result branch publication into this plan; that belongs to
  `ideas/open/450_select_result_branch_publication.md`.
- Do not infer pointer relational semantics from raw BIR compare/branch shape,
  block order, filenames, function names, testcase names, or one dump layout.
- Keep missing, incoherent, unsupported, or policy-ambiguous relational branch
  authority fail-closed.
- Do not implement the `20010329-1 main.entry` candidate until Step 2 chooses a
  predicate/order/provenance policy.
- Keep `930930-1` stack-home and pointer-value-memory rows out of the first
  consumer packet.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Activation-only validation:

```sh
git diff --check
```

Step 1 classification validation:

```sh
git diff --check
```

Result: passed.
