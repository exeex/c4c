# LIR Agent Index Header Hierarchy Runbook

Status: Active
Source Idea: ideas/open/lir-agent-index-header-hierarchy.md

## Purpose

Make `src/codegen/lir` easier for agents to navigate by using headers as
directory-level index surfaces instead of one-to-one partners for implementation
files.

## Goal

Reshape LIR headers and implementation files so path depth communicates public
versus private ownership without changing LIR semantics.

## Core Rule

Use `directory = semantic boundary` and `.hpp = index entry for that boundary`.
Top-level `src/codegen/lir/*.hpp` files are exported surfaces by default;
nested headers are private to their semantic implementation area by default.

## Read First

- `ideas/open/lir-agent-index-header-hierarchy.md`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir.hpp`
- `src/codegen/lir/stmt_emitter.hpp`
- `src/codegen/lir/call_args.hpp`
- `src/codegen/lir/call_args_ops.hpp`
- `src/codegen/lir/const_init_emitter.hpp`
- `src/codegen/lir/lir_printer.hpp`
- `src/codegen/lir/verify.hpp`

## Current Scope

- `src/codegen/lir/`
- public LIR model and entry headers
- private HIR-to-LIR lowering declarations
- statement-emitter and constant-initializer implementation placement
- call-argument surfaces only to decide whether they remain exported or need a
  private split

## Non-Goals

- Do not introduce a separate `include/` tree.
- Do not create one header per `.cpp`.
- Do not change HIR-to-LIR lowering semantics, printer behavior, verifier
  behavior, or testcase expectations.
- Do not turn semantic cleanup opportunities into this structural refactor
  unless they are required to keep the structural move compiling.
- Do not hide cross-component APIs behind private lowering directories.

## Working Model

- Keep `src/codegen/lir/ir.hpp` public for the LIR data model.
- Keep `src/codegen/lir/hir_to_lir.hpp` public for HIR-to-LIR entry APIs.
- Decide whether `lir_printer.hpp` and `verify.hpp` remain justified exported
  headers or should merge into a nearby public LIR-level surface.
- Move implementation-only lowering declarations out of top-level headers.
- Add `src/codegen/lir/hir_to_lir/lowering.hpp` as the private index for
  shared HIR-to-LIR lowering declarations.
- Add `src/codegen/lir/hir_to_lir/call/call.hpp` only if call lowering becomes
  its own semantic directory.

## Execution Rules

- Keep each step behavior-preserving.
- Prefer file moves and include rewrites that preserve blame when practical.
- After each structural code step, build `c4c_codegen` or run the supervisor
  delegated equivalent compile proof.
- Escalate to a broader frontend/codegen test subset after moving shared
  headers or implementation files that affect multiple lowering paths.
- Record any semantic issue discovered during the refactor as a separate open
  idea unless it blocks compilation of the structural work.

## Ordered Steps

### Step 1: Decide Exported Top-Level LIR Surfaces

Goal: establish which top-level headers are public interfaces before moving
private implementation declarations.

Primary target:

- `src/codegen/lir/*.hpp`

Actions:

- Inspect include users for `ir.hpp`, `hir_to_lir.hpp`, `lir_printer.hpp`,
  `verify.hpp`, `call_args.hpp`, `call_args_ops.hpp`,
  `const_init_emitter.hpp`, and `stmt_emitter.hpp`.
- Classify each top-level header as exported, private lowering state, or
  candidate for merge into a public LIR-level API.
- Do not change behavior or test expectations while classifying.
- If a thin header is kept top-level, document why it is an exported surface in
  the code structure or in `todo.md` packet notes.

Completion check:

- The executor can name the intended destination or retention reason for each
  current top-level LIR header.
- No implementation files are moved before the exported/private boundary is
  understood.

### Step 2: Create the Private HIR-to-LIR Lowering Index

Goal: move implementation-only lowering declarations behind the private
`hir_to_lir/` boundary.

Primary targets:

- `src/codegen/lir/hir_to_lir/lowering.hpp`
- `src/codegen/lir/stmt_emitter.hpp`
- `src/codegen/lir/const_init_emitter.hpp`
- `src/codegen/lir/hir_to_lir.cpp`
- `src/codegen/lir/stmt_emitter_*.cpp`
- `src/codegen/lir/const_init_emitter.cpp`

Actions:

- Add `src/codegen/lir/hir_to_lir/lowering.hpp` as the private directory index.
- Move `StmtEmitter` and related implementation-only declarations from the
  top-level `stmt_emitter.hpp` into the private index.
- Move constant-initializer lowering declarations into the private index when
  they are implementation-only.
- Rewrite includes so HIR-to-LIR implementation files include the private
  index instead of top-level implementation headers.
- Remove or retire top-level private headers when they no longer represent
  exported surfaces.

Completion check:

- External callers can include top-level LIR headers without pulling in
  statement-emitter implementation state.
- HIR-to-LIR implementation files can include one private index header for
  shared lowering declarations.
- `c4c_codegen` builds.

### Step 3: Move HIR-to-LIR Implementation Files Under the Semantic Directory

Goal: align implementation file locations with the private lowering boundary.

Primary targets:

- `src/codegen/lir/hir_to_lir/`
- current `src/codegen/lir/hir_to_lir.cpp`
- current `src/codegen/lir/stmt_emitter_*.cpp`
- current `src/codegen/lir/const_init_emitter.cpp`

Actions:

- Move HIR-to-LIR implementation files under `src/codegen/lir/hir_to_lir/`.
- Prefer names that communicate the semantic unit, such as `lowering.cpp`,
  `stmt.cpp`, `expr.cpp`, `lvalue.cpp`, `types.cpp`, and `const_init.cpp`,
  when the move materially improves navigation.
- Update build definitions and includes for the new paths.
- Keep moved code behavior-preserving.
- Do not create matching headers for every moved `.cpp`.

Completion check:

- The source tree makes the HIR-to-LIR private implementation boundary visible
  through path depth.
- Build definitions reference the moved files.
- `c4c_codegen` builds and the supervisor-selected narrow codegen subset
  passes.

### Step 4: Reassess Call Argument Surfaces

Goal: keep cross-component call argument APIs exported while moving only
implementation-only call lowering state into a private subdomain if needed.

Primary targets:

- `src/codegen/lir/call_args.hpp`
- `src/codegen/lir/call_args_ops.hpp`
- call-related `stmt_emitter` implementation files
- optional `src/codegen/lir/hir_to_lir/call/call.hpp`

Actions:

- Inspect users of `call_args.hpp` and `call_args_ops.hpp`, including printer
  and BIR lowering users.
- Keep surfaces top-level when they are genuinely cross-component APIs.
- Split only implementation-only call-lowering declarations behind
  `hir_to_lir/call/` if that reduces navigation cost without hiding exported
  APIs.
- Create `hir_to_lir/call/call.hpp` only when call lowering becomes a coherent
  private subdomain.

Completion check:

- Cross-component call argument APIs remain accessible at top level when
  required.
- Private call lowering state, if any, lives under the HIR-to-LIR private
  hierarchy.
- No testcase-shaped shortcuts or semantic changes are introduced.

### Step 5: Final Structural Validation and Cleanup

Goal: prove the LIR hierarchy refactor is complete and behavior-preserving.

Primary targets:

- `src/codegen/lir/`
- build definitions
- relevant frontend/codegen tests

Actions:

- Remove obsolete top-level private headers and stale include paths.
- Confirm no one-header-per-`.cpp` pattern was introduced.
- Confirm top-level headers are exported surfaces or intentionally retained
  public index headers.
- Run `c4c_codegen` build proof.
- Run the supervisor-selected relevant frontend/codegen test subset; escalate
  to broader validation if multiple structural packets have landed.

Completion check:

- Acceptance criteria from the source idea are satisfied.
- `src/codegen/lir/hir_to_lir/lowering.hpp` is the private lowering index.
- External callers do not need private lowering headers for public LIR use.
- Relevant validation is green.
