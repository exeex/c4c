Status: Active
Source Idea Path: ideas/open/486_bir_index_range_proof_path_dominance_carrier.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Range Proof And Path-Dominance Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 486 by defining the dynamic local-array index range
proof and path-dominance carrier contract.

Accepted carrier requirements:

| Record family | Required fields |
| --- | --- |
| Consumer identity | function id/name, local-array source object, derivation id/result, element-path id/result, consumer block/instruction coordinate, dynamic index value, element count, element size/stride |
| Proof source | proof block label, proof instruction/terminator coordinate, source kind, predicate, compare type, lhs/rhs operands, structured value ids/names where available |
| Lower bound | index operand role, lower bound value, inclusivity, width/signedness policy, normalized proof of `0 <= index` or equivalent |
| Upper bound | index operand role, upper bound value, inclusivity, width/signedness policy, normalized proof of `index < element_count` or equivalent |
| Path/dominance | proof source dominates or guards the consumer, consumer path is covered by the proven branch edge/path, function/control-flow identity matches |
| No-clobber/same-value | no assignment, phi replacement, move/publication, helper/call, inline asm, or other effect invalidates the index value between proof source and consumer |

Required fail-closed statuses:

- `missing_local_array_path`, `missing_dynamic_index`,
  `missing_proof_source`, `unsupported_proof_source`,
  `missing_lower_bound`, `missing_upper_bound`,
  `unsupported_predicate`, `unsupported_index_width`,
  `operand_role_mismatch`, `bound_value_mismatch`,
  `proof_function_mismatch`, `proof_not_dominating_consumer`,
  `path_not_covering_consumer`, `missing_path_validity`,
  `missing_no_clobber`, `index_value_clobbered`,
  `index_value_redefined`, `index_phi_or_alias_unresolved`,
  `call_or_helper_effect_unknown`, `call_or_helper_clobbers_index`,
  `inline_asm_effect_unknown`, `publication_or_move_effect_unknown`,
  `publication_or_move_clobbers_index`, `raw_shape_only`,
  `target_only_or_final_home_only`, and `unsupported_boundary`.

Step 3 readiness:

- A bounded implementation packet is justified for an independent
  metadata/status surface with a planner/checker API that can accept synthetic
  complete inputs and produce `available`.
- Real dynamic local-array rows must remain unavailable unless producer data can
  provide proof source, normalized bounds, path/dominance, and no-clobber facts.
- If current producer data cannot supply those links without raw-shape
  inference, Step 3 should route to the exact lower-level producer blocker
  rather than marking dynamic rows available.

Artifact:

- `build/agent_state/486_step2_range_proof_path_dominance_contract/contract.md`

## Suggested Next

Execute Step 3: implement or route the first range proof/path-dominance carrier
packet. Keep it bounded to metadata/status publication and focused coverage;
do not consume it from idea 484 packaging or scalar local-load production.

## Watchouts

- Do not infer index range from loop shape, value names, testcase names, dump
  order, final homes, or RV64 target behavior.
- Do not mark dynamic GEP rows available without proof source,
  path/dominance, and no-clobber facts.
- Do not change idea 485 carrier records, idea 484 packaging, scalar local-load
  consumption, or RV64/MIR lowering in this proof-carrier packet.
- `MemoryDynamicArrayRangeVerdict::BoundedByElementCount` is not enough by
  itself for idea 486; it lacks proof-source, dominance/path, and no-clobber
  authority.
- `PreparedControlFlowFunction::branch_conditions` exposes compare fields but
  does not by itself prove that a branch condition covers a later dynamic GEP
  consumer.
- Synthetic positive coverage is acceptable for the first status surface; real
  dynamic rows remain unavailable until producer data supplies every required
  fact.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
