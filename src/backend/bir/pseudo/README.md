# Prospective PseudoPreallocation Planning Vocabulary

Contract-Status: documentation-only schema plan under idea 732
Implementation-Status: absent; no name below is claimed as a production `NodeKind`

## Authority and extension rule

The normative NodeKind/tag contract owns the six-axis algebra and stage
semantics. This file names prospective closed planning groups required by D/E
documents. Before implementation, every concrete kind needs a separately
reviewed production registry entry, payload/arity/type/role schema, explicit
stage admission, verifier rules, and positive/negative tests.

## Closed planning groups

| Group | Prospective members/shape | First owner | Required disposition |
|---|---|---|---|
| `D.GenericValue` | abstract pseudo value operations with explicit virtual defs/uses | D1 | D4 proves one-record realizability or expands explicitly |
| `D.GenericMemoryValue` / `D.GenericEffect` | address/memory/effect pseudos with exact roles | D1 | D4/E4 realize every effect and address requirement |
| `D.GenericCall` | temporary unresolved generic call placeholder | D1 | D2 must eliminate completely before D3 |
| `D.CallTransport` | abstract argument/result moves, outgoing stores, call, clobber, call-site preserve/restore | D2 | all defs/uses/clobbers enter E1/E2; no concrete resource |
| `D.PhiPending` | predecessor-qualified merge retained until out-of-SSA | D1 | D5 must eliminate completely |
| `D.InlineAsm` | one opaque token with ordinary typed roles and C9 binding reference | D1 | remains one opaque record; bytes never parsed before assembler |
| `D.ExpansionPlaceholder` | exact bounded target expansion obligation | D1/D4 | D4 must eliminate before post-D4 publication |
| `D.ParallelCopy` / `D.EdgeCopy` / `D.CopyScratch` | explicit out-of-SSA transfer and allocation-visible scratch forms | D5 | parallel/scratch forms resolved before E4; edge copies may remain |
| `E.Spill` / `E.Reload` | explicit allocation retry actions | E3 | directly realizable and exact spill-object keyed |
| `E.FrameAction` | bounded frame adjust/base/save/restore/probe actions | E4 | one-record realizable and exact final-plan covered |

No group is an “all other kinds” bucket. Unknown or omitted input fails at the
owning pass/gate. Shared `Node` storage gives no admission or retention.

## Stage and product keys

Every candidate/publication carries exact graph revision/lineage, target,
schema, preparation bundle, Canonical binding, current C9 projection, and
applicable pass/product fingerprints. A key is not freshness if copied,
compatible, equal-looking, or from another target/revision.

## Identity and result rules

Stage-owner/kind/shape/role/effect changes normally create fresh IDs. Every
replacement/expansion has a total typed old-result map and ordered provenance.
Copy destinations and scratch reservations are explicit role-governed
identities, never hidden outputs or product-only allocation state.

## Publication intervals

D1/D2 candidates remain private. D3 may publish the first allocation-free
PseudoPreallocation capability only after D2 and full verification. D4 and
initial D5 reverify/publish under the same allocation-free gate. E3 and later
assigned candidates use a distinct private gate and cannot be relabelled as a
second public pseudo stage.
