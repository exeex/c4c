# BIR Global Memory Provenance LinkNameId Expansion Runbook

Status: Active
Source Idea: ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md
Activated After: ideas/closed/191_bir_function_signature_byval_metadata_text_retirement.md
Unblocks: ideas/open/188_lir_bir_freeze_closure_gate.md

## Purpose

Expand the global memory/provenance `LinkNameId` cleanup beyond the
addressed-global pointer route closed by idea 187.

## Goal

Make one additional metadata-rich global memory/provenance route use
`LinkNameId` as semantic global identity when available, while keeping raw
global spelling only as a route-local handle or no-id compatibility bridge.

## Core Rule

When a global reference carries `LinkNameId` metadata, matching final spelling
must not override stale, missing, or mismatched global identity metadata on the
selected route.

## Read First

- `ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md`
- `ideas/closed/187_bir_memory_provenance_global_handle_cleanup.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir/global_initializers.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/backend_prepare_structured_context_test.cpp`
- `tests/backend/backend_prepare_stack_layout_test.cpp`

## Current Targets

- Remaining `global_types.find(global_name)` consumers and other global-name
  keyed memory/provenance paths.
- `GlobalAddress` metadata and uses that feed global load/store provenance,
  pointer initializer address resolution, dynamic global array materialization,
  or prepared stack layout.
- String-keyed maps under `src/backend/bir/lir_to_bir/memory/` that may still
  act as semantic global identity instead of route-local handles.
- Focused backend tests for matching, stale, and missing global id metadata on
  the selected route.

## Non-Goals

- Do not remove every string-keyed memory map in this runbook.
- Do not rewrite local slot, local SSA, temporary pointer, or block-label
  handling.
- Do not change final BIR dump spelling.
- Do not redesign alias analysis or broad memory lowering.
- Do not claim progress by adding tests only for the already-hardened idea 187
  addressed-global pointer route.

## Working Model

- Idea 187 converted one selected addressed-global pointer provenance path so
  global identity is `LinkNameId`-backed where metadata exists.
- The remaining audit surface includes global load/store provenance, pointer
  initializer address resolution, dynamic global scalar/array materialization,
  and prepared stack layout handoff paths.
- Some string maps are valid local route handles. The implementation must
  distinguish those handles from semantic global identity instead of treating
  every spelling map as wrong.
- This idea is complete when one additional route is hardened with matching
  positive coverage plus stale or missing metadata rejection or explicit no-id
  compatibility coverage.

## Execution Rules

- Keep packet progress and proof results in `todo.md`.
- Inventory before editing; select one route with clear semantic global
  identity and test coverage potential.
- Preserve raw spelling compatibility only when `LinkNameId` metadata is absent
  or the route is explicitly local.
- Prefer fail-closed checks for stale metadata over fallback to matching final
  spelling.
- After code changes, run build proof plus focused backend BIR
  memory/provenance or LIR-to-BIR tests. Escalate to broader validation before
  claiming the idea complete.

## Step 1: Inventory Remaining Global Provenance Routes

Goal: identify remaining spelling-first global memory/provenance consumers and
select one additional route for `LinkNameId` hardening.

Concrete actions:
- Inspect `GlobalAddress`, BIR global load/store `global_name_id`, pointer
  initializer offsets, dynamic global array/scalar materialization, and
  prepared stack layout global handoff.
- Classify each string-keyed map as route-local, raw/no-id compatibility, or
  semantic global identity.
- Exclude the idea 187 addressed-global pointer route unless it is only needed
  as comparison context.
- Select one additional route whose metadata-rich path can fail closed on
  mismatched or missing `LinkNameId`.
- Record the chosen route, first implementation target, fallback boundary, and
  focused test location in `todo.md`.

Completion check:
- `todo.md` names the selected non-187 route, the spelling-first authority
  site, the expected structured `LinkNameId` source, and the first code slice.

## Step 2: Add Or Tighten LinkNameId Authority For The Selected Route

Goal: make the selected metadata-rich global route carry or verify
`LinkNameId` identity before raw spelling can influence semantics.

Concrete actions:
- Add or reuse a structured `LinkNameId` carrier for the selected route.
- Check present ids against known module globals or the relevant structured
  global table.
- Keep raw spelling maps as route-local handles or no-id compatibility only.
- Make stale, mismatched, or missing metadata-rich global ids fail closed
  instead of falling back to final spelling.

Completion check:
- The selected route no longer uses global spelling as semantic identity when
  `LinkNameId` metadata exists.

## Step 3: Route Downstream Memory Or Prepared Handoff Through Structured Facts

Goal: ensure downstream consumers of the selected route preserve the structured
global identity and do not reintroduce spelling-first semantics.

Concrete actions:
- Update downstream lowering, memory map, or prepared layout handoff code for
  the selected route.
- Preserve final spelling for output and local lookup handles.
- Add comments only where they clarify why a string key is route-local rather
  than semantic identity.
- Avoid unrelated memory lowering rewrites.

Completion check:
- Downstream BIR or prepared data carries the selected route's `LinkNameId`
  identity consistently, with raw fallback visibly separated.

## Step 4: Prove Matching, Stale, And Missing Global Id Behavior

Goal: add focused tests proving the selected route is `LinkNameId`-authoritative
when metadata is available.

Concrete actions:
- Add or update backend BIR memory/provenance or LIR-to-BIR tests for the
  selected route.
- Include a positive structured success case.
- Include a stale or mismatched id case that fails closed even if spelling
  would otherwise match.
- Include a missing-id or no-id compatibility case when that fallback remains
  supported.
- Ensure tests do not rely only on the already-hardened idea 187 path.

Completion check:
- Focused tests fail on spelling-first behavior and pass only when the selected
  route uses `LinkNameId` authority correctly.

## Step 5: Validate And Prepare Completion Evidence

Goal: establish enough proof for supervisor acceptance and for idea 188 to
review idea 194 as a closed freeze-gate dependency.

Concrete actions:
- Run the supervisor-delegated build and focused backend test subset.
- Escalate to the broader validation subset requested by the supervisor before
  claiming source-idea completion.
- Record implementation summary, selected route, compatibility boundary, and
  proof commands in `todo.md`.
- Do not close idea 194 until the source acceptance criteria are met.

Completion check:
- `todo.md` contains the final implementation summary, retained compatibility
  boundary, and green proof, or a precise blocker for lifecycle review.
