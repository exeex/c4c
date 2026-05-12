# LIR Call Argument Structured Payload Boundary Runbook

Status: Active
Source Idea: ideas/open/190_lir_call_argument_structured_payload_boundary.md
Supersedes: blocked active gate from ideas/open/188_lir_bir_freeze_closure_gate.md

## Purpose

Move metadata-rich LIR call lowering away from semantic recovery through
rendered `callee_type_suffix` / `args_str` text.

## Goal

Give generated metadata-rich LIR call sites a structured argument payload that
LIR-to-BIR lowering can use as the authority for argument operand, type, and ABI
facts while retaining rendered call text for output and compatibility paths.

## Core Rule

When structured call metadata is present, generated LIR-to-BIR call lowering
must prefer structured argument facts over reparsed rendered call text. Text
parsing may remain only as an explicit raw/no-metadata compatibility path.

## Read First

- `ideas/open/190_lir_call_argument_structured_payload_boundary.md`
- `ideas/closed/184_direct_call_signature_metadata_structured_boundary.md`
- `ideas/closed/189_direct_call_no_prototype_variadic_signature_mismatch.md`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/call_args.hpp`
- `src/codegen/lir/call_args_ops.hpp`
- `src/codegen/lir/hir_to_lir/call/`
- `src/backend/bir/lir_to_bir/calling.cpp`
- LIR-to-BIR call lowering and call parsing helpers located by `rg "args_str|callee_type_suffix|LirCall" src tests`

## Current Targets

- `LirCallOp` call payload shape.
- `LirCallSignature` metadata boundaries.
- `call_args.hpp` helper APIs and parsed/printed call argument structures.
- LIR-to-BIR direct and indirect call lowering.
- Tests that prove structured metadata, not stale rendered text, owns generated
  call argument facts.

## Non-Goals

- Do not rewrite all LIR instruction operands.
- Do not remove printed call syntax or raw hand-authored LIR fixtures.
- Do not redesign target ABI lowering.
- Do not reopen frontend overload resolution unless a concrete missing fact is
  exposed by the structured payload boundary.
- Do not claim progress through parser renames, printer-only tests, or
  expectation downgrades.

## Working Model

- Ideas 184 and 189 established direct-call signature metadata and repaired a
  no-prototype/variadic signature mismatch.
- That metadata is not enough if the actual generated call argument view is
  still reconstructed from rendered call text.
- Idea 190 is a dependency of idea 191 and a blocker for the idea 188 closure
  gate, so this route should create the structured payload boundary before any
  byval signature-text retirement work.
- Raw/no-id or hand-authored LIR may continue using textual compatibility, but
  that fallback must be explicit and must not silently override structured
  metadata-rich generated calls.

## Execution Rules

- Keep packet progress and proof results in `todo.md`.
- Prefer small semantic slices: inventory first, carrier/API next, lowering
  adoption next, tests and cleanup last.
- Preserve existing printer/output spelling unless changing it is required to
  expose a structured-vs-text authority bug.
- Add or adjust tests to prove stale rendered call text cannot override
  structured metadata when that metadata exists.
- Run build proof after code changes and focused LIR-to-BIR call tests after
  the first semantic lowering change. Escalate to a broader subset before
  claiming idea completion.

## Step 1: Inventory Existing Call Argument Authority

Goal: identify every current generated-call path where BIR lowering derives
argument facts from rendered `callee_type_suffix` or `args_str` text.

Concrete actions:
- Inspect `LirCallOp`, `LirCallSignature`, `call_args.hpp`, and call lowering
  helpers.
- Trace generated direct and indirect call construction into LIR-to-BIR
  lowering.
- Distinguish metadata-rich generated paths from raw/no-metadata compatibility
  paths.
- Record exact text-authority sites and any existing structured facts in
  `todo.md`.

Completion check:
- `todo.md` names the generated metadata-rich paths, raw compatibility paths,
  and first implementation target for adding or selecting the structured
  argument carrier.

## Step 2: Define The Structured Argument Carrier

Goal: add or select the narrow structured payload shape needed by BIR call
lowering.

Concrete actions:
- Define the carrier at the existing LIR call boundary instead of creating a
  parallel broad backend abstraction.
- Include argument operand identity, argument type ref or type-text mirror, and
  byval/sret/variadic markers only as needed by current BIR lowering.
- Keep `callee_type_suffix` and `args_str` as final spelling and raw
  compatibility payloads.
- Preserve existing direct-call signature metadata from ideas 184 and 189.

Completion check:
- Metadata-rich call construction can carry the structured argument facts
  needed by lowering without changing target ABI semantics.

## Step 3: Route Generated LIR-To-BIR Calls Through Structured Facts

Goal: make structured argument metadata the semantic authority when available.

Concrete actions:
- Update LIR-to-BIR call lowering to consume the structured carrier for
  metadata-rich calls.
- Keep the text parser reachable only for explicit no-metadata compatibility
  cases.
- Ensure stale rendered `callee_type_suffix` or `args_str` cannot silently
  change selected argument type, byval/sret marker, or variadic fact when
  structured metadata is present.
- Keep direct and indirect call behavior aligned unless the inventory proves a
  justified narrower first slice.

Completion check:
- Generated metadata-rich call lowering no longer reparses rendered call text
  as semantic authority for argument facts.

## Step 4: Prove Structured Metadata Beats Stale Text

Goal: add focused tests that would fail if generated calls still trusted stale
rendered call text.

Concrete actions:
- Add or update focused LIR-to-BIR direct call coverage.
- Add or update indirect call coverage if the structured carrier applies to
  indirect calls in the implementation slice.
- Include a stale rendered text scenario where structured metadata remains
  authoritative.
- Avoid printer-only assertions as the primary proof.

Completion check:
- Focused call tests prove structured argument metadata controls generated call
  lowering and raw/no-metadata compatibility remains explicit.

## Step 5: Validate And Prepare Completion Evidence

Goal: establish enough proof for supervisor acceptance and for idea 188 to
eventually review idea 190 as a closed dependency.

Concrete actions:
- Run the supervisor-delegated build and focused LIR-to-BIR call test subset.
- Escalate to the broader validation subset requested by the supervisor before
  claiming the idea is complete.
- Record retained compatibility boundaries and proof commands in `todo.md`.
- Do not close idea 190 until the source acceptance criteria are met.

Completion check:
- `todo.md` contains implementation summary, compatibility boundary notes, and
  green proof for focused direct/indirect call coverage or a specific blocker
  that must be handled before closure.
