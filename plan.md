# HIR Module Symbol Structured Lookup Mirror Runbook

Status: Active
Source Idea: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Activated from: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md

## Purpose

Start the HIR structured-identity migration with a narrow, behavior-preserving
module function/global lookup mirror.

Goal: add HIR-owned structured lookup beside existing rendered-name maps,
prove parity, and preserve rendered names for codegen, diagnostics,
mangled/template names, and link-name output.

## Core Rule

Structured keys are for HIR semantic declaration lookup only. Do not remove,
reinterpret, or weaken the rendered-name and link-name surfaces that existing
emission, diagnostics, ABI/link behavior, and template spelling depend on.

## Read First

- `ideas/open/99_hir_module_symbol_structured_lookup_mirror.md`
- HIR module/function/global definitions and lookup helpers.
- AST-to-HIR lowering paths that create functions, bodyless or consteval
  callables, globals, and template/global registrations.
- Existing focused HIR tests named by the validation strategy below.

## Current Scope

- Module-level function lookup.
- Module-level global lookup.
- Declaration-side `TextId` metadata for functions and globals.
- Dual-write structured mirrors beside existing rendered-name maps.
- Structured `DeclRef` lookup when source identity metadata is available.
- Parity checking against legacy rendered lookup during the proof window.

## Non-Goals

- `Module::struct_defs`, `struct_def_order`, or `TypeSpec::tag` migration.
- Struct layout, base tags, codegen type declarations, or LLVM type names.
- Member, static-member, or method owner identity.
- Function-local symbol maps, labels, params, locals, or local const bindings.
- Compile-time engine template or consteval definition registries.
- Enum, const-int, or consteval runtime environment maps.
- Parser or sema rewrites.
- Demoting or deleting legacy rendered-name fallback before targeted proof
  distinguishes structured hits, legacy hits, concrete-ID hits, and
  `LinkNameId` hits, and supervisor approves the regression state.

## Working Model

- Concrete declaration IDs win first when present.
- `LinkNameId` remains a bridge for ABI/link-visible identity.
- Structured source-key lookup runs only when `DeclRef::name_text_id` and
  namespace metadata are available.
- Legacy rendered `fn_index` / `global_index` lookup remains as fallback and
  parity proof during migration.
- Rendered strings, mangled names, diagnostics, and link/codegen output should
  remain byte-for-byte unchanged unless a later plan explicitly authorizes a
  behavior change.

## Execution Rules

- Keep each code-changing step behavior-preserving.
- Prefer small packets that can be built and proven with the focused HIR
  subset before moving to the next step.
- Do not downgrade expectations, mark supported cases unsupported, or add
  testcase-shaped shortcuts.
- Add or update focused tests only to cover existing supported behavior.
- Escalate to LIR/codegen subsets only if emitted names, link names, struct
  layout, or codegen type output changes unexpectedly.
- Keep routine execution progress in `todo.md`; rewrite this runbook only for
  lifecycle-level route changes or blockers.

## Validation Strategy

Minimum proof for implementation packets:

- `cmake --build --preset default`
- `ctest --test-dir build -R "frontend_hir_tests|cpp_hir_template_global_specialization|cpp_hir_if_constexpr_branch_unlocks_later|cpp_hir_multistage_shape_chain" --output-on-failure`

Focused proof should cover:

- namespace-qualified function lookup
- namespace-qualified global lookup
- concrete `DeclRef` IDs still winning over names
- `LinkNameId` fallback and preservation
- template global specialization lookup
- consteval or bodyless callable materialization
- HIR dump parity where both structured and legacy lookup are available

## Ordered Steps

### Step 1: Add HIR module declaration lookup key

Goal: introduce the structured identity value used by module function/global
lookup without changing lookup behavior.

Primary target: HIR module declarations and supporting hash/equality helpers.

Actions:

- Add a key that captures declaration kind, namespace context ID,
  global-qualified state, qualifier segment text IDs, and unqualified
  declaration `TextId`.
- Keep the key HIR-owned and limited to module functions/globals.
- Add deterministic hashing/equality helpers.

Completion check:

- The project builds.
- No existing HIR lookup behavior changes.

### Step 2: Add declaration-side name text metadata

Goal: preserve source declaration name identity on HIR functions and globals.

Primary target: `Function` and `GlobalVar` model definitions.

Actions:

- Add `Function::name_text_id`.
- Add `GlobalVar::name_text_id`.
- Preserve existing rendered `name` fields unchanged.

Completion check:

- Existing creation sites compile after explicit initialization.
- Rendered names remain available for diagnostics, mangling, and codegen.

### Step 3: Populate function/global `name_text_id`

Goal: carry existing AST/HIR metadata into the new declaration-side fields.

Primary target: normal lowering, bodyless or consteval callable registration,
global lowering, and template/global registration paths.

Actions:

- Populate function `name_text_id` from available declaration metadata.
- Populate global `name_text_id` from available declaration metadata.
- Keep fallback behavior intact where metadata is unavailable.

Completion check:

- Focused HIR tests continue to pass.
- Missing metadata does not break legacy rendered-name lookup.

### Step 4: Dual-write module structured mirrors

Goal: build structured module mirrors beside existing rendered maps.

Primary target: `Module::fn_index` and `Module::global_index` ownership area.

Actions:

- Add a structured function lookup mirror.
- Add a structured global lookup mirror.
- Dual-write mirrors from the same insertion points that update rendered maps.
- Preserve rendered maps as authoritative fallback during the proof window.

Completion check:

- Function/global insertion updates both rendered and structured indexes when
  structured metadata exists.
- Existing rendered map consumers keep working.

### Step 5: Add structured `DeclRef` lookup helpers

Goal: read the structured mirrors for source-identity lookup while preserving
current precedence.

Primary target: HIR `DeclRef` resolution helpers for module functions/globals.

Actions:

- Keep concrete IDs as the first lookup authority.
- Keep `LinkNameId` bridge behavior intact.
- Use structured source-key lookup when name text and namespace metadata are
  available.
- Fall back to legacy rendered `fn_index` / `global_index` lookup.

Completion check:

- Namespace-qualified function/global lookup works through structured keys
  where metadata is present.
- Legacy lookup still handles cases without structured metadata.

### Step 6: Add parity checks during migration

Goal: prove structured lookup and rendered lookup agree where both are
available, without changing user-visible behavior.

Primary target: module lookup helper paths and HIR debug/parity reporting.

Actions:

- Compare structured and rendered lookup results when both inputs can be
  formed.
- Return existing behavior during the proof window.
- Make mismatches visible enough for focused tests or debug output to catch.

Completion check:

- HIR dump or focused diagnostics can demonstrate parity.
- No mismatch path silently changes the selected declaration.

### Step 7: Strengthen focused HIR proof if needed

Goal: cover existing supported behavior touched by the mirror.

Primary target: focused HIR tests only.

Actions:

- Add or update tests for namespace-qualified function lookup.
- Add or update tests for namespace-qualified global lookup.
- Cover concrete ID precedence, `LinkNameId` preservation, template global
  specialization, and consteval/bodyless callable materialization when existing
  supported behavior lacks coverage.

Completion check:

- The focused HIR subset passes without expectation downgrades.
- Test changes validate semantic behavior, not testcase-shaped shortcuts.

### Step 8: Add targeted lookup proof/instrumentation

Goal: keep rendered fallback parked while proving which lookup authority
resolves each module function/global reference path.

Primary target: module function/global lookup helpers, HIR debug/parity
reporting, and focused HIR proof.

Actions:

- Add targeted proof or instrumentation that distinguishes structured hits,
  legacy rendered hits, concrete-ID hits, and `LinkNameId` hits.
- Cover both module function and module global references.
- Preserve existing lookup precedence while collecting proof:
  concrete declaration IDs first, `LinkNameId` bridge next where applicable,
  structured lookup when metadata is complete, then legacy rendered fallback.
- Make the proof visible in focused HIR output, diagnostics, counters, or an
  equivalent supervisor-reviewable artifact.
- Do not demote rendered fallback in this step.

Completion check:

- Focused proof shows which path resolved the intended structured, legacy,
  concrete-ID, and `LinkNameId` cases for functions and globals.
- Existing supported behavior still passes without expectation downgrades.
- Supervisor has enough evidence to decide whether fallback demotion is safe
  or should remain parked.

### Step 9: Audit and remediate legacy fallback readiness

Goal: enumerate remaining legacy-rendered hits and metadata gaps before any
fallback demotion is considered.

Primary target: module function/global lookup proof output, declaration
metadata propagation, and narrow HIR fixtures that exercise remaining
legacy-rendered paths.

Actions:

- Use the accepted Step 8 authority proof to enumerate every remaining
  legacy-rendered module function/global hit in focused coverage.
- Classify each legacy hit as expected compatibility behavior, missing
  declaration metadata, incomplete qualifier metadata, or another concrete
  migration gap.
- Remediate narrow metadata gaps only when existing AST/HIR metadata already
  supports a behavior-preserving structured key.
- Preserve legacy rendered fallback for cases that still lack complete
  structured metadata or intentionally require rendered/link identity.
- Do not change fallback precedence or remove rendered lookup in this step.
- Record any remaining demotion blockers in `todo.md` for supervisor review.

Completion check:

- Focused proof identifies remaining legacy-rendered hits and metadata gaps.
- Remediated cases move to structured lookup without behavior changes.
- Remaining fallback users are explicitly classified.
- Supervisor has enough evidence to approve, narrow, or reject any later
  fallback demotion packet.

### Step 10: Demote legacy fallback only after explicit approval

Goal: shift semantic lookup authority away from rendered fallback only if
Step 9 audit/remediation proves the remaining surface is safe.

Primary target: lookup helper precedence and migration notes.

Actions:

- Require explicit supervisor approval before changing fallback authority.
- Demote rendered fallback only if Step 9 evidence and regression stability are
  approved.
- Keep rendered names and link IDs for codegen, diagnostics, and ABI/link
  output even if semantic lookup authority shifts.
- If proof is incomplete or ambiguous, leave demotion parked for a later idea
  instead of weakening behavior.

Completion check:

- Supervisor explicitly approves fallback demotion or leaves it parked.
- Rendered-name preservation remains explicit.

## Acceptance Criteria

- HIR functions and globals preserve declaration-side `TextId` metadata.
- `Module` has structured lookup mirrors for functions and globals beside
  existing rendered-name maps.
- HIR declaration lookup can use structured source identity when available
  while preserving concrete ID, `LinkNameId`, and rendered-name fallback
  behavior.
- Rendered names, mangled names, diagnostics, and link/codegen output remain
  unchanged.
- Struct/type, member/method, template registry, enum/const-int, and consteval
  environment cleanup remain outside this first slice.
- Focused HIR proof passes without expectation downgrades or testcase-shaped
  shortcuts.
