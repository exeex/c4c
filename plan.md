# String Authority Regression Guard Runbook

Status: Active
Source Idea: ideas/open/170_string_authority_regression_guard.md

## Purpose

Add a lightweight regression guard that catches suspicious new semantic string
authority without banning ordinary `std::string` use.

Goal: make new string-keyed semantic lookup paths require explicit
classification as structured authority, compatibility bridge, display/output,
diagnostic/debug, or route-local identity.

Core Rule: guard against semantic identity drift, not against strings used for
text, diagnostics, final output, local generated names, or documented bridges.

## Read First

- `ideas/open/170_string_authority_regression_guard.md`
- `ideas/closed/167_whole_codebase_string_authority_final_audit.md`
- `ideas/closed/168_compatibility_bridge_retirement.md`
- `ideas/closed/169_route_local_identity_domain_cleanup.md`
- Existing repo workflow scripts under `scripts/`
- Current string-authority comments and classifications in `src/`

## Current Scope

- Suspicious semantic string-keyed containers such as
  `unordered_map<std::string, ...>`, `map<std::string, ...>`,
  `*_by_name`, `*_name_map`, and broad rendered-name lookup helpers.
- A repo-native allowlist or classification file with owner, domain, category,
  and reason for each accepted hit.
- A repeatable local guard command and focused tests that prove representative
  semantic string authority additions are caught.
- Documentation or command help that tells developers how to classify valid
  display, diagnostic, compatibility, route-local, and false-positive strings.

## Non-Goals

- Do not build a perfect static analyzer.
- Do not ban diagnostics, printers, ABI spelling, route-local generated names,
  string pools, or final output text.
- Do not reopen compatibility bridges or route-local identity cleanup by hand.
- Do not weaken existing tests or reclassify real semantic authority as display
  to reduce guard noise.
- Do not make the guard so broad that normal development requires routine
  bypasses.

## Working Model

- The guard should scan source text with conservative patterns first, then use
  explicit classifications to decide which hits are accepted.
- Classification data is part of the guard contract. Each allowlisted hit must
  name its owner, domain, category, and why the string is not ordinary semantic
  authority.
- A suspicious unclassified hit should fail with enough context for the author
  to either add a real structured domain or classify the retained string use.
- Regression tests should exercise both sides: known valid classified strings
  are accepted, and representative new semantic maps are rejected.

## Execution Rules

- Inventory current hits before enforcing failures.
- Seed classifications from ideas 167, 168, and 169; do not re-litigate closed
  classifications unless the code no longer matches them.
- Keep the first implementation narrow and repo-native, preferably under
  `scripts/` plus a test hook or CTest target if that matches nearby patterns.
- Prefer clear, reviewable allowlist data over hidden regex exceptions.
- Add build or test proof for every code-changing step. Escalate to broader
  validation when the guard becomes part of default developer or CTest workflow.

## Steps

### Step 1: Inventory Guard Candidates

Goal: identify the current suspicious string-authority patterns and decide the
initial guard surface.

Primary Targets:
- `src/frontend/parser/`
- `src/frontend/sema/`
- `src/frontend/hir/`
- `src/codegen/lir/`
- `src/backend/`
- `src/codegen/shared/`

Actions:
- Search for string-keyed containers, lookup helpers, `find(name)` style paths,
  `*_by_name`, `*_name_map`, and rendered-name fallback helpers.
- Classify each meaningful hit as structured authority, compatibility bridge,
  display/output, diagnostic/debug, route-local identity, generated temporary
  name, ABI spelling, or false positive.
- Record the chosen first guard patterns and expected false positives in
  `todo.md`, not in the source idea.

Completion Check:
- `todo.md` names the initial scanned patterns, the chosen guard categories,
  and the classification data shape the executor should implement.

### Step 2: Add Classification Data

Goal: create the explicit allowlist or classification artifact consumed by the
guard.

Actions:
- Add a reviewable repo-owned classification file or embedded data format with
  owner, domain, category, reason, and removal condition when applicable.
- Seed it from the current inventory and the closed idea 167, 168, and 169
  classifications.
- Keep broad exceptions out of the data unless they are justified by a real
  domain boundary.

Completion Check:
- Existing accepted string uses are classified with enough detail for review.
- Adding a new suspicious hit without classification would be observable by the
  planned guard.

### Step 3: Implement The Local Guard

Goal: provide a repeatable local command that fails on unclassified suspicious
string-authority patterns.

Actions:
- Add the smallest repo-native script or check that scans the covered source
  regions and reads the classification data.
- Report failures with file, line, matched pattern, and required classification
  fields.
- Include representative fixtures or self-tests that prove an obvious semantic
  `unordered_map<std::string, ...>` addition is rejected while classified
  display, diagnostic, compatibility, and route-local cases are accepted.

Completion Check:
- The guard command runs locally and fails for an unclassified representative
  semantic string-keyed lookup pattern.
- Fresh script/test proof passes.

### Step 4: Integrate Developer Workflow

Goal: make the guard visible without making normal string use painful.

Actions:
- Wire the guard into an appropriate CTest target, script, or documented
  developer command based on existing repo conventions.
- Document how to classify valid strings and what evidence reviewers should
  expect for new allowlist entries.
- Keep the integration narrow enough that unrelated diagnostics or final output
  strings do not routinely block work.

Completion Check:
- Developers can run the guard from a documented command.
- The command is included in the selected local validation path or CTest target.

### Step 5: Prove Guard Coverage And Non-Disruption

Goal: verify the guard catches the intended drift and does not regress normal
compiler validation.

Actions:
- Run the guard command and its focused tests.
- Run the supervisor-selected build and test subset after integration.
- If the guard is wired into broad validation, run the matching broader or full
  checkpoint requested by the supervisor.
- Record proof commands and results in `todo.md`.

Completion Check:
- Guard proof passes.
- Build or CTest proof passes.
- Any failures are classified as guard issues to fix, pre-existing unrelated
  failures, or source-idea blockers requiring supervisor review.
