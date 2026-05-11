# Sema Type Utils Static Eval Structured Lookup Runbook

Status: Active
Source Idea: ideas/open/164_sema_type_utils_static_eval_structured_lookup.md

## Purpose

Retire the remaining ordinary rendered-name authority in sema `type_utils`
static integer evaluation while preserving explicit no-metadata compatibility.

## Goal

Make `static_eval_int` prefer structured or `TextId`-aware enum constant
lookup for covered callers, fail closed on complete structured misses, and
retain `unordered_map<string, long long>` lookup only as a documented legacy
bridge.

## Core Rule

Do not replace one raw spelling key with another incomplete key. Any covered
structured lookup must carry enough enum/domain context to prevent
same-spelled constants from colliding.

## Read First

- Source idea: `ideas/open/164_sema_type_utils_static_eval_structured_lookup.md`
- Static eval implementation: `src/frontend/sema/type_utils.cpp`
- Static eval declarations and call sites for `static_eval_int`
- Recent consteval and validate authority work from ideas 158 and 159, only as
  needed to match existing metadata patterns
- Focused sema tests that exercise enum constants, default expressions, and
  same-spelled names

## Current Scope

- Inventory every `static_eval_int` caller and classify whether it can provide
  structured enum metadata, `TextId` metadata, or only rendered compatibility
  data.
- Add a structured enum constant lookup path in `type_utils`.
- Prefer the structured path when metadata is complete.
- Make complete structured misses fail closed instead of falling through to
  rendered `n->name` lookup.
- Document any retained string map as a compatibility bridge with owner,
  limitation, and removal condition.
- Add focused sema coverage for same-spelled enum constants that must not
  collide during static evaluation.

## Non-Goals

- Do not rework the full consteval interpreter from idea 159.
- Do not reopen validate table ownership from idea 158.
- Do not remove diagnostic or source spelling strings.
- Do not make broad HIR or backend changes.
- Do not claim progress through expectation rewrites or diagnostic spelling
  changes alone.

## Working Model

- `static_eval_int` currently has a rendered enum constant bridge using
  `unordered_map<string, long long>` and `n->name`.
- Covered callers should pass structured enum identity, scoped enum-domain
  data, or `TextId` plus enough domain context to disambiguate source identity.
- Raw `TextId` alone is not sufficient where enum/domain scope matters.
- Rendered strings can remain for no-metadata callers, diagnostics, source
  spelling, and a documented compatibility path.

## Execution Rules

- Keep source-idea intent unchanged unless a real lifecycle correction is
  needed.
- Update `todo.md` with caller inventory, chosen first packet, and proof as
  execution proceeds.
- Prefer small mechanical call-site conversions over broad sema rewrites.
- When adding lookup helpers, keep ownership and fail-closed behavior explicit
  at the API boundary.
- Every code-changing step needs fresh build or focused sema proof. Escalate to
  broader validation if the helper signature touches many sema callers.

## Ordered Steps

### Step 1: Inventory Static Eval Callers

Goal: identify every path that still feeds rendered enum constants into
`static_eval_int`.

Primary targets:
- `src/frontend/sema/type_utils.cpp`
- declarations and call sites for `static_eval_int`
- enum/default-expression sema tests

Actions:
- Find all `static_eval_int` declarations, definitions, wrappers, and callers.
- Classify each caller as structured-metadata capable, `TextId` capable,
  rendered-compatibility only, or unrelated to enum constants.
- Identify the narrowest first caller group that can prove structured lookup
  without changing unrelated sema behavior.
- Record the caller inventory and first packet target in `todo.md`.

Completion check:
- `todo.md` names each caller group and the selected first implementation
  target.
- No implementation or test files are edited before the inventory is recorded.

### Step 2: Add a Structured Enum Lookup Input

Goal: define the structured lookup contract for enum constants evaluated by
`static_eval_int`.

Primary targets:
- `src/frontend/sema/type_utils.cpp`
- matching headers or helper declarations used by `static_eval_int`

Actions:
- Inspect existing enum metadata, `TextId`, and scoped-domain helper types
  available near sema type utilities.
- Add or reuse a small structured enum-constant lookup representation that can
  disambiguate same-spelled constants across enum domains.
- Keep the rendered `unordered_map<string, long long>` path available only as
  an explicit compatibility input.
- Make the API communicate whether structured metadata is complete for a
  lookup attempt.

Completion check:
- The structured lookup input is available to `static_eval_int` without forcing
  unrelated callers through fabricated metadata.
- Existing no-metadata callers still compile through the compatibility bridge.

### Step 3: Prefer Structured Lookup and Fail Closed

Goal: make covered enum constant evaluation use structured identity before
rendered names.

Primary targets:
- `static_eval_int` name/enum-constant handling in
  `src/frontend/sema/type_utils.cpp`
- covered caller group selected in Step 1

Actions:
- Resolve enum constants through the structured input when metadata is
  complete.
- If complete structured metadata misses, report failure for that expression
  instead of falling through to `enum_consts.find(n->name)`.
- Use rendered-name lookup only for callers explicitly classified as
  no-metadata compatibility paths.
- Add concise comments only at the compatibility boundary, naming the owner,
  limitation, and removal condition.

Completion check:
- Covered paths no longer use rendered spelling as the ordinary enum authority.
- The string map is visibly fenced as compatibility, not the default semantic
  path.

### Step 4: Convert Covered Callers

Goal: pass structured enum metadata from all callers that can provide it.

Primary targets:
- caller sites identified as structured-metadata capable in Step 1
- sema helpers that already own enum/domain metadata

Actions:
- Convert one caller group at a time to populate the structured lookup input.
- Avoid broad helper rewrites unless they remove real duplication across the
  converted callers.
- Keep rendered-only callers on the compatibility path with a written
  justification in `todo.md`.
- After each conversion, run the narrow build or sema test proof delegated by
  the supervisor.

Completion check:
- All covered callers pass structured metadata.
- Remaining rendered-only callers are documented as compatibility callers with
  a removal condition.

### Step 5: Add Collision Coverage

Goal: prove same-spelled enum constants do not collide in `type_utils` static
evaluation.

Primary targets:
- focused sema tests for enum/default/static-eval expressions

Actions:
- Add or update tests where same-spelled enum constants exist in distinct
  structured domains.
- Ensure the test observes lookup identity or evaluation behavior, not only
  diagnostic spelling.
- Include a negative or fail-closed case if a complete structured miss is
  reachable through existing sema diagnostics.

Completion check:
- Focused tests fail against rendered-name authority and pass through
  structured lookup.
- The proof demonstrates domain separation rather than output text churn.

### Step 6: Final Proof and Closure Readiness

Goal: prepare the idea for supervisor review and eventual closure.

Actions:
- Re-run the focused sema proof covering static evaluation and enum constants.
- Run broader build or sema validation if the helper signature touched multiple
  sema areas.
- Confirm `static_eval_int` retains rendered lookup only behind the documented
  compatibility boundary.
- Record final proof, retained compatibility callers, and any residual blocker
  in `todo.md`.

Completion check:
- Source idea acceptance criteria are satisfied or exact blockers are recorded
  for lifecycle decision.
- No source-level enum/default static-eval covered path still treats
  `n->name` as ordinary lookup authority.
