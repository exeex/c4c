# Architecture Review Template

Use this checklist for every pass, analysis, preparation product, allocation
phase, publication gate, observation helper, and MIR stage. A review compares
both adjacent boundaries and the linked source idea. Checking one document or
one named testcase is insufficient.

Start from the [root common authority spine](README.md#common-contract-authority-spine).
Do not review a local restatement as a second NodeKind/tag authority. The
[normative NodeKind contract](../../../docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md)
owns stage admission, tag meaning, identity, and common publication rejection;
the pass owner supplies only its closed explicit lowering matrix. A proposed
Markdown symbol is not implementation evidence.

## 1. Complete boundary walk

Record the input/output type, owner, exact revision/key, verifier profile,
failure result, and next consumer for every row:

| Boundary | Required review question |
|---|---|
| import -> `RawBir` | Does typed import finish without text/name/pointer recovery, and does full Raw verification atomically publish the exact frozen draft? |
| `RawBir` -> `CanonicalBir` | Does each ordered canonical pass preserve predecessor profiles, own only its declared forms, invalidate analyses correctly, and end in full Canonical verification of one frozen revision? |
| `CanonicalBir` -> preparation | Does `VerifiedPreparationInput` borrow the exact Canonical revision and target, and do layout plus `abi -> calls -> variadic -> address -> inline_asm -> runtime_helpers` publish immutable products with ordered predecessor fingerprints? |
| preparation -> D1 -> D2 -> D3 `PseudoBir` | Do constraints bind ordinary values, D1 fork a separate closed pseudo schema, D2 alone add shared ABI-aware call transport, and D3 fully verify before publication? |
| `PseudoBir` -> D4 -> initial D5 | Does D4 perform every required target one-to-many expansion and full reverification, then does initial D5 remove all phi semantics into the bounded intermediate `ParallelCopy`/`EdgeCopy` and `CopyScratch` schema with exact projection? |
| initial D5 -> E1 | Are liveness, interference, fixed homes, call/inline-asm roles, clobbers, simultaneous-copy semantics, scratch reservations, and pressure derived from the exact fully reverified initial-D5 candidate revision? |
| E1 -> E2 | Does E2 alone assign legal abstract category/class-or-group/slot homes using the exact E1 and target-pool keys? |
| E2 -> E3 -> E1 retry | Does E3 alone add explicit abstract spill identities and `Spill`/`Reload`; does every mutation advance and pass the private assigned-candidate gate before fresh E1/E2 facts, without invoking the allocation-free Pseudo publication gate? |
| stable E3 -> D5 copy resolution -> E4 -> `AllocatedBir` | Does D5 alone resolve every bundle; does E4 materialize every required frame action as a bounded fixed-role one-record node before final projection/E1/E2/E3/frame/target closure; do all final keys bind `FrameActionFingerprint`; and does E4 publish atomically? |
| `AllocatedBir` -> E4 MIR-readiness capability / `MirReadyBirView` -> MIR | Do all capabilities name the same materialized revision and exact frame plan; does MIR apply one mapping per explicit node without choosing placement, inserting a frame record, expanding, or returning repair? |

For each transition, attach the predecessor fingerprint set and prove the
consumer rejects a missing, stale, foreign, mixed-target, or cross-revision
product. State whether mutation is forbidden, failure-atomic, idempotent, or a
bounded retry. No later stage may be described as repairing malformed input.

## 2. Ownership and observation review

- Is every produced fact assigned to exactly one stage, pass, planner,
  analysis, allocator phase, or publication gate?
- Are all other uses borrowing, read-only, revision-bound consumers?
- Can diagnostics, rendering, audit, caches, compatibility quarantine, or
  legacy evidence alter success, severity, ordering, product selection, target
  route, allocation, or publication? Any yes is rejection.
- Can a name, pointer, index, rendered string, legacy route/agreement record,
  or debug snapshot enter a stage API? Any yes is rejection.
- Does failure publish no partial stage object, reusable green report,
  refreshed key, assignment fragment, or alternate graph?
- Are the E4 MIR-readiness capability and `MirReadyBirView` over the exact owning
  `AllocatedBir`, rather than copied or rebuilt instruction storage?
- Is C-phase `Prepared` described only as immutable Canonical admission by
  exact-revision reference plus exact target/preparation products, never as an
  E4 readiness alias or a mutable prepared graph?
- Does every pass matrix close its admitted input set with explicit
  retain/replace/expand/merge/delete/reject outcomes and fail unknown, illegal,
  omitted, and unhandled vocabulary?
- Does any claim of SSA validity cite B4's dynamic graph proof rather than the
  static stage-qualified `SsaEligible` classification?
- Does `FrameActionMaterializationTransaction` insert every required entry,
  exit, dynamic-lifetime, call, save/restore, probe, and adjustment action as a
  bounded explicit one-record node before final projection, and does the final
  `FrameRealizationPlan` cover each node and access without hidden F1 work?
- Are published D3/D4/initial-D5 `PseudoBir` and private assigned E3/D5
  candidates checked by distinct gates with opposite allocation-state rules?

## 3. Legacy disposition review

- Does every current file and externally used symbol under
  `src/backend/legacy/` match exactly one row in `LEGACY_COVERAGE.md`?
- Does every retained behavior have one `Accepted` owner, or an explicit
  `Reject`/`Defer` disposition with the earlier profiles that forbid it?
- Are duplicate routes, lookup agreement, persistent analysis mirrors,
  prepared graph/state, special value carriers, physical-home side tables, and
  phase strings deleted rather than reproduced?
- Are error paths, diagnostics, object data, inline assembly, calls/returns,
  variadics, atomics, runtime helpers, uncommon integer/float widths, dynamic
  stack/frame behavior, and all RV64/AArch64/x86 differences accounted for?
- Which exact legacy reader and writer becomes unnecessary, and what is its
  deletion checkpoint?

## 4. Same-family and anti-overfit proof

- Identify the capability family, all adjacent operation shapes, all supported
  targets/profiles, success cases, rejection cases, retry paths, cancellation,
  stale-key cases, and resource failures before selecting tests.
- Prove the general typed rule or lowering behavior, then sample the named
  regression. A patch whose main effect is recognizing that regression's
  spelling, opcode, type, block shape, or target is rejected.
- Do not weaken a supported path to `unsupported`, rewrite an expectation to
  claim progress, add a testcase-shaped matcher, or accept rendered/legacy
  evidence in place of capability repair.
- Require nearby same-feature coverage and at least one negative that would
  pass under the suspected shortcut.
- For inline asm, prove ordinary input/result SSA transport, read/write old-use
  versus new-result identity, structured constraint binding, ties as home
  equality rather than SSA identity, clobbers, groups, pressure/spill retry,
  and original text preservation without parsing template instructions early.

## 5. Acceptance record

Record the reviewed source-idea checkpoint, document/code diff base, owners,
open gaps, legacy dispositions, exact proof commands, regression-log paths when
applicable, and result for every boundary row. Acceptance requires no
unresolved ownership, adjacency, revision, quarantine, or same-family gap.

Reject the slice if any stage can consult legacy state, any observer can affect
compilation, any product key can float across revisions, any allocation fact
can bypass E1/E2/E3/E4, any MIR route can perform ordinary allocation or
capacity spill/reload, or proof is primarily a named testcase/expectation
change.
