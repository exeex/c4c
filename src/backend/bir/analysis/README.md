# Revision-Bound BIR Analysis Framework Contract

Contract-Status: common analysis/product contract converging under idea 732
Implementation-Status: absent; no framework implementation is claimed

The root [`BIR README`](../README.md) owns the normative stage order and names
the first consumer of every analysis family. Analyses are dependency facts,
not additional mutable stages, and this document cannot move a consumer or
insert a new row into that order. The canonical pass framework requests only
target-independent facts during `B1`-`B8`; target/layout/preparation and
allocation facts first become legal at their root-declared later stages.

An analysis result is an immutable, disposable view of one exact published BIR
revision. Stable BIR IDs are its only persistent entity keys. Dense numbering,
worklist positions, pointers, names, rendered text, and legacy route records
are result-local conveniences and never semantic identity or publication
authority.

The [root common authority spine](../README.md#common-contract-authority-spine)
and [normative NodeKind/tag contract](../../../../docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md)
own stage and identity meanings. This framework never infers a domain from a
kind tag alone: the caller must present the exact published/candidate
capability admitted by the analysis descriptor. In particular,
`SsaEligible` does not authorize an SSA-dependent analysis without B4's dynamic
proof, and C-phase `Prepared` products do not create a mutable graph revision.

Every result and immutable preparation product is keyed by all authority it
observes: graph epoch/module/function revision and digest, schema/options,
target/profile/layout identity when applicable, predecessor product
fingerprints, and algorithm version. Missing, stale, foreign, mixed-target,
cross-revision, or merely equal-looking keys fail closed. A mutation cannot
retag an old result; preservation creates or installs a checked result under
the exact new key only after the private candidate passes its gate. Rollback
publishes no cache entry, refreshed handle, partial product, or alternate graph.

## 1. Scope and ownership

This framework owns:

- analysis descriptors, dependency traits, result schemas, and semantic
  equality used by preservation audits;
- exact cache keys and checked handles;
- computation deduplication and deterministic cache publication;
- invalidation, transitive dependency eviction, and checked rebinding after a
  committed mutation;
- separation of target-independent canonical facts from later target-bound
  allocation facts.

It does not own root stage order, IR mutation, stable-ID allocation, revision
increments, pass scheduling, verifier rules, allocation policy, spills, target
opcode choice, or stage-token publication. Core owns IDs and revisions; the
pass framework owns transactions and preservation claims; the verifier owns
validity; the pipeline owns orchestration within the root-declared canonical
interval.

The initial analysis families are:

- [`cfg`](cfg/README.md)
- [`dominance`](dominance/README.md)
- [`liveness`](liveness/README.md)
- [`memory_effects`](memory_effects/README.md)
- [`provenance`](provenance/README.md)
- [`call_graph`](call_graph/README.md)
- [`comparison`](comparison/README.md)
- [`publication`](publication/README.md)

Def-use is a core editor-maintained invariant, not an optional analysis.

## 2. Closed descriptors and scopes

Every registered analysis has one stable numeric `AnalysisId`, one schema
version, one scope, declared dependencies, declared input axes, and closed
invalidation traits. Unknown or duplicate IDs, dependency cycles, a scope that
cannot observe its declared inputs, or an undeclared target dependency reject
registry construction before BIR is consumed.

```cpp
enum class AnalysisScope : std::uint8_t {
  Function,
  Module,
};

enum class AnalysisDomain : std::uint8_t {
  CanonicalSemantic,
  TargetBoundAllocation,
};

struct AnalysisDescriptor final {
  AnalysisId id;
  AnalysisScope scope;
  AnalysisDomain domain;
  std::uint32_t schema_version;
  AnalysisIdSet dependencies;
  MutationEffectSet invalidated_by;
  bool observes_module_tables;
  bool observes_function_bodies;
  bool observes_target_layout;
  bool observes_preparation_facts;
};
```

`CanonicalSemantic` analyses may observe Raw, intermediate canonical, or
Canonical BIR semantics as their local contracts allow. They must reject
`TargetProfile`, target layout, ABI placement, constraint binding, physical or
pseudo-physical homes, spill decisions, target opcodes, MIR, renderer output,
and environment-selected backend features.

`TargetBoundAllocation` is a separate cache domain used no earlier than root
`E1`. Its descriptors must name the exact post-out-of-SSA BIR revision and the
exact target-layout, typed-constraint, call/preparation, and algorithm-schema
keys they read. A result in this domain cannot be requested through a
canonical-pass session, preserved backward into `CanonicalBir`, or installed
under a `CanonicalSemantic` key.

## 3. Exact revision keys

No analysis is keyed by a single vague "current revision." The minimum key is:

```cpp
struct AnalysisKey final {
  AnalysisId id;
  std::uint32_t schema_version;
  ModuleEpoch epoch;
  ModuleRevision module_revision;
  std::optional<FunctionId> function;
  std::optional<FunctionRevision> function_revision;
  FunctionRevisionDigest observed_functions;
  AnalysisOptionsFingerprint options;
  std::optional<TargetLayoutKey> target_layout;
  PreparationFactsDigest preparation_facts;
};
```

Rules for forming the key are exact:

1. A function result always carries `{epoch, FunctionId, FunctionRevision}`.
   If it reads types, constants, globals, declarations, symbol visibility, or
   other module tables, it also carries `ModuleRevision`.
2. A module result that reads any function body carries the ordered digest of
   every observed `(FunctionId, FunctionRevision)` pair. `ModuleRevision`
   alone cannot prove body freshness.
3. A result that reads only module-owned tables has an empty function digest;
   a result may not omit an axis merely because its current value is zero.
4. Options include every semantic choice and algorithm/schema version. Host
   timing, pointer values, worker count, cache state, and unordered traversal
   order are forbidden key or result inputs.
5. Canonical-domain keys have no target-layout key and an empty preparation
   digest. Allocation-domain keys require both whenever their traits declare
   those inputs.

An `AnalysisHandle<R>` retains the complete key. Every dereference checks the
calling view/capability against it. A mismatch returns structured
`StaleAnalysis`; it never silently points at the newest result, mutates its key,
or recomputes through the old handle.

## 4. Computation and deterministic cache publication

Only an `AnalysisManager` can compute or return a result. The manager validates
the requested analysis domain against the caller's stage capability, resolves
dependencies in the registered acyclic order, freezes the exact input views,
and computes from immutable storage.

Concurrent identical requests may share one in-flight computation. Publication
to the cache is all-or-nothing: dependencies and the result must still match
their complete keys when computation finishes. If a revision boundary has
advanced, the computed value is discarded and the request reports stale input
or retries as a fresh manager request; it is never published under the new key.

Results use stable IDs in externally observable maps and sort output by the
core canonical entity order. Dense indices may be built inside one result for
speed, but must be reconstructed from that result's stable-ID table and cannot
escape through diagnostics, mutations, another analysis key, or serialized
authority. Analyses cannot retain editors, mutable storage, pass sessions,
candidate pointers, or callbacks that mutate BIR.

Computation failure is structured and cached only if the failure key and
diagnostic ordering are deterministic. Cancellation and resource exhaustion do
not publish partial results.

## 5. Transaction and publication boundary

Published analysis caches describe published immutable revisions. A pass
transaction edits a private candidate and cannot modify, refresh, or attach
facts to the input revision's cache.

Candidate verification may compute transaction-local scratch facts against the
candidate view. Those facts die on rollback and are not returned as ordinary
analysis handles. They become cache candidates for a new revision only after:

```text
derive exact MutationSummary
  -> run required postconditions and verifier-on-commit
  -> decide preservation/rebinding from registered traits
  -> commit exactly one new private-fork revision
  -> install only results stamped with that exact new revision
  -> invalidate all other results and transitive dependents
  -> atomically publish the complete occurrence fork/checkpoint
```

If the verifier, postcondition check, preservation validator, commit, or
occurrence publication fails, no candidate result becomes observable and the
last-good checkpoint's cache remains unchanged. A verifier report is not an
analysis-preservation proof, and an analysis result cannot mint a stage token.

## 6. Invalidation and checked preservation

Each successful mutation has one core-derived `MutationSummary`. The manager
computes effective preservation rather than trusting the pass's declaration:

1. Intersect the invocation's `PreservedAnalyses` with the pass descriptor's
   statically permitted set.
2. Compare the exact mutation effects with each analysis's `invalidated_by`
   traits.
3. Require all dependencies to be preserved or freshly recomputed at the new
   key.
4. If effects overlap, reject preservation unless that analysis supplies a
   mutation-specific validator and it proves semantic equality on the complete
   before/candidate views.
5. In preservation-audit mode, recompute independently and compare using the
   analysis's defined semantic equality.
6. Evict every rejected result and its transitive dependents before exposing
   the new revision.

An unchanged transaction with an empty summary retains the same revision and
may reuse the same handles. After any revision increment, every old handle is
stale even when its analysis is preserved. Preservation means the manager may
install a checked immutable result under the new exact key; it never retargets
the old handle.

Conservative minimum invalidation includes:

| Mutation family | Results invalidated unless exactly revalidated |
|---|---|
| terminator, successor, block, edge-key, indirect-target, or asm-goto change | CFG, dominance/post-dominance, loops, SSA availability, liveness, path-sensitive provenance/effects, and all dependents |
| operand, definition, use, phi incoming, or value-type change | liveness, comparison/publication flow, memory effects, provenance, and dependents |
| load/store/atomic/address/effect change | memory effects, provenance, liveness where uses change, and dependents |
| call target, call bundle, intrinsic identity/effects, declaration, linkage, or visibility change | call graph, publication flow, memory effects, and dependents |
| module type, constant, global, initializer, symbol, or function-set change | every result whose descriptor observes module tables; body-observing module digests are rebuilt |
| pseudo node, explicit copy, assigned home, spill/reload, constraint, clobber, or preparation-fact change | allocation liveness/interference and every downstream allocation-domain result |

`PreservedAnalyses::all()` is valid only for an empty summary or when every live
result independently passes its registered preservation rule. A pass cannot
declare CFG preserved merely because it did not add a block, or liveness
preserved merely because the number of values is unchanged.

## 7. Root-stage and target separation

The root analysis table fixes when a family may first be requested. A local
analysis README may narrow that point but cannot move it earlier. In
particular:

- canonical `P01`-`P07` analyses derive only target-independent semantic facts;
- `C1` target selection and `C2` layout do not retroactively alter or rekey a
  canonical result;
- preparation facts are immutable side products bound to Canonical revision,
  layout key, and their own schema/revision digest, never fields attached to a
  canonical analysis;
- allocation liveness/interference at `E1` is freshly computed for the exact
  post-`D5` revision and exact later facts;
- an `E3` spill/reload mutation invalidates `E1` facts and follows only the
  root-declared `E3 -> E1` retry edge. A cached liveness result cannot skip or
  reorder that edge;
- target-aware facts never flow backward into Raw/Canonical passes, their
  caches, or their verifier profiles.

## 8. Cross-document requirements

Each analysis-family README must define:

- accepted stage capability and analysis domain;
- exact stable-ID inputs and complete revision/key axes;
- dependencies and deterministic computation order;
- result schema, lifetime, and semantic equality;
- invalidating mutation effects and any exact rebind validator;
- failure/cancellation behavior and diagnostic keys;
- first root-stage consumer and prohibited consumers;
- disposition of mapped legacy behavior without retaining legacy authority.

The pass framework owns the closed `AnalysisId` values used by descriptors;
this document owns the traits and result contracts registered under those IDs.
Core wins on ID/revision semantics, verifier wins on validity, and the root
README wins on order. Any disagreement fails closed: no result is published or
preserved until the contracts are reconciled.

## 9. Acceptance checks

Review must reject an implementation if any answer is yes:

- Can a handle survive an epoch/revision/digest mismatch without returning
  `StaleAnalysis`?
- Can dense numbering, a pointer, name, rendered text, or a legacy record become
  BIR identity?
- Can an old handle be retargeted after preservation?
- Can a pass publish candidate facts before verifier-on-commit succeeds?
- Can a mutation omit transitive invalidation or preserve a dependency at the
  wrong key?
- Can a canonical analysis inspect target, ABI, constraint, home, spill, MIR,
  or renderer facts?
- Can an allocation result omit the post-out-of-SSA revision or later-fact
  keys it consumed?
- Can an analysis result insert, skip, or reorder a root stage or retry edge?

Expected implementation proof includes wrong-epoch/revision/digest tests,
dependency-invalidation tests, preservation-audit recomputation, rollback and
verifier-failure cache isolation, concurrent stale-computation races,
deterministic dense-index reconstruction, target-domain access rejection, and
`E3 -> E1` allocation-fact invalidation.
