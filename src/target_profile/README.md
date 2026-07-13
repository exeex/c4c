# External Target-Profile Selection and Validation Boundary

Contract-Status: under-review
Implementation-Status: partial-foundation
Kind: external-boundary
Phase-ID: C1
Upstream: one exact B8 verifier-gated target-independent unallocated `CanonicalBir` plus one explicit `TargetRequest`
Downstream: one immutable validated `TargetProfile` plus complete `TargetFingerprint` consumed by the C1 verifier binding and C2
Owner-Path: `src/target_profile/README.md`
Last-Reconciled-Commit: `bd0a13414`

## Purpose

C1 is the sole external authority for selecting, normalizing and validating an
explicit target request. It accepts only the exact immutable `CanonicalBir`
published by B8 plus the caller's request, validates every target axis as one
coherent product, and publishes one immutable validated `TargetProfile` and
one complete `TargetFingerprint`. It never chooses a target for the user and
never writes target facts into Raw or Canonical BIR.

## Owns

- the closed `TargetRequest` schema and explicit caller-selected defaulting
  modes;
- normalization of requested triple, architecture, OS, backend ABI,
  relocation model, floating-point ABI and enabled capabilities;
- cross-field coherence, registered-support and schema/version validation;
- deterministic formation of one `TargetFingerprint`/validated target-context
  key covering every normalized target axis and selection table version;
- stable structured selection/validation failures and atomic no-output failure.

## Does Not Own

- the user's target choice or environment-dependent target inference that the
  request did not explicitly authorize;
- B8 Canonical verification/publication, mutation of `RawBir` or
  `CanonicalBir`, or reconstruction of a Canonical capability from a view,
  stamp, report or semantic hash;
- the verifier's exact-Canonical `VerifiedPreparationInput` binding gate;
- parsing rendered `data_layout`, deriving target layout, ABI placement,
  preparation or constraints, or executing C2-C9;
- pseudo lowering, liveness, assignment, homes, spills, frames, MIR, rendering
  or emission;
- publication/value-flow, call-graph, provenance, memory-effects or any other
  analysis result as selection, validation or acceptance authority.

## Inputs

C1 receives one move-only exact B8 `CanonicalBir` capability and one immutable
`TargetRequest`. The Canonical capability and stamp are carried opaquely to the
adjacent verifier binding; target selection does not inspect, validate, copy or
mutate either one. Every
request axis is explicit either as a concrete value or as a caller-selected
registered defaulting mode whose resolved value becomes part of the key.

### Exact C1 input matrix

| Input / axis | Exact required state | Explicit optional/default form | Failure / forbidden substitution |
|---|---|---|---|
| Canonical capability | one move-only verifier-gated B8 `CanonicalBir` with exact `PipelineStageStamp` | empty module is valid | Raw, candidate, view, copy or earlier report is not an admissible C1 input; C1 does not repair or relabel it |
| Canonical stamp | exact epoch, module revision, ordered function-revision digest, canonical plan/options lineage, ordinal 7 and every P01-P07 property | none | carried opaquely; only the verifier binding may compare it and reject stale/partial/equal-looking/mixed lineage |
| requested triple | one normalized registered triple spelling | caller may explicitly select `HostDefault` with the resolved triple frozen in the request | empty implicit host probing or triple/axis disagreement is `TargetTripleInvalid` |
| requested architecture | one registered `TargetArch` agreeing with the triple | caller-selected `FromTriple` is resolved before validation | unknown, unsupported or conflicting architecture is `TargetArchInvalid` |
| requested OS | one registered `TargetOs` agreeing with triple and ABI | caller-selected `FromTriple` is resolved before validation | unknown, unsupported or conflicting OS is `TargetOsInvalid` |
| requested backend ABI | one registered `BackendAbiKind` legal for architecture/OS | caller-selected registered default is resolved and recorded | unknown, target-incompatible or unavailable ABI is `TargetAbiInvalid` |
| requested relocation model | one registered static/PIC/PIE model supported by target and route | caller-selected schema default is resolved and recorded | omitted implicit mutation or unsupported combination is `TargetRelocationInvalid` |
| requested floating-point ABI | one closed ABI mode with exact argument/result capability requirements | caller-selected ABI-derived mode is resolved and recorded | contradictory lane/calling capability is `TargetFloatAbiInvalid` |
| requested capabilities | one closed, sorted enabled/disabled capability set with registry identities | explicit empty set is valid | host probing, unknown capability, duplicate or unsupported combination is `TargetCapabilityInvalid` |
| selection version | exact target-selection algorithm and registered-support-table versions | none | missing or mismatched version is `TargetSelectionVersionInvalid` |
| schema versions | exact `TargetRequest`, `TargetProfile` and `TargetFingerprint` schema versions | none | missing, newer, older or mixed schema is `TargetSchemaInvalid` |

## Outputs

Success publishes exactly one immutable validated profile/fingerprint pair.
Failure returns one structured error and publishes no profile, fingerprint,
binding capability, cache entry or BIR revision. Both input objects remain
unchanged on every route.

### Exact C1 output matrix

| Output / result | Exact consumer | Required binding | Failure / forbidden escape |
|---|---|---|---|
| validated immutable `TargetProfile` | C1 `verify_preparation_input` binding and C2 | exact normalized triple/arch/OS/backend-ABI/relocation/float-ABI/capabilities and schema versions | directly constructed, partially normalized, mutable or merely compatible profile is not validated output |
| complete `TargetFingerprint` / validated target-context key | verifier binding, C2 and all later product keys | deterministic fingerprint of every normalized request axis, selected defaults, registry/table versions and profile schema | architecture/triple-only hash, omitted axis or reconstructed equality is not capability identity |
| normalized request record | diagnostics and deterministic replay only | exact original request, selected default modes and resolved values | cannot grant acceptance, alter the profile or replace the fingerprint |
| structured `TargetSelectionFailure` | caller and diagnostics | stable rule/axis/request/version/cause and deterministic ordering | report grants no profile, fingerprint, `VerifiedPreparationInput` or retry authority |

## Adjacent-Stage Contract

The [ordered Canonical pipeline](../backend/bir/pipeline/README.md) alone
supplies the exact immutable B8 `CanonicalBir`. C1 validates the explicit
request independently of BIR contents and passes that same Canonical owner,
the validated profile and complete fingerprint to the [shared verifier's
preparation-input gate](../backend/bir/verify/README.md). The verifier alone
binds the fingerprint to the unchanged Canonical `PipelineStageStamp` and may
return `VerifiedPreparationInput`. [C2 target layout](../backend/bir/target_layout/README.md)
then consumes that binding and exact validated target key; C1 cannot derive or
publish layout.

Authority is closed:

| Authority | Sole owner | Cannot do |
|---|---|---|
| explicit request normalization, validation, registered support and target fingerprint | external C1 boundary | choose a request for the user, mutate BIR or derive layout |
| full Canonical rules and exact Canonical/target binding into `VerifiedPreparationInput` | BIR verifier | select/default target axes, manufacture a profile or derive layout |
| finite target register/layout vocabulary and `VerifiedTargetLayout` | C2 target layout | change request/profile/fingerprint, mutate Canonical or act as C1 validation |

## Ordered Behavior

1. Require the exact B8 Canonical capability type and immutable explicit
   request. Carry Canonical and its stamp opaquely; validate only the request
   identity/schema before target normalization.
2. Resolve only caller-selected registered defaults and record each selected
   mode and resolved value.
3. Normalize every request axis, then validate all axes together against the
   exact selection and support-table versions.
4. Build one private immutable `TargetProfile` and complete fingerprint from
   the same normalized record; recheck field-for-field coverage and coherence.
5. Atomically publish the pair or discard the private candidate and return one
   structured failure. Never consume, edit or replace Canonical storage.
6. Submit the unchanged Canonical owner and validated pair to the verifier-only
   binding gate; no C1 report or fingerprint can create that capability.

## Invariants

- There is exactly one normalized profile and one complete fingerprint per
  accepted request. Similarity, a subset hash or architecture equality is not
  acceptance.
- Every default is explicitly selected by the caller and included in replay
  identity; current environment, worker, cache or pointer state is never an
  implicit key input.
- Raw and Canonical remain target-independent and unallocated. Source-semantic
  typed sizes, alignments and address spaces are not selected target layout.
- Analyses remain later exact-revision dependencies. None participates in C1
  selection, validation, fingerprinting or publication.

## Failure and Atomicity

Any malformed request, unknown or conflicting axis, unsupported combination,
schema/table mismatch, cancellation or deterministic
resource failure discards every private normalized/profile/fingerprint
candidate. No partial profile, key, cache entry, `VerifiedPreparationInput` or
BIR revision is published. The exact Canonical owner and request are unchanged.

## Target, ABI, and Layout Rules

C1 validates target and coarse ABI identity only. It does not derive pointer
widths, storage layouts, register classes, ABI value placement, helpers,
constraints, homes, spills, frame offsets or target opcodes. Rendered
`data_layout` and compatibility strings are origin/parity evidence only and
cannot manufacture a request axis, profile fact or fingerprint.

## Implementation State

Implementation is partial foundation only. The build-included
[`target_profile.hpp`](../target_profile.hpp) and
[`target_profile.cpp`](../target_profile.cpp) provide a mutable
`TargetProfile` value, triple parser/default helpers, name helpers, target
arch/OS/backend-ABI/relocation enums and two coarse float-register capability
booleans. Existing frontend, LIR and legacy consumers are migration evidence.

Absent are the closed `TargetRequest` and capability registry, explicit
default-mode records, all-axis coherence/support validator, immutable validated
profile capability, `TargetFingerprint`/target-context key, structured C1
failure, Canonical-bound C1 entry point and verifier-gate integration. Current
`target_profile_from_triple` is not C1 validation or fingerprint publication,
and no checked-in implementation can produce C1 success.

## Proof Requirements

- prove exact B8 Canonical plus explicit request input and no Raw/view/stale
  substitution;
- prove every target axis/default/schema/table version appears in both
  validation and the fingerprint;
- prove atomic no-output failure and byte-identical Raw/Canonical storage;
- prove external C1, verifier binding and C2 layout authorities are distinct;
- prove no analysis, layout, preparation, allocation or MIR authority leaks
  into C1; and
- reconcile every implementation claim with code, build inclusion and callable
  reachability.

## Open Questions

None. Adding a target axis, default mode, capability registry or validation
owner requires a coordinated update to this boundary, its consumers and the
fingerprint schema.

## Review Checklist

- [x] Metadata and core-first ownership are explicit.
- [x] Input and output matrices are exhaustive and failure-atomic.
- [x] C1 owns selection/validation/fingerprinting but not user choice.
- [x] Verifier-only Canonical binding and C2-only layout are preserved.
- [x] Raw/Canonical mutation, rendered-layout parsing and analyses are excluded.
- [x] Partial-foundation implementation truth is explicit.
