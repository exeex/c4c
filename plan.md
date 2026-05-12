# Identity Authority Migration Closure Gate Runbook

Status: Active
Source Idea: ideas/open/171_identity_authority_migration_closure_gate.md
Activated After: idea 170 closure commit `82dbddc61`

## Purpose

Close the name and string identity-authority migration as a coherent milestone,
or identify the narrow follow-up that blocks closure.

Goal: verify that the parser/sema/HIR/LIR/BIR/backend identity migration has a
stable proof story, a usable regression guard, and no untracked semantic
string-authority leftovers.

Core Rule: this is a closure gate, not a new cleanup pass or the start of the
type-identity migration.

## Read First

- `ideas/open/171_identity_authority_migration_closure_gate.md`
- `ideas/closed/167_whole_codebase_string_authority_final_audit.md`
- `ideas/closed/168_compatibility_bridge_retirement.md`
- `ideas/closed/169_route_local_identity_domain_cleanup.md`
- `ideas/closed/170_string_authority_regression_guard.md`
- `scripts/string_authority_guard.py`
- `scripts/string_authority_classifications.json`
- `test_before.log` and `test_after.log` when present

## Current Scope

- Verify the accepted post-170 full-suite baseline and monotonic regression
  guard result.
- Verify the idea 170 string-authority guard is runnable by developers and has
  documented classification behavior.
- Reconcile the idea 167 audit inventory against closed bridge, route-local,
  and guard work.
- Produce a compact identity-domain closure ledger for `TextId`, structured
  semantic keys, `QualifiedNameKey`, owner-aware template keys, `LinkNameId`,
  route-local ids, and display text.
- Decide whether to close the migration or open a narrow follow-up for any
  remaining blocker.

## Non-Goals

- Do not start idea 172 or any type-identity migration work.
- Do not perform broad implementation refactors unless a concrete closure
  blocker requires a separate follow-up idea.
- Do not reopen already closed parser/sema/HIR/backend ideas without a
  specific regression or audit item.
- Do not classify route-local display strings as source or link identity bugs.
- Do not weaken tests, rewrite expectations, or downgrade supported behavior to
  claim closure.

## Working Model

- The supervisor has accepted the post-170 full-suite baseline:
  `3137/3137` with a passing monotonic regression guard.
- This runbook should collect and verify evidence, then leave durable closure
  information in the lifecycle artifacts and final closure note.
- Remaining string paths must be classified as structured authority,
  documented compatibility bridge, route-local identity, display/output,
  diagnostic/debug, ABI spelling, or follow-up work.
- If a blocker is discovered, record it as a narrow follow-up idea instead of
  expanding this gate into implementation work.

## Execution Rules

- Prefer `todo.md` for live evidence notes and proof results.
- Edit `plan.md` only if the closure route or proof contract changes.
- Edit the source idea only for lifecycle status, closure notes, or a durable
  dependency/link correction.
- Use existing guard and baseline artifacts before rerunning broad validation.
- Escalate to supervisor review if closure would require implementation work or
  a new open idea.

## Steps

### Step 1: Reconcile Closure Inputs

Goal: confirm the closure gate has all required prior evidence and dependency
artifacts.

Actions:
- Inspect the closed ideas listed in `Read First` and summarize the relevant
  ownership decisions in `todo.md`.
- Confirm the accepted post-170 full-suite baseline and monotonic regression
  guard result are available or explicitly supplied by the supervisor.
- Confirm there is no active `plan.md`/`todo.md` mismatch and this plan links
  only to idea 171.

Completion Check:
- `todo.md` records the accepted baseline evidence, closed dependency paths,
  and any missing artifact that would block closure.

### Step 2: Verify Guard Usability

Goal: prove the idea 170 guard is usable as the ongoing regression boundary.

Primary Targets:
- `scripts/string_authority_guard.py`
- `scripts/string_authority_classifications.json`
- `tests/CMakeLists.txt`

Actions:
- Identify the documented or CTest-backed command developers should run.
- Verify the guard classifications expose owner, domain, category, and reason
  for accepted hits.
- Run the supervisor-selected guard proof command when delegated.
- Record command and result in `todo.md`.

Completion Check:
- `todo.md` names the guard command, classification behavior, and fresh or
  accepted proof result.

### Step 3: Reconcile Remaining Identity Inventory

Goal: decide whether every remaining string-authority item is closed,
classified, out of scope, or a follow-up.

Actions:
- Review the idea 167 inventory against idea 168 compatibility bridge notes,
  idea 169 route-local identity notes, and idea 170 guard classifications.
- Classify remaining items as fixed, documented bridge, route-local identity,
  display/output, diagnostic/debug, ABI spelling, or follow-up candidate.
- Do not change implementation code during reconciliation.

Completion Check:
- `todo.md` contains a concise remaining-item ledger with no unclassified
  semantic string-authority entries.

### Step 4: Write Closure Ledger

Goal: produce the closure ledger required by the source idea.

Actions:
- Record which identity domain owns each class of name:
  `TextId`, structured semantic keys, `QualifiedNameKey`,
  owner-aware template keys, `LinkNameId`, route-local ids, and display text.
- Call out bridge owners, limitations, and removal conditions where bridges
  remain.
- Keep the ledger in `todo.md` during execution; move the compact durable
  result into the closure note only when the source idea is ready to close.

Completion Check:
- The ledger is reviewable and does not handwave remaining semantic string
  lookups.

### Step 5: Decide Closure Or Follow-Up

Goal: either close the identity-authority migration or create a narrow
follow-up idea for any true blocker.

Actions:
- Run or reference the supervisor-selected final proof for this lifecycle-only
  closure gate.
- If all acceptance criteria are satisfied, update idea 171 with closure notes,
  remove active lifecycle files, and move the idea to `ideas/closed/`.
- If a blocker remains, keep idea 171 open and create or request a narrow
  follow-up idea that describes the blocker and reject signals.

Completion Check:
- Closure is accepted with proof and durable closure notes, or the blocker is
  recorded without expanding this gate into unrelated implementation work.
