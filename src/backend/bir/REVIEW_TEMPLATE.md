# Architecture Review Template

Use this checklist for every pass, analysis, preparation product, allocation
phase, publication gate, observation helper, and MIR stage. A review compares
both adjacent boundaries and the linked source idea. Checking one document or
one named testcase is insufficient.

## 1. Complete boundary walk

Record the input/output type, owner, exact revision/key, verifier profile,
failure result, and next consumer for every row:

| Boundary | Required review question |
|---|---|
| import -> `RawBir` | Does typed import finish without text/name/pointer recovery, and does full Raw verification atomically publish the exact frozen draft? |
| `RawBir` -> `CanonicalBir` | Does each ordered canonical pass preserve predecessor profiles, own only its declared forms, invalidate analyses correctly, and end in full Canonical verification of one frozen revision? |
| `CanonicalBir` -> preparation | Does `VerifiedPreparationInput` borrow the exact Canonical revision and target, and do layout plus `abi -> calls -> variadic -> address -> inline_asm -> runtime_helpers` publish immutable products with ordered predecessor fingerprints? |
| preparation -> D1 -> D2 -> D3 `PseudoBir` | Do constraints bind ordinary values, D1 fork a separate closed pseudo schema, D2 alone add shared ABI-aware call transport, and D3 fully verify before publication? |
| `PseudoBir` -> D4 -> D5 | Does D4 perform every required target one-to-many expansion and full reverification, then does D5 remove all phi semantics using directly realizable edge/copy operations? |
| D5 -> E1 | Are liveness, interference, fixed homes, call/inline-asm roles, clobbers, and pressure derived from the exact fully reverified D5 candidate revision? |
| E1 -> E2 | Does E2 alone assign legal abstract category/class-or-group/slot homes using the exact E1 and target-pool keys? |
| E2 -> E3 -> E1 retry | Does E3 alone add explicit abstract spill identities and `Spill`/`Reload`; does every mutation advance and fully reverify the candidate before fresh E1/E2 facts? |
| stable E3 -> E4 -> `AllocatedBir` | Does E4 freeze one candidate, rerun cumulative graph/Pseudo/D5/allocation checks, reject mixed or stale products, and publish atomically? |
| `AllocatedBir` -> `PreparedBir` / `MirReadyBirView` -> MIR | Do all capabilities name the same immutable revision, with the readiness object owning no graph and the view only borrowing; does MIR perform same-key concrete mapping and one-to-one selection without ordinary allocation fallback? |

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
- Are `PreparedBir` and `MirReadyBirView` capabilities over the exact owning
  `AllocatedBir`, rather than copied or rebuilt instruction storage?

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
