# Idea 494 Step 2 - Interval Effect Classifier Contract

## Decision

Define a fail-closed interval effect classifier contract now, but do not
publish available interval facts yet.

The current inputs can identify the selected proof edge, dynamic local-array
index, and LIR producer key. They cannot truthfully identify the ordered
prepared/BIR endpoint for the local-array address derivation. Until a real
bridge exists from `lir_producer_lookup_key` or
`lir_producer_instruction_index` to that prepared/BIR endpoint, every
same-value/no-clobber interval fact remains unavailable with a precise missing
bridge status.

## Interval Key

Each classifier row is keyed by one populated selected proof-edge path record
and one dynamic local-array producer row.

Required key fields:

| Field group | Required fields |
| --- | --- |
| LIR producer identity | `lir_producer_lookup_key`, `lir_producer_function_name`, `lir_producer_block_label`, `lir_producer_instruction_index`, `lir_producer_coordinate_status`, and `lir_producer_operation_role=address_derivation`. |
| Local-array identity | local-array element path identity, derived pointer/result identity, source object identity, and exactly one dynamic `LocalArrayIndexRecord`. |
| Dynamic index identity | structured `bir::Value` name/id where available, type/width, and operand role in the proof compare. |
| Proof source identity | proof function, proof block, proof branch/compare condition, predicate, compare type, lhs/rhs operands, selected outcome, selected successor, non-selected successor, and bound contribution. |
| Path scope | selected proof-edge/path coverage facts, dominance or guard booleans, and same-function requirement. |

The classifier must treat `lir_producer_instruction_index` as a LIR
producer-site coordinate only. It is not a prepared traversal index, BIR
instruction index, or ordered endpoint for effect slicing.

## Interval Semantics

`interval_start` is the selected proof edge:

- proof function and proof block match the selected proof-edge path record;
- selected outcome and selected successor are explicit;
- the dynamic index operand in the proof compare matches the local-array
  dynamic index by structured value identity and type;
- cross-block available records may start at selected successor entry.

`interval_end` is the local-array address-derivation event for the same
`lir_producer_lookup_key`.

The end is not currently available as a prepared/BIR ordered endpoint. A future
bridge must publish or derive an authoritative prepared/BIR event coordinate
for the address-derivation site. The bridge must be keyed back to the exact
`lir_producer_lookup_key` and must be ordered in the effect stream that the
classifier scans.

Same-block proof/producer rows are fail-closed unless that bridge also proves
the proof source precedes the address derivation without reinterpreting
`lir_producer_instruction_index`.

## Same-Value Criterion

The classifier may report same-value only when all of these facts are explicit:

- the proof compare operand and local-array dynamic index are the same
  structured value with matching type/width;
- phi/alias or transfer records on the selected path preserve that identity, or
  resolve to a known equivalent value;
- no assignment, redefinition, publication, move bundle, parallel copy, call,
  helper, inline asm, or unknown modeled effect changes or invalidates the
  dynamic index between `interval_start` and `interval_end`;
- the effect scan is bounded by the selected path and the prepared/BIR endpoint
  for the exact address derivation.

Selected path availability, value spelling, testcase shape, final homes, target
behavior, or nearby BIR adjacency are never sufficient same-value evidence.

## Effect Classes

The classifier must distinguish these effect families before an interval can
be available:

| Effect class | Available condition | Fail-closed condition |
| --- | --- | --- |
| Assignment / redefinition | No write or redefinition of the dynamic index in the bounded interval. | Any matching redefinition or inability to scan the bounded interval. |
| Phi / alias / join transfer | Dynamic-index identity is explicitly preserved or resolved across selected transfers. | Unresolved alias/phi identity or path transfer ambiguity. |
| Calls / helpers | Call and helper plans prove the dynamic index value is preserved. | Unknown call/helper effect or possible clobber. |
| Inline asm | Inline asm carriers prove no side effect or clobber reaches the index. | Unknown side effect or clobber metadata. |
| Publications | Source/destination homes and memory-access facts prove no index clobber. | Unclassified publication or publication targeting/invalidating the index. |
| Move bundles | Ordered prepared move data proves no index clobber. | Unclassified move bundle or move targeting/invalidating the index. |
| Parallel copies | Parallel-copy bundle facts prove selected-path preservation. | Unresolved or clobbering parallel copy. |
| Unknown modeled effects | No unknown effect lies in the bounded interval. | Any unclassified modeled effect in the interval. |

Detailed producer statuses should remain distinct even if a later consumer maps
some of them to narrower proof vocabulary.

## Status Vocabulary

The contract requires at least these distinguishable statuses:

| Status | Meaning |
| --- | --- |
| `available` | Dynamic-index identity, selected path, ordered interval end, and every effect family prove same-value/no-clobber. This status is currently disallowed until the endpoint bridge exists. |
| `missing_lir_producer_lookup_key` | The selected proof-edge row is not tied to a concrete producer key. |
| `missing_lir_producer_coordinate` | Required LIR producer coordinate fields are absent or unavailable. |
| `unsupported_lir_producer_role` | The producer role is not local-array `address_derivation`. |
| `missing_dynamic_index` | No single structured dynamic index is available. |
| `dynamic_index_operand_mismatch` | The proof compare operand does not match the dynamic index by structured identity and type. |
| `missing_selected_edge_or_outcome` | The selected proof edge or selected successor is not explicit. |
| `missing_path_validity` | No selected path/dominance/guard fact covers the producer block. |
| `path_not_covering_lir_producer` | A path fact exists but does not cover the LIR producer site. |
| `missing_prepared_bir_endpoint_bridge` | No authoritative bridge maps the producer key/site to the prepared/BIR address-derivation endpoint. |
| `prepared_bir_coordinate_confusion` | Classification would require treating `lir_producer_instruction_index` as a prepared/BIR instruction coordinate. |
| `missing_same_block_ordering` | Proof and producer are in the same block without a truthful order proof. |
| `selected_path_only_inference` | The row would infer no-clobber from path availability rather than scanned effects. |
| `index_value_redefined` | An assignment/redefinition changes the dynamic index in the interval. |
| `index_phi_or_alias_unresolved` | Phi, alias, join, or transfer identity is unresolved. |
| `call_or_helper_effect_unknown` | A call/helper effect cannot prove preservation. |
| `call_or_helper_clobbers_index` | A call/helper may clobber the index. |
| `inline_asm_effect_unknown` | Inline asm side effects or clobbers are not classified. |
| `inline_asm_clobbers_index` | Inline asm may clobber the index. |
| `publication_effect_unknown` | Publication effects are not classified for the index. |
| `publication_clobbers_index` | A publication may clobber the index. |
| `move_bundle_effect_unknown` | Move-bundle effects are not classified for the index. |
| `move_bundle_clobbers_index` | A move bundle may clobber the index. |
| `parallel_copy_effect_unknown` | Parallel-copy effects are not classified for the index. |
| `parallel_copy_clobbers_index` | A parallel copy may clobber the index. |
| `unknown_effect` | A modeled effect in the bounded interval has no safe classification. |
| `raw_shape_only` | The evidence is testcase, spelling, final-home, adjacency, or target-shape inference. |

## Availability Rule

For this slice, all otherwise promising rows must remain unavailable with
`missing_prepared_bir_endpoint_bridge` when the only endpoint evidence is
`lir_producer_lookup_key` plus `lir_producer_instruction_index`.

`available` may be emitted only after a later packet supplies a real
LIR-producer-to-prepared/BIR endpoint bridge and the bounded effect scan proves
every effect class safe. The classifier must refuse coordinate confusion and
must refuse selected-path-only inference.

## Focused Test Implications

Later focused tests can use this contract as follows:

- positive fixture: synthetic or real record with a bridged prepared/BIR
  endpoint, matching dynamic index, covered selected path, and safe effect
  classifications returns `available`;
- bridge-missing fixture: populated selected proof-edge path with only
  `lir_producer_lookup_key` and `lir_producer_instruction_index` returns
  `missing_prepared_bir_endpoint_bridge`;
- same-block fixture without truthful ordering returns
  `missing_same_block_ordering` or `missing_prepared_bir_endpoint_bridge`;
- coordinate-confusion fixture that attempts to use
  `lir_producer_instruction_index` as a prepared/BIR order returns
  `prepared_bir_coordinate_confusion`;
- selected-path-only fixture without effect classifications returns
  `selected_path_only_inference` or the precise missing effect status;
- one fixture per effect family returns the matching unknown/clobber status
  instead of `available`.

## Step 3 Boundary

Step 3 should route to the lower bridge owner or add only fail-closed status
surface/tests. It must not implement available interval facts until the
prepared/BIR endpoint bridge exists. It must not populate idea 489 proof facts,
idea 486 checker inputs, idea 484 packaging, scalar local-load consumption, or
RV64/MIR lowering.

## Proof

```sh
git diff --check
```

Result: passed.
