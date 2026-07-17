# F2 Exact Machine Verification Boundary

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent as one shared exact-candidate verifier

Phase: F2
Upstream: sealed private F1 machine graph and construction manifest
Downstream: verified machine-graph capability for F3

## Purpose

F2 proves that the exact private machine graph is structurally and
target-semantically ready for terminal consumers. External MIR/target rules
own machine semantics; this BIR-local owner defines the required gate.

## Owns

Exact candidate admission, cumulative instruction/operand/CFG/register/frame/
relocation-readiness validation, manifest reconciliation, diagnostics, and
failure-atomic minting of the F2 capability.

## Does Not Own

Graph construction, instruction selection, rewrite, peephole optimization,
allocation, spill insertion, copy scheduling, ABI or frame repair, assembly
parsing, encoding, object serialization, or linking.

## Inputs

The sealed private F1 graph and manifest, exact target/registry/source-view
keys, external target machine schema, register and operand rules, frame rules,
and relocation-readiness vocabulary.

## Input NodeKind/Tag Vocabulary

Exactly the closed target machine-record vocabulary named by the F1 registry
version and manifest. Unknown, stale, omitted, pseudo-BIR, or unregistered
machine kinds have no default admission.

## Required Analyses and Products

CFG, def-use, register-unit, frame, and relocation-readiness observations are
read-only verifier products over the exact candidate. They confer no mutation
authority and cannot substitute for manifest/source/target identity.

## Ordered Behavior

1. Freeze candidate, manifest, source-view, target, schema, and registry keys.
2. Reconcile record count, stable order, fresh identity, and F1 bijection.
3. Validate instruction forms, complete operands, CFG and terminators.
4. Validate fixed registers, aliases, clobbers, stack/frame locations and
   explicit frame-record coverage.
5. Validate symbols, relocation intents, opaque-inline-asm bindings and F3
   readiness; mint one capability only after every check passes.

## NodeKind/Tag Lowering Matrix

| F2 input subset | Verification disposition | Required facts | Output marking | Mutation | Failure |
|---|---|---|---|---|---|
| ordinary machine instruction | validate | registered form, operands, effects, CFG legality | verified in exact capability | none | malformed/illegal/unregistered rejects |
| move/spill/reload record | validate | fixed registers/locations, alias and memory legality | verified | none | illegal home or hidden temporary rejects |
| frame record | validate | exact plan correspondence, point, roles, balance and coverage | verified | none | missing/duplicate/unplanned record rejects |
| opaque inline-asm record | validate envelope only | unchanged payload, concrete bindings, effects/clobbers | late-parser-ready | none; never parse | payload/binding drift rejects |
| symbol/relocation-bearing record | validate readiness | target relocation kind, symbol/addend/section intent | F3-ready reference | none | incomplete/illegal intent rejects |
| unknown, stale, omitted, pseudo-BIR, or extra record | reject | none | none | none | `F2VocabularyInvalid` |

## Identity and Provenance

The capability names the exact machine graph revision and manifest. Machine
IDs remain authoritative only inside that graph; BIR IDs remain provenance.
Equal contents or regenerated identifiers do not establish identity.

## Outputs

One immutable verified-machine capability borrowing the exact sealed graph and
naming all verifier product keys, source manifest, target/schema/registry keys,
and relocation-readiness digest. It neither owns nor copies graph storage.

## Verification and Publication

Publication is all-or-nothing across the whole graph. Every registered record,
edge, operand, register, frame reference and relocation intent must pass the
same exact gate; function subsets and diagnostic-only checks mint nothing.

## Analysis Preservation and Invalidation

Any graph, manifest, target, schema, registry, frame, symbol, or relocation
change invalidates every F2 result and the capability. Reverification starts
from a newly sealed F1 candidate, never from repaired verifier state.

## Failure and Diagnostics

Diagnostics identify exact record/rule/product provenance. Failure, timeout,
cancellation, or resource exhaustion publishes nothing and never rewrites,
deletes, inserts, reschedules, reassigns, or requests an implicit retry.

## Adjacent-Stage Contract

F1 supplies one sealed private graph; F2 alone certifies it. F3 accepts only
this exact capability. External MIR target owners define machine legality;
external assemblers/object/linkers retain their own terminal semantics.

## Implementation State

Target-local checks may exist, but no evidence proves one shared gate over the
exact F1 manifest and capability model. This contract is therefore absent.

## Proof Requirements

Prove complete registered-vocabulary coverage, F1 bijection reconciliation,
instruction/operand/CFG/register/frame/relocation checks, opaque payload
preservation, exact-key invalidation, no mutation, and atomic capability mint.

## Open Questions

None at the architecture boundary. Target verifier implementations may remain
separate if they satisfy this common exact gate and capability contract.
