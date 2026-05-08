# Parser Out Of Class Owner Probe Token Sequence Runbook

Status: Active
Source Idea: ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md

## Purpose

Remove rendered-string authority from parser out-of-class member and constructor owner probing.

Goal: make out-of-class owner probing preserve token `TextId` sequences and structured qualified-owner metadata so constructor, destructor, conversion, operator, and member-owner decisions do not depend on rendered `qualified_owner` strings.

## Core Rule

`TextId` is spelling identity only. Compound owner meaning must live in structured token sequences, qualified-owner keys, and domain tables; rendered owner spelling is display or temporary compatibility only.

## Read First

- `ideas/open/151_parser_out_of_class_owner_probe_token_sequence.md`
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/core.cpp`
- `src/frontend/parser/impl/types/template.cpp`
- Relevant Sema/HIR owner-carrier consumers discovered from the parser owner-probe path

## Current Targets

- `probe_special_member_owner` and `consume_special_member_owner` in `src/frontend/parser/impl/declarations.cpp`.
- Nearby out-of-class declaration paths that build or consume `qualified_owner`, `ctor_name`, `qualified_ctor_name`, or equivalent rendered owner spelling.
- Constructor, destructor, conversion, operator, and member-owner shape checks that compare spelling instead of structured token components.
- Narrow Sema/HIR consumer paths that must receive resolved owner identity or structured deferred owner carriers.

## Non-Goals

- Do not remove every parser `token_spelling(...)` call.
- Do not redesign full C++ member lookup, overload resolution, or declaration semantics.
- Do not force all owner resolution to finish in Sema before HIR.
- Do not treat a single `TextId` containing `A::B` as semantic owner identity.
- Do not remove diagnostic or debug owner rendering.
- Do not weaken tests, mark out-of-class cases unsupported, or add named-case shortcuts.
- Do not mix broad backend, HIR, or declaration rewrites into this route.

## Working Model

- Parser recognizes declaration shape from source token sequence and emits structured owner-probe carriers.
- Structured owner data should include owner qualifier `TextId` segments, base/member `TextId`, global qualifier state, source spans where available, and a qualified-owner or domain key when the parser can produce one.
- Sema resolves non-dependent owner identity through domain tables and carries dependent/template owners forward structurally.
- HIR consumes resolved owner identity or structured deferred owner carriers; it does not split or compare rendered owner strings to rediscover owner identity.
- Rendered owner spelling may remain as diagnostics, debug output, legacy mirrors, or compatibility adapters with a clear removal condition.

## Execution Rules

- Start with inventory before changing code; classify each spelling use as semantic authority, display/syntax behavior, compatibility, or unrelated.
- Keep parser spelling uses that are source-literal handling, keyword-like extension syntax, diagnostics, debug output, or legacy name mirrors outside this route unless they feed owner identity.
- Replace decision authority before deleting compatibility rendering.
- Prefer one semantic rule over testcase-shaped matching for constructors, destructors, conversion functions, operators, and nested owners.
- For every code-changing step, run a fresh build or narrower compile proof chosen by the supervisor.
- Escalate validation when a step touches shared declaration parsing, owner lookup, or HIR lowering behavior.

## Step 1: Inventory Owner String Authority

Goal: identify every current parser decision path where rendered owner spelling controls out-of-class owner or special-member classification.

Primary target:
- `src/frontend/parser/impl/declarations.cpp`

Actions:
- Inspect `probe_special_member_owner`, `consume_special_member_owner`, and nearby out-of-class declaration handling.
- List every use of `qualified_owner`, `out_owner_segments`, `out_owner_text_ids`, `ctor_name`, `qualified_ctor_name`, and `token_spelling(...)` that can influence constructor, destructor, conversion, operator, or member-owner decisions.
- Classify each finding as semantic authority, compatibility/display, syntax handling, or unrelated.
- Check nearby qualified-name carriers in `src/frontend/parser/impl/core.cpp` only enough to know which structured metadata already exists.
- Record the first implementation packet boundary and supervisor-delegated proof command in `todo.md` before code changes begin.

Completion check:
- `todo.md` names the concrete first implementation packet, owned files, and proof command.
- The inventory distinguishes suspicious owner-probe spelling from allowed parser spelling uses.
- No source idea edit is needed unless the inventory contradicts durable source intent.

## Step 2: Define Structured Owner-Probe Carrier

Goal: make parser owner probing return structured data instead of making rendered owner strings the decision key.

Actions:
- Introduce or adapt a probe result that carries owner qualifier `TextId` segments, base/member `TextId`, global qualifier state, source spans where available, and structured qualified-owner key metadata when available.
- Preserve rendered owner spelling only as display, diagnostics, debug output, or temporary compatibility.
- Keep compatibility fields clearly secondary so new code cannot treat them as semantic authority.
- Avoid representing `A::B` as a single semantic owner token.

Completion check:
- The parser compiles with structured owner-probe data available at the out-of-class declaration decision points.
- Existing rendered owner output, if still present, is not the primary identity key for new owner-probe decisions.

## Step 3: Migrate Constructor And Special-Member Classification

Goal: make constructor, destructor, conversion, operator, and member syntax classification compare structured token components.

Actions:
- Replace constructor checks such as `token_spelling(parser.peek(1)) == seg` with direct `TextId` or structured segment comparison.
- Update destructor, conversion, operator, and member-owner checks that currently depend on rendered `qualified_owner` or flattened spelling.
- Keep grammar-shape classification in parser, but leave final semantic owner truth to Sema or deferred structured carriers.
- Keep `qualified_ctor_name` or similar strings only as display/legacy mirrors if still needed.

Completion check:
- Out-of-class special-member classification no longer succeeds primarily through rendered owner string comparison.
- Nested-owner and operator syntax use the same structured path, not separate named-case shortcuts.

## Step 4: Thread Structured Owner Identity To Consumers

Goal: ensure downstream owner resolution receives structured identity or deferred owner carriers instead of rendered owner spelling.

Actions:
- Inspect Sema/HIR consumers reached by the changed parser owner-probe path.
- Pass resolved owner identity where available and structured deferred owner carriers for dependent/template cases.
- Remove or isolate consumer code that splits or compares rendered owner strings for owner identity.
- Document any retained compatibility route with its owner, limitation, and removal condition.

Completion check:
- Required Sema/HIR consumers compile against the structured owner data.
- Remaining rendered owner spelling is classified as display, diagnostics, syntax, or temporary compatibility.

## Step 5: Add Focused Owner-Probe Tests Or Probes

Goal: prove the route fixes a case that flattened rendered-owner authority could misclassify.

Actions:
- Add or update focused coverage for nested owners, same-spelling local names, operator declarations, constructors, destructors, or conversion functions.
- Prefer a case that would fail or become ambiguous under flattened `A::B` owner text.
- Avoid unsupported expectation downgrades or expectation-only progress.
- Check nearby same-feature cases enough to reject a named-fixture shortcut.

Completion check:
- The focused test or probe exercises structured owner identity rather than only matching current output text.
- Nearby constructor/operator/nested-owner behavior has been considered in `todo.md`.

## Step 6: Validate And Retire Compatibility Authority

Goal: verify acceptance criteria and leave any remaining compatibility spelling routes explicit.

Actions:
- Re-run the supervisor-selected proof command and any broader validation required by the touched parser/Sema/HIR surfaces.
- Inventory remaining `qualified_owner` rendered comparisons, owner-spelling splits, and spelling-derived semantic lookup keys touched by this idea.
- Remove obsolete compatibility adapters when all active call sites use structured carriers.
- Record remaining compatibility limits and removal conditions in `todo.md`.

Completion check:
- Source idea acceptance criteria are satisfied or remaining work is explicitly recorded for lifecycle review.
- Regression proof is fresh.
- Reviewer reject signals have been checked, especially renamed string authority, `TextId`-alone compound owner identity, named-case shortcuts, unsupported downgrades, and rendered-owner HIR lookup.
