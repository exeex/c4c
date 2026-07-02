# BIR Route8 Return-Chain Body Extraction Runbook

Status: Active
Source Idea: ideas/open/521_bir_route8_return_chain_body_extraction.md
Activated From: post-520 lifecycle activation

## Purpose

Continue the staged BIR core cleanup after render ownership by extracting the
route8 return-chain implementation bodies from central `bir.cpp` into a focused
owner while keeping the public BIR declaration surface stable.

## Goal

Move only the existing route8 return-chain bodies into a narrow translation
unit, preserving return-chain records, ordering, optional/nullopt decisions,
diagnostics, public declarations, and all callers.

## Core Rule

This is a behavior-preserving body extraction. Do not change route8 semantics,
route1 scalar producer identity, public headers, route6/route7/facade behavior,
tests, expectations, unsupported markers, or producer capability.

## Read First

- ideas/open/521_bir_route8_return_chain_body_extraction.md
- docs/bir_core_cleanup/follow_up_ideas.md
- docs/bir_core_cleanup/destination_map.md
- docs/bir_core_cleanup/structure_snapshot.md
- docs/bir_core_cleanup/declaration_inventory.md
- docs/bir_core_cleanup/implementation_inventory.md
- .codex/skills/c4c-clang-tools/SKILL.md
- src/backend/bir/bir.cpp
- src/backend/bir/bir.hpp
- relevant BIR/backend CMake metadata only if a new route8 translation unit is
  selected

## Current Targets

- Route8 return-chain implementation cluster currently in
  `src/backend/bir/bir.cpp`, especially the route8 return-chain index builder,
  record helpers, and public finders around the route8 cluster identified by
  the BIR cleanup artifacts.
- Candidate destination:
  - `src/backend/bir/bir_route8.cpp` for existing route8 return-chain bodies
  - an optional private route8 helper header only if AST-backed evidence proves
    it is needed for existing helper visibility
- Public declarations stay in `src/backend/bir/bir.hpp`.

## Non-Goals

- Do not move route8 declarations out of `bir.hpp`.
- Do not alter `Route1SourceValueIdentity`, route1 scalar producer behavior,
  integer constant handling, or value identity matching.
- Do not touch route6 call publication, route7 comparison, route-index facade,
  fused compare, materialized condition, or call-result helpers.
- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`,
  `MemoryAddress`, or broad route declarations.
- Do not implement or modify BIR producer capability from idea 422.
- Do not change tests, expectations, unsupported markers, allowlists, or
  pass/fail accounting.

## Working Model

Idea 518 identified route8 as the most leaf-like route family after the public
render helper cleanup. Route8 primarily publishes return-chain records and
finders, with route1 source identity as the main shared input. The extraction
should make route8 ownership explicit without hiding route1 dependency
direction or creating new coupling to route6, route7, or the route-index
facade.

## Execution Rules

- Use `.codex/skills/c4c-clang-tools/` before raw long-file reading.
- If `c4c-clang-tool` or `c4c-clang-tool-ccdb` is missing from `PATH`, run
  `scripts/build_install_c4c_clang_tools.sh` and retry from `PATH`.
- Start with AST-backed symbol, signature, caller/callee, and type-reference
  queries for route8 symbols and route1 identity dependencies.
- Inspect only the minimal raw source needed to confirm ambiguous query
  results.
- Keep edits scoped to route8 body ownership and build metadata required by the
  new translation unit.
- Prefer preserving declarations in `bir.hpp` over any header split.
- For any code change, run build proof plus focused route8/backend proof chosen
  by the supervisor; include backend link-time proof because this slice adds or
  repoints route8 definitions.

## Step 1: Audit Route8 Symbols And Dependencies

Goal: Determine the exact route8 bodies, public queries, direct callers,
callees, and route1 identity dependencies before moving code.

Actions:

- Confirm clang-tools availability.
- Run AST-backed symbol and function-signature queries on `bir.cpp` and
  `bir.hpp` for route8 return-chain functions and records.
- Query direct callers and callees for the route8 index builder, return-chain
  record helpers, and public finders.
- Query or inspect route1 identity dependencies used by route8 bodies.
- Inspect minimal raw source around route8 definitions and declarations only
  where AST results need confirmation.
- Record whether any route8 body depends on route6, route7, facade internals,
  or non-route8 private helpers.

Completion check:

- `todo.md` records the clang-tools commands, route8 symbol map, dependency
  map, caller/callee map, and whether route8 body extraction is safe as a
  focused TU.

## Step 2: Select Route8 Body Boundary

Goal: Choose the smallest body-only route8 destination that preserves public
declarations and dependency direction.

Actions:

- Compare the audit results against the 518 destination map and follow-up
  staging rules.
- Select `src/backend/bir/bir_route8.cpp` only if route8 bodies can move there
  without requiring route8 declaration movement or new route6/route7/facade
  coupling.
- Add an optional private helper header only if existing helper visibility
  cannot be preserved cleanly inside the new TU.
- If the audit proves route8 is not currently separable, preserve bodies in
  `bir.cpp` and record the reason instead of forcing a move.
- Do not edit implementation code until the destination decision is explicit.

Completion check:

- `todo.md` names the selected route8 boundary, any rejected alternatives, and
  the dependency rationale.

## Step 3: Apply Minimal Route8 Body Extraction Or Preserve Decision

Goal: Implement the selected behavior-preserving route8 ownership outcome.

Actions:

- If moving bodies, relocate only the selected route8 return-chain definitions
  and strictly required anonymous helpers into `bir_route8.cpp`.
- Preserve public declarations in `bir.hpp`.
- Add only required build metadata for the new translation unit and direct BIR
  tests that compile BIR sources manually.
- Keep return-chain record construction, ordering, optional/nullopt decisions,
  diagnostics, and route1 identity use unchanged.
- If preserving bodies, make no source changes unless a tiny comment is truly
  needed to prevent route drift.

Completion check:

- Code diff is limited to route8 body ownership and required build metadata.
- No header split, route1 semantic change, route6/route7/facade movement,
  producer capability, or test expectation change appears in the diff.

## Step 4: Validate Route8 Ownership

Goal: Prove behavior and linkage are unchanged.

Actions:

- Run `git diff --check`.
- Run a fresh build.
- Run focused return-chain/backend proof covering route8 public queries and
  callers when available.
- Run the delegated backend subset if no narrower route8 proof exists or if
  link-time coverage is needed for the new translation unit.

Completion check:

- Validation commands and results are recorded in `todo.md`.
- Any skipped focused proof is explained with the nearest broader substitute.
- If regression guard is run, record the before/after scope and result.

## Step 5: Handoff Or Close Readiness

Goal: Decide whether idea 521 can close or needs a follow-up adjustment.

Actions:

- Summarize the final disposition: moved to `bir_route8.cpp` or intentionally
  preserved in `bir.cpp`.
- Record residual risks and whether source idea acceptance criteria are
  satisfied.
- Confirm no separate cleanup initiative was discovered that belongs in
  `ideas/open/`.

Completion check:

- `todo.md` contains close-readiness notes.
- The source idea can be closed by lifecycle review after validation is
  accepted.
