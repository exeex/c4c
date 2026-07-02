# BIR Render Owner Preservation Runbook

Status: Active
Source Idea: ideas/open/520_bir_render_owner_preservation.md
Activated From: user request to activate 520

## Purpose

Start the first BIR cleanup follow-up from idea 518 by preserving existing
printer and validator ownership, then deciding whether public render helper
bodies can move out of central `bir.cpp` without semantic or dependency churn.

## Goal

Audit and, if safe, isolate the public BIR render helper bodies while keeping
route analysis, validation, printer output, public declarations, and all
behavior unchanged.

## Core Rule

This is a behavior-preserving ownership cleanup. Do not change BIR semantics,
printed spelling, validation diagnostics, route records, public function
signatures, or gcc_torture expectations.

## Read First

- ideas/open/520_bir_render_owner_preservation.md
- docs/bir_core_cleanup/follow_up_ideas.md
- docs/bir_core_cleanup/destination_map.md
- docs/bir_core_cleanup/structure_snapshot.md
- docs/bir_core_cleanup/declaration_inventory.md
- docs/bir_core_cleanup/implementation_inventory.md
- .codex/skills/c4c-clang-tools/SKILL.md
- src/backend/bir/bir.cpp
- src/backend/bir/bir.hpp
- src/backend/bir/bir_printer.cpp
- src/backend/bir/bir_validate.cpp

## Current Targets

- Public render helper bodies currently in `src/backend/bir/bir.cpp`, especially:
  - `render_type`
  - `render_binary_opcode`
  - `render_cast_opcode`
- Candidate destination:
  - `src/backend/bir/bir_printer.cpp` only if non-printer dependency impact is
    acceptable
  - a new narrow render translation unit only if printer ownership would create
    undesirable linkage or dependency coupling
  - central `bir.cpp` if moving is not clearly safe

## Non-Goals

- Do not move route analysis declarations or records.
- Do not change `print(Module, ...)` or `validate(Module, ...)` behavior.
- Do not fold route-index validation helpers into module validation.
- Do not start route body extraction.
- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`, `MemoryAddress`,
  route records, or route queries.
- Do not change tests, expectations, unsupported markers, allowlists, or
  pass/fail accounting.

## Working Model

Idea 518 found that `bir_printer.cpp` and `bir_validate.cpp` already have clear
owners for private printer and validator helpers. The remaining first-slice
question is whether public render helper bodies in `bir.cpp` can be moved
without making non-printer users depend on printer-only implementation details.

Early BIR cleanup should prove dependency direction before moving bodies. If
the audit shows the central location is currently the least-coupled owner, this
plan may complete by recording the preserve decision instead of forcing a move.

## Execution Rules

- Use `.codex/skills/c4c-clang-tools/` before raw long-file reading.
- If `c4c-clang-tool` or `c4c-clang-tool-ccdb` is missing from `PATH`, run
  `scripts/build_install_c4c_clang_tools.sh` and retry from `PATH`.
- Start with AST-backed `list-symbols`, `function-signatures`,
  `function-callers`, `function-callees`, and type-reference queries for the
  target helpers and candidate destination files.
- Keep edits scoped to BIR render ownership and build metadata required by a
  new translation unit, if one is justified.
- Prefer no implementation movement over an unclear or broad coupling move.
- For any code change, run build proof and focused backend/BIR proof selected
  by the supervisor.

## Step 1: Audit Render Helper Consumers

Goal: Determine who calls the public render helpers and what dependencies a
move would introduce.

Actions:

- Confirm clang-tools availability.
- Run AST-backed symbol and function-signature queries on `bir.cpp`,
  `bir.hpp`, and `bir_printer.cpp`.
- Query direct callers/callees for `render_type`, `render_binary_opcode`, and
  `render_cast_opcode`.
- Inspect only the minimal raw source needed to confirm ambiguous query
  results.
- Record whether callers are printer-only, mixed public model users, or other
  backend users.

Completion check:

- `todo.md` records the clang-tools commands, consumer map, and whether a move
  is safe, unsafe, or needs a narrow render TU.

## Step 2: Choose Preserve, Printer Move, Or Narrow Render TU

Goal: Select the smallest behavior-preserving ownership decision.

Actions:

- Compare consumer map against the 518 destination map.
- If all meaningful users are printer-adjacent and no new dependency coupling
  appears, choose `bir_printer.cpp`.
- If helpers are public and used outside printer code, decide whether a narrow
  render TU is clearer than `bir_printer.cpp`.
- If neither destination reduces coupling, preserve bodies in `bir.cpp` and
  record the reason.
- Do not edit implementation code until the destination decision is explicit.

Completion check:

- `todo.md` names the selected disposition and its dependency rationale.

## Step 3: Apply Minimal Ownership Change Or Preserve Decision

Goal: Implement the selected behavior-preserving outcome.

Actions:

- If moving bodies, relocate only the selected render helper definitions and
  any strictly required includes/build metadata.
- Preserve declarations in `bir.hpp`.
- Keep printed spelling and validation diagnostics unchanged.
- If preserving bodies, make no source changes unless a tiny comment is truly
  needed to prevent future route drift.

Completion check:

- Code diff is limited to the selected render ownership outcome.
- No route extraction, route declaration movement, validation rewrite, or test
  expectation change appears in the diff.

## Step 4: Validate Render Ownership

Goal: Prove behavior and linkage are unchanged.

Actions:

- Run `git diff --check`.
- Run a fresh build.
- Run focused BIR printer/validator proof if available.
- If public render helper linkage changes, run at least one backend or object
  emission subset that links non-printer users.

Completion check:

- Validation commands and results are recorded in `todo.md`.
- Any skipped focused proof is explained with the nearest broader substitute.

## Step 5: Handoff Or Close Readiness

Goal: Decide whether idea 520 can close or needs a follow-up adjustment.

Actions:

- Summarize the final disposition: moved to printer, moved to narrow render TU,
  or intentionally preserved in `bir.cpp`.
- Record residual risks and whether the source idea acceptance criteria are
  satisfied.
- Confirm no new cleanup initiative was discovered that belongs in
  `ideas/open/`.

Completion check:

- `todo.md` contains close-readiness notes.
- The source idea can be closed by lifecycle review after validation is accepted.
