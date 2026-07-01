Status: Active
Source Idea Path: ideas/open/486_bir_index_range_proof_path_dominance_carrier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Index Range Proof Inputs

# Current Packet

## Just Finished

Completed Step 1 for idea 486 by auditing current index range proof,
path/dominance, and no-clobber inputs for dynamic local-array indices.

Representative audit:

| Field | Current input | Classification |
| --- | --- | --- |
| Source object | `local_array_source_objects` from static local array `alloca` | available from idea 485 |
| Derivation | `local_array_derivations` from explicit local array/view GEP | available from idea 485 |
| Element path/layout | `local_array_element_paths` with element count/size and byte layout | available from idea 485 |
| Dynamic index identity | `LocalArrayIndexRecord::value` carries the BIR index value, e.g. `%idx` | available |
| Current dynamic status | `LocalArrayCarrierStatus::MissingIndexRangeProof` | intentionally unavailable |
| Proof source identity | no durable compare/branch proof source record tied to the local-array index | missing |
| Lower/upper bound facts | no durable normalized `0 <= index < element_count` proof | missing |
| Predicate/operand mapping | branch conditions expose compare fields, but no carrier maps them to local-array index bounds | missing |
| Path/dominance validity | prepared CFG has block/branch facts and some reachability-style helpers, but no proof-to-GEP consumer certificate | missing |
| Index no-clobber/same-value | no record proving the index value is unchanged between proof source and GEP consumer | missing |

Existing usable surfaces:

- `bir::MemoryDynamicArrayFacts` can compute passive byte-range envelope
  verdicts after requested ranges are known, but it is not proof-source or
  path authority.
- `PreparedControlFlowFunction` exposes structured blocks and branch
  conditions.
- Prepared lookup helpers show reachability/dominance-style filtering exists
  for other domains, but there is no index-range proof carrier today.
- Existing path/clobber status vocabularies in frame-slot and semantic
  materialization records are useful precedent, not direct authority for
  dynamic local-array indices.

Artifact:

- `build/agent_state/486_step1_index_range_proof_inputs/audit.md`

## Suggested Next

Execute Step 2: define the dynamic local-array index range proof carrier
contract. Require explicit proof source identity, bound facts, predicate/operand
role mapping, path/dominance validity, and index no-clobber/same-value evidence;
preserve dynamic rows as unavailable when any fact is missing.

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
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
