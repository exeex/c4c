# BIR Route3 Memory-Access Body Extraction Runbook

Status: Active
Source Idea: ideas/open/525_bir_route3_memory_access_body_extraction.md

## Purpose

Extract route3 memory-access record construction and query implementation
bodies into a focused owner while preserving the public route3 surface and
route6 access to route3 facts.

## Goal

Move existing route3 memory-access implementation bodies out of the current
monolithic owner without changing memory-access semantics, public declarations,
or route6 publication behavior.

## Core Rule

This is a behavior-preserving body extraction. Do not change `MemoryAddress`,
memory provenance, object extent, byte range, storage authority declarations, or
route6 call-publication source policy.

## Read First

- `ideas/open/525_bir_route3_memory_access_body_extraction.md`
- `src/backend/bir/bir.hpp`
- Current route3 memory-access implementation bodies and direct callers
- Route6 call-publication consumers of route3 memory facts
- `.codex/skills/c4c-clang-tools/SKILL.md`

## Current Targets / Scope

- Route3 memory-access record construction bodies.
- Route3 memory-access public query bodies.
- A new `bir_route3_memory.cpp` only if it is the narrowest focused owner.
- Private helper declarations only when needed for the body move.
- Build and focused proof covering memory-access sources and route6 consumers.

## Non-Goals

- Do not move public route3 declarations out of `bir.hpp`.
- Do not move route3 into `src/backend/bir/lir_to_bir/memory/`.
- Do not change memory provenance, storage authority, object extent, byte
  range, or `MemoryAddress` semantics.
- Do not change route6 call-argument publication source policy.
- Do not split public memory model headers in this runbook.
- Do not rewrite tests, expectations, unsupported markers, or contracts to make
  the move pass.

## Working Model

Route3 is BIR-side indexing over lowered blocks. It publishes memory-access
facts that route6 and other consumers may query. The implementation owner can be
split from the monolithic body file, but public model authority remains in
`bir.hpp` for this idea.

## Execution Rules

- Use AST-backed `c4c-clang-tools` queries before raw long-file reading for
  symbol discovery, signatures, direct callers/callees, and type references.
- Keep public names, signatures, namespaces, enum values, record ordering, and
  query behavior identical.
- Prefer moving complete existing bodies over rewriting logic.
- Preserve route6 access by compiling and testing route6 consumers after the
  move.
- Keep the patch narrow: source movement plus required build wiring only.
- If the move exposes a missing semantic producer fact, stop and report it as a
  separate idea instead of repairing it inside this cleanup slice.

## Ordered Steps

### Step 1: Map route3 memory-access symbols and dependencies

Goal: Establish the exact route3 memory-access bodies, helpers, type
references, and route6 consumers before editing code.

Primary target: route3 memory-access implementation and public declarations.

Actions:

- Load `.codex/skills/c4c-clang-tools/SKILL.md`.
- Confirm the repo-local `c4c-clang-tool` and `c4c-clang-tool-ccdb` binaries
  are available.
- Query top-level BIR symbols related to route3 memory access.
- Query signatures, direct callers/callees, and type references for the route3
  memory-access records and public queries.
- Identify route6 call-publication tests or backend subsets that consume the
  same route3 facts.
- Record concise query results and the proposed move boundary in `todo.md`.

Completion check:

- `todo.md` names the route3 bodies to move, any required private helpers, the
  route6 consumers to protect, and the initial proof command the supervisor
  should delegate for the move.

### Step 2: Extract route3 memory-access bodies

Goal: Move the existing route3 memory-access implementation into a focused
translation unit without semantic edits.

Primary target: `src/backend/bir/`

Actions:

- Add `src/backend/bir/bir_route3_memory.cpp` only if Step 1 confirms it is the
  right focused owner.
- Move the selected route3 implementation bodies intact.
- Add private helper declarations only where needed to preserve linkage and
  avoid public API churn.
- Update build wiring for the new file if one is added.
- Keep public route3 declarations in `bir.hpp`.

Completion check:

- The project builds through the delegated build command.
- No public route3 declaration has moved out of `bir.hpp`.
- The diff is limited to the body move, required private declarations, build
  wiring, and `todo.md` progress.

### Step 3: Prove memory-access and route6 behavior

Goal: Demonstrate that route3 memory-access semantics and route6 consumers
still observe the same facts after the extraction.

Primary target: focused backend/BIR proof selected by the supervisor.

Actions:

- Run the delegated focused memory-access/source proof.
- Run the delegated route6 call-publication consumer proof.
- If narrow proof is insufficient because route6 dependencies are broader than
  expected, report the needed broader backend subset instead of changing route6
  behavior.
- Record commands and outcomes in `test_after.log` and `todo.md` as delegated.

Completion check:

- Build proof is green.
- Focused memory-access/source proof is green.
- Route6 consumer proof is green or a precise blocker is recorded.
- No memory-access construction, route6 fact access, or public declaration
  semantics changed.

### Step 4: Handoff for review and next cleanup idea

Goal: Leave the route3 cleanup slice ready for supervisor acceptance and the
ordered BIR cleanup sequence ready to proceed to route5.

Primary target: lifecycle and proof notes only.

Actions:

- Summarize the body movement boundary, protected public API, and proof results
  in `todo.md`.
- Call out any unresolved route3/route6 coupling that should affect the next
  follow-up.
- Do not activate `ideas/open/526_bir_route5_publication_body_extraction.md`
  inside this runbook.

Completion check:

- The supervisor can review a narrow behavior-preserving route3 extraction.
- The next ordered open idea remains parked until this active plan is complete
  or explicitly switched.
