Status: Active
Source Idea Path: ideas/open/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit LIR Producer Certificate Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 490.

Audit result:

| Area | Classification |
| --- | --- |
| LIR producer binding | `LocalArrayElementPathRecord::lir_producer_*` and `lir-producer:` keys are sufficient certificate keys for address-derivation paths. |
| Prepared proof source | `PreparedBranchCondition` fused compare fields and compare operand producer helpers provide candidate branch/compare proof-source facts. |
| Path/dominance helpers | Prepared block dominance/reachability helpers exist, but are helper-level and not durable certificates keyed to `lir_producer_*` or selected edge/outcome. |
| LIR-to-prepared coordinate mapping | Block labels may be matchable; `lir_producer_instruction_index` remains a LIR producer-site index and cannot be compared to prepared/BIR instruction indices without a separate truthful bridge. |
| Dynamic index identity | Dynamic index value identity exists on `LocalArrayIndexRecord`; no interval certificate proves same-value preservation. |
| Effects/no-clobber | Redefinition, phi/alias, call/helper, inline asm, publication/move-bundle, and parallel-copy effects have adjacent surfaces or status vocabulary, but no dynamic-index interval certificate producer keyed to `lir_producer_*`. |
| Step 2 readiness | Step 2 can define a bounded certificate contract now. Step 3 must remain fail-closed if the missing interval/path producers cannot be implemented without inference. |

Supporting artifact:

- `build/agent_state/490_step1_lir_producer_certificate_inputs_audit/audit.md`

## Suggested Next

Execute Step 2: define the LIR producer path/no-clobber certificate contract,
using the smallest scalar dynamic local-array address-derivation representative
and requiring explicit proof source, selected edge/outcome, path coverage,
dominance/guard validity, dynamic-index identity, and same-value/no-clobber
facts.

## Watchouts

- Do not populate idea 486 checker inputs directly in this certificate idea.
- Do not infer path coverage or no-clobber from branch proximity, loop shape,
  value names, testcase names, dump order, final homes, or target behavior.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate; do
  not reinterpret it as a prepared traversal or BIR `Block::insts` coordinate.
- If Step 3 cannot produce interval effects safely, route to
  `dynamic local-array LIR producer interval effect classifier`.
- Existing untracked `review/*.md` files were not touched.

## Proof

Step 1 audit validation:

```sh
git diff --check
```

Result: passed.
