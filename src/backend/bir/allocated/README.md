# Allocated BIR

Status: scaffold (unimplemented).

## Owns

E4 owns one private `AllocatedPublicationTransaction`. It consumes the stable
E3 candidate, lets D5 resolve all copy bundles, materializes every required
frame action as an explicit BIR node, rebuilds every exact-current product, and
only then freezes and publishes the final revision. Success atomically mints
one owning `AllocatedBir`, one `PreparedBir` readiness capability bound to it,
and borrowing `MirReadyBirView` instances. All capabilities name the same
immutable module/function revisions and product fingerprints; none clones the
graph. These names are prospective while the scaffold remains unimplemented.

E4 does not select machine instructions, spell concrete registers, encode
frame offsets, parse assembly, repair allocation, or reinterpret constraints.
F1 is an apply-only consumer of the already explicit graph and verified plans.

## Input and private ordering

The transaction begins with the exact stable E3 candidate and its current
projection, `LivenessInterferenceKey`, `AssignmentKey`, `SpillStateKey`, D2
call obligations, C2 layout, C3/C4 plans, C6 address facts, target mapping
tables, and preparation fingerprints. D5 first runs its bounded
`CopyResolutionTransaction`; no `ParallelCopy` or `CopyScratch` survives.

The rest of the enclosing transaction has one mandatory order:

1. build a private deterministic frame-action draft from the exact
   D5-resolved graph and the named E3/C2/C3/C4/D2/C6/target facts;
2. run `FrameActionMaterializationTransaction`, producing the final graph;
3. run the sole `ConstraintProjectionTransaction` for that final revision;
4. recompute E1 and install its final `LivenessInterferenceKey`;
5. have E2 validate and install the unchanged assignments without allocation;
6. have E3 validate and install the unchanged spill state without mutation;
7. derive the non-mutating final `FrameRealizationPlan` and
   `FrameRealizationKey` over the materialized graph; and
8. run the target registry and install the final `TargetRealizabilityKey`.

Every product names the final `PipelineStageStamp`,
`CopyResolutionFingerprint`, `FrameActionFingerprint`, exact upstream keys,
and applicable schema/target fingerprints. The draft and every predecessor or
pre-materialization product are lineage only. Stable IDs, structural equality,
or a preservation record cannot rekey them. Installation occurs only after all
eight steps succeed.

## Frame-action materialization

`FrameActionMaterializationTransaction` is the sole graph-mutating E4
subordinate. Its private draft deterministically fixes frame regions, object
placements, bases, offsets, displacements, stack size/alignment, action points,
and registered mapping-rule IDs for E3 spill objects, D2 call objects and
hidden carriers, C6 address obligations, static and dynamic lifetimes, and
callee-save obligations. For callee saves it intersects the D2 abstract
function-level obligation set with exact post-allocation used units; only that
result may produce `FrameCalleeSave`/`FrameCalleeRestore`. D2
`AbiPreserve`/`AbiRestore` remain per-call value transport and cannot satisfy or
duplicate these entry/exit actions. The draft is neither a product nor an F1
instruction source.

The transaction inserts only the closed E4-candidate frame-action family:

- `FrameAdjust`, with one fixed stack-pointer input/output role;
- `FrameBaseSetup` and `FrameBaseRestore`, with fixed ABI frame/base roles;
- `FrameCalleeSave` and `FrameCalleeRestore`, with one fixed callee-save role
  and one exact frame-object reference; and
- `FrameProbe`, when required by the selected supported ABI/target rule, with
  fixed stack/frame roles.

Each variant has one registered direct mapping to exactly one machine record,
fixed effects and ABI/frame roles, and no allocatable result, temporary, or
scratch request. The materializer may emit a deterministic sequence of
multiple one-record nodes, including chunked `FrameAdjust` or `FrameProbe`
sequences, but no node expands later. Any additional action required by a
supported ABI/target must first be added as another finite reviewed variant;
an action needing allocatable scratch or lacking a direct mapping fails before
publication.

Action points are exact entry, normal/exceptional exit, dynamic-lifetime, and
call-required positions derived from the draft. Inserted nodes receive fresh
deterministically reserved instruction IDs, exact effect and stack-object
references, and ordinary origin records. The transaction advances the
revision/stage stamp and emits a complete mutation summary,
`FrameActionFingerprint`, replacement map, and tombstones. Existing IDs remain
stable. Cancellation, ID exhaustion, inconsistent placement, or any node or
action failure rolls back the complete enclosing transaction; neither a draft,
partial sequence, fingerprint, product, nor capability is published.

Core `StackSave`/`StackRestore`, `DynamicAlloc`, and lifetime nodes remain
source-semantic operations with their original token, dominance, and lifetime
meaning. E4 `FrameBaseSetup`/`FrameBaseRestore` and `FrameAdjust` are target
frame establishment/teardown actions derived from final placement; they cannot
replace, consume, reinterpret, or suppress a semantic stack operation. When
both are required, both remain with distinct IDs and exact coverage.

## Final frame realization and output

After materialization, exact-current projection/E1/E2/E3 closure, the
non-mutating frame owner derives `FrameRealizationPlan` and
`FrameRealizationKey` from the final graph. The plan covers every explicit
frame-action node and every spill/reload, call, local/dynamic-frame, scratch,
and address access. It contains exact placements and registered mappings but
cannot insert a node or conceal an action. Every record-producing prologue,
epilogue, stack adjustment, save/restore, probe, and other frame action is an
explicit graph node before F1.

The final target checker consumes that exact plan and proves one registered
mapping for every non-`InlineAsm` node. Its `TargetRealizabilityKey`
incorporates `FrameRealizationKey` and `FrameActionFingerprint`.

The resulting `MirReadyBirView` exposes the closed semantic/pseudo nodes,
E3 `Spill`/`Reload`, D5-resolved `EdgeCopy`, explicit E4 frame-action nodes,
assignments, the exact frame plan, and fingerprint lineage. Every allocatable
identity has one legal abstract home or explicit spill residency; the new
frame actions have only their fixed ABI/frame roles.

## Verification and publication gate

`AssignedAllocationCandidateGate` admits the E4 frame-action variants only
after materialization and requires exact coverage by the final frame and target
products. It still forbids them before E4; `PseudoPublicationGate` always
rejects them. The Allocated gate additionally proves:

- every ordinary/copy/call/inline-asm role remains legally assigned and all
  ties, aliases, groups, clobbers, and interference are satisfied;
- every spill object and transition remains complete, with no implicit
  residency change or allocation repair;
- no `ParallelCopy` or `CopyScratch` remains and every `EdgeCopy` preserves its
  resolved simultaneous-copy semantics;
- every required frame action appears exactly once at its planned point as an
  admitted one-record node, and every admitted node has exact plan coverage;
- projection, E1, E2, E3, frame realization, and target realizability all name
  the materialized final revision and the same `FrameActionFingerprint`; and
- no hidden frame work, active editor, mixed key, or missing mapping
  remains.

Any failure discards the complete `AllocatedPublicationTransaction` and mints
none of `AllocatedBir`, `PreparedBir`, `MirReadyBirView`, a partial function
token, or a reusable green report.

F1 accepts only `MirReadyBirView`, rechecks all keys, and emits exactly one
machine record for each node by applying its registered mapping and concrete
spellings. It cannot add frame records, choose placements, change assignments,
insert spills, create scratch, schedule copies, expand a node, or repair the
graph. Mapping failure requires a separately reviewed upstream BIR change.
