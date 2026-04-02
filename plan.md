# Header Entry Surface And Function Naming Refactor Runbook

Status: Active
Source Idea: ideas/open/header_entry_surface_and_function_naming_refactor.md

## Purpose

Turn parser and HIR entry headers into reliable navigation surfaces and make
helper naming reflect actual parser/lowering behavior instead of historical
accidents.

## Goal

Apply behavior-preserving declaration regrouping and targeted helper renames
across the parser and HIR lowering surfaces so a new agent can identify the
right entry points and infer helper contracts from names alone.

## Core Rule

Keep this refactor behavior-preserving. Do not use naming cleanup as cover for
parser architecture changes, HIR redesign, or unrelated semantic fixes.

## Read First

- [ideas/open/header_entry_surface_and_function_naming_refactor.md](/workspaces/c4c/ideas/open/header_entry_surface_and_function_naming_refactor.md)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/hir/hir.hpp](/workspaces/c4c/src/frontend/hir/hir.hpp)
- [src/frontend/hir/hir_lowering.hpp](/workspaces/c4c/src/frontend/hir/hir_lowering.hpp)
- [src/frontend/hir/hir_lowerer_internal.hpp](/workspaces/c4c/src/frontend/hir/hir_lowerer_internal.hpp)

## Current Targets

- Parser entry/index surface in [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- Parser implementation families in [src/frontend/parser](/workspaces/c4c/src/frontend/parser)
- HIR entry/index surface in [src/frontend/hir/hir.hpp](/workspaces/c4c/src/frontend/hir/hir.hpp)
- HIR lowering surfaces in [src/frontend/hir/hir_lowering.hpp](/workspaces/c4c/src/frontend/hir/hir_lowering.hpp) and [src/frontend/hir/hir_lowerer_internal.hpp](/workspaces/c4c/src/frontend/hir/hir_lowerer_internal.hpp)
- Cross-translation-unit HIR lowering helpers in [src/frontend/hir](/workspaces/c4c/src/frontend/hir)

## Non-Goals

- Do not redesign parser or HIR architecture.
- Do not collapse internal headers into one shared surface.
- Do not rename stable external APIs unless the clarity gain is direct and
  material.
- Do not mix parser-debug or STL bring-up work into this plan.

## Working Model

- Entry headers should declare their role near the top and act as indexes to
  the main implementation files.
- Declaration order should follow reader tasks, not historical insertion order.
- Parser helper verbs must distinguish probing, consuming, skipping, expecting,
  and semantic parse operations.
- HIR helper verbs must distinguish build orchestration, lowering, resolution,
  materialization, collection, and recording.
- Rename only when the current name misstates the helper contract or breaks
  family consistency.

## Execution Rules

- Inspect existing helper families before renaming them.
- Keep rename batches scoped by subsystem and verb family.
- Update declarations, definitions, and call sites in the same slice.
- Prefer small comment additions that explain header role or implementation map;
  avoid narrative history comments.
- If execution uncovers a separate initiative, record it under `ideas/open/`
  instead of broadening this runbook.

## Ordered Steps

### Step 1: Audit parser and HIR naming/grouping surfaces

Goal:
Build the rename/grouping inventory before touching declarations.

Primary targets:
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/hir/hir.hpp](/workspaces/c4c/src/frontend/hir/hir.hpp)
- [src/frontend/hir/hir_lowering.hpp](/workspaces/c4c/src/frontend/hir/hir_lowering.hpp)
- [src/frontend/hir/hir_lowerer_internal.hpp](/workspaces/c4c/src/frontend/hir/hir_lowerer_internal.hpp)

Concrete actions:
- identify header sections that are mixed by responsibility
- list parser helpers whose names misdescribe probe/parse/consume/skip behavior
- list HIR helpers whose names misdescribe build/lower/resolve/materialize/collect behavior
- define the target grouping for each high-priority header

Completion check:
- there is a concrete rename table and grouping plan for parser and HIR surfaces

### Step 2: Normalize entry/header structure

Goal:
Make the high-priority headers readable top-down before deep renames land.

Primary targets:
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/hir/hir.hpp](/workspaces/c4c/src/frontend/hir/hir.hpp)
- [src/frontend/hir/hir_lowering.hpp](/workspaces/c4c/src/frontend/hir/hir_lowering.hpp)
- [src/frontend/hir/hir_lowerer_internal.hpp](/workspaces/c4c/src/frontend/hir/hir_lowerer_internal.hpp)

Concrete actions:
- add short file-role comments where the header currently lacks one
- add compact implementation maps that point to the main owning `.cpp` files
- regroup declarations by reader task and responsibility
- move the most important public entry points toward the top

Completion check:
- each target header reads as an index with explicit sections and a clear file role

### Step 3: Rename parser helpers by behavior category

Goal:
Align parser helper names with actual parse behavior and commitment semantics.

Primary targets:
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/parser](/workspaces/c4c/src/frontend/parser)

Concrete actions:
- rename pure probes to `peek_*`, `is_*`, `has_*`, or `can_*` as appropriate
- rename speculative parsers to `try_parse_*`
- keep committed syntax builders under `parse_*`
- rename token-advancing helpers to `consume_*`, `skip_*`, or `expect_*`
- normalize construction helpers under `make_*`, `build_*`, or `finalize_*`

Completion check:
- renamed parser helper families read consistently in declarations, definitions, and call sites

### Step 4: Rename HIR lowering helpers by semantic role

Goal:
Make HIR lowering/helper families describe pipeline behavior consistently.

Primary targets:
- [src/frontend/hir/hir_lowering.hpp](/workspaces/c4c/src/frontend/hir/hir_lowering.hpp)
- [src/frontend/hir/hir_lowerer_internal.hpp](/workspaces/c4c/src/frontend/hir/hir_lowerer_internal.hpp)
- [src/frontend/hir](/workspaces/c4c/src/frontend/hir)

Concrete actions:
- reserve `build_*` for pipeline/module construction
- reserve `lower_*` for AST or semantic lowering into HIR
- reserve `resolve_*` for lookup/substitution/fixup
- reserve `materialize_*` for deferred-to-concrete HIR conversion
- reserve `collect_*` for scan-only discovery and `record_*` for persisted facts

Completion check:
- cross-TU HIR helper families use consistent verbs and no major misnamed helpers remain

### Step 5: Validate navigability and build health

Goal:
Prove the refactor preserved behavior and improved header readability.

Concrete actions:
- build after the rename batches land
- run targeted tests affected by parser/HIR compile paths
- run the broader regression suite required by the execution prompt
- do a top-down read of the entry headers for navigability

Completion check:
- build and tests remain green or monotonic, and the entry headers work as module indexes
