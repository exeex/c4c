# BIR Route1 Scalar Producer Body Extraction Runbook

Status: Active
Source Idea: ideas/open/522_bir_route1_scalar_producer_body_extraction.md
Activated From: post-521 lifecycle activation

## Purpose

Continue the staged BIR core cleanup after route8 by extracting the shared
route1 scalar producer implementation bodies from central `bir.cpp` into a
focused owner while keeping the public BIR declaration surface stable.

## Goal

Move only the existing route1 scalar producer bodies into a narrow translation
unit, preserving scalar producer identity, integer constant evaluation, source
identity matching, public declarations, and downstream route consumers.

## Core Rule

This is a behavior-preserving body extraction. Do not change producer
semantics, constant folding, value matching, public headers, route2/route4/
route5/route6/route7/route8 behavior, tests, expectations, unsupported
markers, or producer capability.

## Read First

- ideas/open/522_bir_route1_scalar_producer_body_extraction.md
- docs/bir_core_cleanup/follow_up_ideas.md
- docs/bir_core_cleanup/destination_map.md
- docs/bir_core_cleanup/structure_snapshot.md
- docs/bir_core_cleanup/declaration_inventory.md
- docs/bir_core_cleanup/implementation_inventory.md
- .codex/skills/c4c-clang-tools/SKILL.md
- src/backend/bir/bir.cpp
- src/backend/bir/bir.hpp
- src/backend/bir/bir_private.hpp
- relevant BIR/backend CMake metadata only if a new route1 translation unit is
  selected

## Current Targets

- Route1 scalar producer implementation cluster currently in
  `src/backend/bir/bir.cpp`, especially producer identity construction,
  integer constant helpers, source identity matching, and public producer query
  helpers identified by the BIR cleanup artifacts.
- Candidate destination:
  - `src/backend/bir/bir_route1.cpp` for existing route1 scalar producer bodies
  - a narrow private helper boundary only if AST-backed evidence proves
    existing helper visibility cannot be preserved cleanly inside the new TU
- Public declarations stay in `src/backend/bir/bir.hpp`.

## Non-Goals

- Do not move route1 declarations out of `bir.hpp`.
- Do not alter scalar producer semantics, integer constant evaluation, source
  identity matching, or value identity matching.
- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`,
  `MemoryAddress`, broad route declarations, or constructors.
- Do not combine route1 extraction with route2, route4, route5, route6,
  route7, route8, or route-index facade behavior changes.
- Do not implement or modify BIR producer capability from idea 422.
- Do not change tests, expectations, unsupported markers, allowlists, or
  pass/fail accounting.

## Working Model

Idea 518 identified route1 as the shared scalar producer boundary for later
route extraction. Routes 2, 4, 5, 6, 7, and 8 consume route1 producer facts, so
this extraction should make route1 ownership explicit without changing the
direction or meaning of downstream dependencies.

## Execution Rules

- Use `.codex/skills/c4c-clang-tools/` before raw long-file reading.
- If `c4c-clang-tool` or `c4c-clang-tool-ccdb` is missing from `PATH`, run
  `scripts/build_install_c4c_clang_tools.sh` and retry from `PATH`.
- Start with AST-backed symbol, signature, caller/callee, and type-reference
  queries for route1 scalar producer symbols and downstream route consumers.
- Inspect only the minimal raw source needed to confirm ambiguous query
  results.
- Keep edits scoped to route1 body ownership and build metadata required by the
  new translation unit.
- Prefer preserving declarations in `bir.hpp` over any header split.
- For any code change, run build proof plus focused backend proof chosen by the
  supervisor; include downstream route consumer coverage because route1 feeds
  routes 2, 4, 5, 6, 7, and 8.

## Step 1: Audit Route1 Symbols And Dependencies

Goal: Determine the exact route1 bodies, public queries, direct callers,
callees, type references, and downstream consumers before moving code.

Actions:

- Confirm clang-tools availability.
- Run AST-backed symbol and function-signature queries on `bir.cpp`,
  `bir.hpp`, and `bir_private.hpp` for route1 scalar producer functions,
  identity records, and integer constant helpers.
- Query direct callers and callees for producer identity construction, source
  identity matching, integer constant helpers, and public producer finders.
- Query downstream route consumers across route2, route4, route5, route6,
  route7, and route8.
- Inspect minimal raw source around route1 definitions and declarations only
  where AST results need confirmation.
- Record whether any route1 body depends on route-specific implementation
  internals or non-route1 private helpers.

Completion check:

- `todo.md` records the clang-tools commands, route1 symbol map, dependency
  map, caller/callee map, downstream consumer map, and whether route1 body
  extraction is safe as a focused TU.

## Step 2: Select Route1 Body Boundary

Goal: Choose the smallest body-only route1 destination that preserves public
declarations and downstream dependency direction.

Actions:

- Compare the audit results against the 518 destination map and follow-up
  staging rules.
- Select `src/backend/bir/bir_route1.cpp` only if route1 bodies can move there
  without requiring route1 declaration movement or downstream route behavior
  changes.
- Add a private helper boundary only if existing helper visibility cannot be
  preserved cleanly inside the new TU.
- If the audit proves route1 is not currently separable, preserve bodies in
  `bir.cpp` and record the reason instead of forcing a move.
- Do not edit implementation code until the destination decision is explicit.

Completion check:

- `todo.md` names the selected route1 boundary, any rejected alternatives, and
  the dependency rationale.

## Step 3: Apply Minimal Route1 Body Extraction Or Preserve Decision

Goal: Implement the selected behavior-preserving route1 ownership outcome.

Actions:

- If moving bodies, relocate only the selected route1 scalar producer
  definitions and strictly required anonymous helpers into `bir_route1.cpp`.
- Preserve public declarations in `bir.hpp`.
- Add only required build metadata for the new translation unit and direct BIR
  tests that compile BIR sources manually.
- Keep producer identity construction, integer constant evaluation, source
  identity matching, value matching, optional/nullopt decisions, and diagnostics
  unchanged.
- If preserving bodies, make no source changes unless a tiny comment is truly
  needed to prevent route drift.

Completion check:

- Code diff is limited to route1 body ownership and required build metadata.
- No header split, producer semantic change, downstream route behavior change,
  producer capability, or test expectation change appears in the diff.

## Step 4: Validate Route1 Ownership

Goal: Prove behavior, downstream consumers, and linkage are unchanged.

Actions:

- Run `git diff --check`.
- Run a fresh build.
- Run focused backend proof covering producer-index consumers, including
  downstream route4, route5, route6, route7, and route8 paths where available.
- Run the delegated backend subset if no narrower producer-consumer proof
  exists or if link-time coverage is needed for the new translation unit.
- Record any regression guard before/after scope and result if the supervisor
  runs one for acceptance.

Completion check:

- Validation commands and results are recorded in `todo.md`.
- Any skipped focused proof is explained with the nearest broader substitute.
- If regression guard is run, record the before/after scope and result.

## Step 5: Handoff Or Close Readiness

Goal: Decide whether idea 522 can close or needs a follow-up adjustment.

Actions:

- Summarize the final disposition: moved to `bir_route1.cpp` or intentionally
  preserved in `bir.cpp`.
- Record residual risks and whether source idea acceptance criteria are
  satisfied.
- Confirm no separate cleanup initiative was discovered that belongs in
  `ideas/open/`.

Completion check:

- `todo.md` contains close-readiness notes.
- The source idea can be closed by lifecycle review after validation is
  accepted.
