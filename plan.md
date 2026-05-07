# Rendered Qualified Compatibility Bridge Removal Runbook

Status: Active
Source Idea: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md

## Purpose

Remove the remaining rendered qualified-name compatibility bridges after idea
146 moved qualified-name authority to structured parser, Sema, and HIR
carriers.

Goal: qualified template and HIR semantic lookup paths must carry structured
qualified-name metadata instead of reconstructing lookup authority by splitting
one rendered `A::B::C` spelling.

## Core Rule

Do not claim bridge-removal progress by renaming, hiding, or rewrapping the same
rendered-spelling split. Semantic lookup authority must come from structured
carriers such as `QualifiedNameKey`, owner/name/domain metadata, or deferred
carrier payloads.

## Read First

- Source idea: `ideas/open/147_rendered_qualified_compatibility_bridge_removal.md`
- Parent context: `ideas/closed/146_qualified_name_deferred_carrier_authority.md`

## Current Scope

- `find_compatibility_key_from_rendered_qualified_spelling()`
- `intern_compatibility_key_from_rendered_qualified_spelling()`
- parser calls to `qualified_key_in_context()` that still accept one rendered
  `TextId` containing `::`
- shared rendered-spelling helpers such as `split_qualified_name_scope()` when
  reachable from semantic lookup
- Sema/HIR fallback comments and routes that still describe no-structured-
  metadata compatibility bridges
- qualified template and HIR tests that prove these paths no longer depend on
  rendered qualified spelling splits

## Non-Goals

- Do not reopen the broad qualified-name authority migration completed by idea
  146.
- Do not change C++ lookup semantics beyond removing rendered-spelling
  compatibility bridges.
- Do not force all dependent lookup to finish before HIR.
- Do not rewrite diagnostics, dumps, display spelling, ABI names, or emitted
  symbol spelling unless they are incorrectly used as semantic lookup
  authority.
- Do not weaken tests, mark supported cases unsupported, or downgrade qualified
  template/HIR expectations.

## Working Model

1. Inventory every semantic caller that can still reach rendered qualified-name
   bridge helpers.
2. Migrate callers that still hand off one rendered `TextId` into structured
   qualified-name carriers.
3. Delete or isolate rendered-spelling helpers so any surviving use is
   display-only and explicitly labeled.
4. Add focused tests that would fail if qualified template or HIR lookup depends
   on splitting rendered `A::B::C` spelling.

## Execution Rules

- Use `rg` first to locate bridge symbols, rendered `TextId` qualified handoffs,
  and fallback comments.
- Keep each implementation packet tied to one caller family or bridge boundary.
- Prefer semantic carrier migration over expectation edits.
- If a packet only rewrites comments, display spelling, or test expectations,
  do not count it as capability progress.
- If a narrow qualified template fixture passes, inspect nearby same-feature
  qualified template and HIR paths before declaring the bridge path migrated.
- For code-changing packets, get fresh build or compile proof plus the narrow
  test subset chosen by the supervisor.
- Escalate to broader parser/Sema/HIR validation after bridge deletion or after
  multiple narrow carrier-migration packets.

## Steps

### Step 1: Inventory Reachable Rendered-Spelling Bridges

Goal: identify every current semantic route that can still depend on rendered
qualified-name splitting.

Primary targets:
- bridge helper definitions and direct callers
- parser `qualified_key_in_context()` calls with rendered qualified `TextId`
- `split_qualified_name_scope()` semantic callers
- Sema/HIR fallback comments that describe missing structured metadata

Actions:
- Locate all occurrences of the target helpers and comments.
- Classify each occurrence as semantic lookup authority, structured-carrier
  migration work, display-only spelling, or dead/unreachable code.
- Record in `todo.md` which caller family should be delegated first and why.

Completion check:
- `todo.md` lists the reachable semantic bridge callers, the display-only or
  dead exclusions, and the first implementation packet candidate.

### Step 2: Replace Parser Rendered Qualified `TextId` Handoffs

Goal: stop parser semantic paths from passing one rendered `TextId` containing
`::` as lookup authority.

Primary targets:
- parser calls to `qualified_key_in_context()`
- callers that can construct or preserve a structured `QualifiedNameKey`
- deferred carrier payloads that can retain owner/name/domain metadata

Actions:
- For each parser caller family from Step 1, preserve or thread structured
  qualified metadata instead of rendered spelling.
- Keep display spelling separate from semantic lookup keys.
- Add or update focused tests for the migrated parser caller family.

Completion check:
- The migrated parser caller family no longer needs rendered qualified spelling
  reconstruction, and its narrow proof is green.

### Step 3: Migrate Qualified Template and HIR Compatibility Paths

Goal: make qualified template and HIR compatibility routes consume structured
metadata instead of falling back to rendered qualified spelling.

Primary targets:
- qualified template carrier construction and lookup handoff points
- HIR carrier or lookup paths identified by Step 1
- no-structured-metadata fallback routes and comments

Actions:
- Replace rendered qualified `TextId` handoff with structured carrier payloads
  for one qualified template or HIR caller family at a time.
- Remove fallback comments only when the underlying structured route is present.
- Add tests that fail if the path regresses to splitting `A::B::C`.

Completion check:
- The migrated qualified template/HIR family carries structured metadata end to
  end, and its focused tests are green without expectation weakening.

### Step 4: Delete or Isolate Compatibility Bridge Helpers

Goal: remove semantic reachability of rendered qualified-name compatibility
bridges.

Primary targets:
- `find_compatibility_key_from_rendered_qualified_spelling()`
- `intern_compatibility_key_from_rendered_qualified_spelling()`
- `split_qualified_name_scope()` and related shared helpers

Actions:
- Delete bridge helpers when all semantic callers are gone.
- If a rendered-spelling helper must survive for display-only behavior, move or
  label it so semantic lookup cannot call it.
- Remove obsolete fallback comments that still describe compatibility bridge
  behavior.

Completion check:
- No semantic parser, Sema, or HIR lookup path can call rendered-spelling
  compatibility key reconstruction, and any surviving split helper is
  display-only.

### Step 5: Prove Bridge Removal Across Parser, Sema, and HIR

Goal: validate the source idea acceptance criteria without broad unrelated
rewrites.

Actions:
- Run the supervisor-chosen focused qualified template/HIR tests.
- Run a broader parser/Sema/HIR validation checkpoint after bridge deletion.
- Inspect the diff for reviewer reject signals before requesting acceptance.

Completion check:
- Focused tests cover the formerly regressing qualified template/HIR cases.
- Broader parser/Sema/HIR validation remains green.
- The diff does not retain rendered `A::B::C` splitting behind a renamed
  semantic helper or weaker test contract.
