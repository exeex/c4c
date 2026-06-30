Status: Active
Source Idea Path: ideas/open/469_branch_stack_load_authority_metadata.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Branch Stack-Load Authority Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 469: defined the prepared branch-stack-load
authority metadata contract.

Decision: a bounded Step 3 metadata packet is justified. The accepted route is
a separate `PreparedBranchStackLoadAuthority`-style surface, not a weakening of
`PreparedFusedPointerBranchPublication` and not RV64 branch-load emission.
Stack slot ids, offsets, homes, and object facts remain evidence only; an
available record must explicitly authorize loading that value at that branch
site.

Accepted available-record contract:

| Area | Required fact |
| --- | --- |
| Branch identity | Prepared branch condition with function/block identity, terminator target labels, condition value, predicate, compare type, and matching raw terminator where applicable. |
| Operand role | Explicit role for the stack-home value: `condition`, `lhs`, or `rhs`; role is not inferred from raw BIR shape or dump spelling. |
| Value identity | Prepared value id/name, type width, and expected scalar/GPR load width for the role. |
| Stack home | Value home is a stack slot with slot id, stack offset, size, alignment, and matching prepared stack object/frame-slot facts. |
| Load policy | Explicit branch-site `load_from_stack_slot` policy for that branch/role/value; no policy means unavailable. |
| Freshness | Prepared fact proves the slot contains the current value at the branch site. |
| Clobber safety | Prepared fact proves no intervening call, helper, move bundle, or stack write invalidates the slot before the branch-site load. |
| Scratch/order boundary | Metadata must preserve enough role/load-width information for a later consumer to choose safe scratch/order; Step 3 must not emit RV64. |
| Pointer status | Pointer operands must have explicit acceptable pointer/provenance status; unknown pointer-value memory or external-formal ambiguity keeps the row unavailable. |

Unavailable/fail-closed statuses to model:

| Status bucket | Examples |
| --- | --- |
| Missing branch identity | Missing names, missing branch condition, missing terminator, unsupported terminator/branch condition, condition mismatch, target mismatch. |
| Missing role/value/home | Missing operand role, missing value home, home value mismatch, unsupported non-stack home for this authority. |
| Stack object mismatch | Missing stack object, slot/object mismatch, insufficient size, insufficient alignment, or out-of-range load width. |
| Missing policy | Stack home exists but no branch-site stack-load policy is present. |
| Freshness/clobber unavailable | Missing freshness proof, missing clobber-safety proof, or explicit clobbered/stale status. |
| Pointer not proven | Pointer operand has unknown provenance/layout/range status, pointer-value memory gap, or external formal caveat. |
| Adjacent owner | Select-result stack destination, block-entry stack publication gap, generic stack-to-register move, or raw-shape-only fixture. |

Representative classification under this contract:

| Row | Contract status | Reason |
| --- | --- | --- |
| `f.block_1` condition `%t2` | Positive metadata candidate if explicit policy/freshness/clobber facts are added. | Scalar condition stack slot facts are coherent; first missing fact is branch-site load authority. |
| `f.block_1` lhs `%t1` | Unavailable until pointer status is explicit. | Stack facts are coherent, but pointer/formal provenance remains ambiguous and cannot be accepted from slot facts alone. |
| `f.block_1` rhs `%p.reg2` | Out of scope for stack-load metadata. | Already register-home. |
| `f.block_4` `%t8/%t7` | Unavailable. | Missing branch stack-load authority and `%t7` pointer-value/provenance status is not proven. |
| `f.logic.end.14` `%t23/%t22` | Unavailable, adjacent owner. | Select-result/block-entry stack destination work remains separate. |

Artifact:
`build/agent_state/469_step2_branch_stack_load_authority_contract/contract.md`.

## Suggested Next

Execute Step 3 from `plan.md`: implement or route the first metadata packet.

Suggested bounded packet:

- Owned files:
  `src/backend/prealloc/publication_plans.hpp`,
  `src/backend/prealloc/publication_plans.cpp`,
  `tests/backend/bir/backend_prepare_stack_layout_test.cpp`,
  `todo.md`,
  `test_after.log`,
  `build/agent_state/469_step3_branch_stack_load_authority_metadata/**`.
- Add a prepared metadata record/status surface for branch-stack-load authority.
- Positive coverage should prove an available scalar stack-home branch
  condition or operand only when branch identity, role, stack object, explicit
  load policy, freshness, and clobber-safety are coherent.
- Negative coverage should keep missing policy, missing freshness, missing
  clobber-safety, mismatched role/home/object, pointer unknown, select-result
  stack destination, GPR-only rows, and raw-shape-only fixtures unavailable.
- Proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not edit RV64 object emission in the metadata packet.
- Do not weaken `PreparedFusedPointerBranchPublication`; it should continue to
  require GPR-compatible condition and operand homes.
- Treat `PreparedDependencyOperandAuthority` as status precedent only; it is
  select-edge dependency metadata, not the branch-site authority record.
- Do not infer authority from raw BIR, `.prepared.out` spelling, stack-slot
  numbering, block order, function/test names, or the `930930-1` shape.
- Keep pointer-value/provenance repair for `%t7`, external formal provenance,
  and select-result/block-entry stack-destination repair separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 proof:

```sh
git diff --check
```

Result: passed.
