# BIR String Legacy Path Cleanup Runbook

Status: Active
Source Idea: ideas/open/122_bir_string_legacy_path_cleanup.md
Activated From: ideas/open/122_bir_string_legacy_path_cleanup.md

## Purpose

Remove or demote legacy string authority from BIR so semantic identity is
carried by structured ids, typed records, or explicit resolver tables instead
of raw spelling.

## Goal

Clean the BIR and LIR-to-BIR semantic identity boundary without depending on
current MIR or target assembly behavior as the primary acceptance surface.

## Core Rule

Do not prove this cleanup through MIR/x86 renderer behavior, expectation
downgrades, or testcase-shaped rewrites. Use BIR, BIR verifier, BIR printer,
focused dump, and LIR-to-BIR tests as the primary proof surface.

## Read First

- `ideas/open/122_bir_string_legacy_path_cleanup.md`
- BIR data model, builders, verifier, printer, and dump paths.
- LIR-to-BIR lowering and resolver boundaries.
- Existing BIR, BIR verifier, BIR printer, focused dump, and LIR-to-BIR tests.
- Idea 121 closure only as prerequisite context; do not reopen renderer
  recovery here.

## Current Targets

- String-valued BIR fields, helper APIs, builders, printers, verifiers, and
  tests.
- LIR-to-BIR lowering surfaces that carry or reconstruct identity.
- BIR label, symbol, type, layout, function, and value identity paths.
- Compatibility payloads, selector/user-input fields, unresolved-id
  boundaries, and display spellings that intentionally remain string-backed.

## Non-Goals

- Do not rewrite MIR or target x86 assembly generation in this idea.
- Do not use current MIR/ASM output as the primary proof that BIR string
  authority was removed.
- Do not start parser or HIR cleanup before the BIR and LIR-to-BIR authority
  boundary is clear.
- Do not delete strings retained only for display spelling, selector input,
  stable dumps, compatibility fixtures, or intentionally unresolved ids.
- Do not mark supported tests unsupported or weaken expectations to make
  string cleanup pass.
- Do not absorb new x86/MIR renderer or handoff gaps into this plan; record a
  blocker and create a follow-up idea if that work is required.

## Working Model

- BIR semantic identity should be represented by structured ids, typed records,
  or explicit resolver tables whenever that data already exists upstream.
- Retained strings must be classified as display spelling, selector/user
  input, compatibility payload, unresolved-id boundary, or another explicit
  non-authority boundary.
- BIR dump spelling should remain stable unless the supervisor approves a
  separate contract change.
- LIR-to-BIR lowering should carry ordinary structured identity forward rather
  than forcing BIR to rediscover semantics from spelling.

## Execution Rules

- Start with inventory and classification before changing representation or
  tests.
- Prefer existing BIR ids, typed records, and resolver APIs over new broad
  abstractions.
- Keep each code-changing step behavior-preserving except for the intended
  removal of raw-string semantic authority.
- Pair implementation packets with fresh build or focused BIR/LIR-to-BIR test
  proof chosen by the supervisor.
- Add negative coverage when a string authority path is demoted so drifted or
  missing structured identity fails at the semantic boundary.
- Keep compatibility and display strings explicit; do not erase stable dump
  spelling just to reduce string counts.
- Escalate to broader backend validation before closure because BIR identity is
  a shared compiler boundary.

## Ordered Steps

### Step 1: Inventory BIR String Authority

Goal: produce a complete map of BIR string uses and classify which ones are
semantic authority versus retained non-authority strings.

Primary targets:
- BIR model and record definitions.
- BIR builders, helpers, verifier, printer, and focused dump paths.
- LIR-to-BIR lowering and resolver boundaries.
- BIR and LIR-to-BIR tests that mention string-backed identity.

Concrete actions:
- Inspect string-valued BIR fields and helper APIs.
- Classify each string use as display spelling, selector/user input,
  compatibility payload, unresolved-id boundary, semantic authority, or another
  explicit retained category.
- Identify ordinary label, symbol, type, layout, function, and value identity
  cases where structured ids or typed records already exist upstream.
- Record the first safe implementation packet and proof command in `todo.md`
  when execution begins.
- Do not change implementation files in this lifecycle step unless the
  supervisor delegates a specific inventory-driven repair.

Completion check:
- `todo.md` names the first implementation packet, the authority path it owns,
  retained string categories, and the focused proof command to run.

### Step 2: Demote Local BIR Semantic String Authority

Goal: remove raw-string semantic comparisons inside ordinary BIR helpers,
builders, verifier paths, and printer-adjacent logic where structured identity
is already available.

Primary targets:
- BIR helper and builder APIs that accept or compare names as identity.
- BIR verifier checks that can consume structured ids or typed records.
- Printer and dump logic that must preserve spelling without owning semantic
  identity.

Concrete actions:
- Replace semantic raw-string comparisons with structured ids or typed records
  for one bounded authority family at a time.
- Preserve display spelling as a payload when dumps or diagnostics need it.
- Add or update verifier coverage so missing, drifted, or mismatched
  structured identity is rejected without falling back to raw spelling.
- Keep compatibility and unresolved-id boundaries named explicitly when a
  string must remain.

Completion check:
- Focused BIR tests prove the structured path and retained non-authority string
  category for each modified authority family.

### Step 3: Carry Structured Identity Through LIR-To-BIR

Goal: make LIR-to-BIR lowering carry ordinary semantic identity into BIR
instead of requiring BIR to reconstruct that identity from spelling.

Primary targets:
- LIR-to-BIR lowering.
- Resolver tables or typed records already available before BIR construction.
- Tests that exercise lowered labels, symbols, functions, values, types, or
  layouts.

Concrete actions:
- Thread existing structured identity through the lowering boundary for the
  next owned authority family.
- Keep user-facing names and dump spelling separate from identity records.
- Add boundary tests that show both successful structured lowering and failure
  on missing or drifted identity.
- Do not pull parser or HIR cleanup into this step; record upstream gaps
  separately if BIR cannot receive the needed structure yet.

Completion check:
- LIR-to-BIR tests prove BIR receives structured identity for the modified
  family and no longer relies on raw spelling for ordinary semantics.

### Step 4: Document Retained String Boundaries

Goal: make every retained BIR string field explicit and defensible as
non-authority or intentionally unresolved.

Primary targets:
- BIR model comments or nearby documentation.
- Verifier, printer, and dump tests that expose retained string behavior.
- Compatibility fixtures that still carry raw strings by design.

Concrete actions:
- Document retained string fields with their classification.
- Add or tighten tests for display spelling, selector/user input,
  compatibility payloads, and unresolved-id boundaries.
- Confirm retained strings are not used as fallback semantic authority in the
  modified BIR paths.
- Record any unresolved upstream or compatibility blocker in `todo.md` before
  proposing source-idea changes.

Completion check:
- The retained string map is discoverable in code or tests, and focused proof
  distinguishes retained spelling from semantic identity.

### Step 5: Broader Reprove And Lifecycle Decision

Goal: prove the BIR string-authority cleanup at the appropriate breadth and
decide whether the source idea can close or needs another runbook revision.

Primary targets:
- Focused BIR, BIR printer, BIR verifier, focused dump, and LIR-to-BIR tests.
- Broader backend or compiler validation selected by the supervisor.
- Any follow-up idea needed for x86/MIR renderer or handoff gaps discovered
  during this cleanup.

Concrete actions:
- Re-run the focused proof matrix that covers structured identity and retained
  string boundaries.
- Run broader validation appropriate for a shared BIR/LIR-to-BIR boundary.
- Review unsupported or compatibility cases to ensure no supported path was
  weakened.
- If an x86/MIR renderer gap blocks acceptance, record it as a separate
  follow-up idea instead of expanding this plan.
- Ask the supervisor for completion review before lifecycle closure.

Completion check:
- Focused and broader proof are recorded in `todo.md`, overfit risk is
  reviewed, retained boundaries are documented, and the plan owner can decide
  whether the source idea is complete.
