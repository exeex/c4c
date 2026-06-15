Status: Active
Source Idea Path: ideas/open/268_phase_f4_prepared_bir_module_liveness_authority_blocker_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Decide Later Work Eligibility

# Current Packet

## Just Finished

Step 4, `Decide Later Work Eligibility`, converted the Step 2 and Step 3
blocker map into a later-work decision without implementation changes.

Decision:
- Later private-pass-context, demotion, wrapper, migration, deletion, public
  field hiding, or implementation work is blocked for this runbook.
- The blocker is evidentiary, not mechanical: Step 2 found no exact
  identity-only reader and no independent semantic liveness fact outside the
  public prepared `PreparedBirModule::liveness` row produced by
  `BirPreAlloc::run_liveness()`.
- Step 3 found the fail-closed map blocker-heavy. Absent/skipped rows have only
  partial helper/status fail-closed behavior; stale rows lack freshness or
  generation checks; mismatch handling is local and inconsistent across
  readers; duplicate/conflict rows lack uniqueness or conflict diagnostics;
  unsupported rows lack a liveness-row contract; fallback rows are not globally
  distinguishable from complete authority; derived printer, route-debug,
  helper/status, regalloc/value-location, call-plan, wrapper, fallback, and
  exact target-output surfaces remain compatibility authority.

Exact blockers tied to evidence:
- No exact identity-only reader exists. Current readers either publish or derive
  target-policy, printer/debug/status, fallback, wrapper, helper, regalloc,
  value-location, call-plan, or exact-output behavior. Moving liveness behind a
  private pass context, wrapper, or demoted field would hide the only current
  semantic authority rather than proving an identity-only mirror.
- No independent semantic liveness fact exists. The current semantic fact is
  the public prepared liveness row itself. A later packet must not relabel that
  row as BIR, route, private accessor, or compatibility-helper authority unless
  it first proves the new fact is semantically independent and the public row is
  only a compatibility mirror.
- Fail-closed coverage is incomplete. There is no central row validator proving
  row coverage, row freshness, row/module consistency, uniqueness,
  unsupported-state handling, or marked fallback behavior before downstream
  publication.
- Compatibility surfaces still depend on current behavior. Prepared printer
  text, route-debug filtering, helper/oracle/status text, fallback behavior,
  wrapper behavior when present, regalloc/value-location summaries, call plans,
  and exact target output must stay stable unless a separate lifecycle idea
  proves and migrates a replacement authority.

Separate lifecycle initiative decision:
- A separate lifecycle initiative is not required immediately to complete this
  blocker-map runbook. Closure of the current map is sufficient because the
  correct output is a negative eligibility decision: no later implementation
  packet is authorized from this evidence.
- If the supervisor wants progress toward eventual implementation, the bounded
  follow-up initiative should be named `Prepared liveness row validation and
  semantic authority replacement`. That future idea should be scoped to proving
  a semantic liveness authority, one identity-only reader, retained
  compatibility surfaces, and central fail-closed behavior before any demotion
  or migration.

Minimum conditions for any future implementation idea:
- Name one exact identity-only reader and the identity it consumes.
- Name one independent semantic liveness fact that owns meaning, and prove the
  public prepared liveness row is only a retained compatibility mirror.
- Add or prove central fail-closed contracts for absent/skipped, stale,
  mismatch, duplicate/conflict, unsupported, fallback, and derived
  printer/target behavior.
- Preserve existing public prepared compatibility surfaces until a separate
  lifecycle-authorized migration proves their replacement: prepared printer,
  route-debug, helper/oracle/status, fallback, wrapper, regalloc/value-location,
  call-plan, and exact target-output behavior.
- Avoid expectation downgrades, helper/status relabeling, route-debug/printer
  output rewrites, named-fixture proof, or target-policy movement into BIR as a
  substitute for semantic proof.

## Suggested Next

Execute Step 5 from `plan.md`: validate that the map-only lifecycle slice did
not weaken implementation files, expectations, helper/oracle/status output,
fallback names, route-debug output, prepared-printer output, wrapper output,
exact target output, unsupported behavior, or baselines. The expected Step 5
handoff is to let the supervisor decide whether reviewer scrutiny is needed
before closure or before creating any separate follow-up idea.

## Watchouts

- This active plan is analysis and blocker mapping only.
- Do not demote, delete, privatize, wrap, migrate, or implement
  `PreparedBirModule::liveness`.
- Do not weaken printer, route-debug, helper/oracle/status, fallback, wrapper,
  exact target-output, unsupported, or baseline behavior.
- Do not claim progress through expectation rewrites, helper renames,
  classification-only notes, or named-fixture proof.
- Step 2 did not find an exact identity-only reader. Step 3 did not find a
  complete fail-closed row contract. Treat public `PreparedBirModule::liveness`
  as blocked semantic authority.
- `PrepareOptions::run_liveness` remains present but is not honored by
  production `BirPreAlloc::run()`; manual staged tests use it as phase-control
  compatibility.
- Do not infer implementation eligibility from helper missing facts alone:
  helper status is the strongest fail-closed evidence, but regalloc,
  value-location, call-plan, route-debug, printer, and target-output surfaces do
  not share a central liveness row validator.
- The only bounded implementation-enabling follow-up identified here is a
  separate lifecycle idea for prepared liveness row validation and semantic
  authority replacement. This runbook does not authorize starting that work.

## Proof

Analysis-only packet. Delegated proof command:
- read back `todo.md`
- `git status --short`
- `git diff --check`

No build or CTest was required by the packet because only `todo.md` changed.
Result: `todo.md` readback completed, `git status --short` reported only
`M todo.md`, and `git diff --check` passed with no whitespace errors.
