# LIR Module Declaration Type Shadow Convergence Runbook

Status: Active
Source Idea: ideas/open/762_lir_module_declaration_type_shadow_convergence.md

## Purpose

Retire module-level rendered type text as semantic authority only where an
accepted structured carrier already exists, preserving output compatibility.

## Goal

Make structured IDs and type references authoritative for extern declarations,
function signatures, globals, and struct declarations without broad type-model
or backend rewrites.

## Core Rule

Do not recover or choose module identity from rendered declaration/type text.
Legacy strings may remain only as documented compatibility or emission shadows
after an equivalent structured source is checked.

## Read First

- `ideas/open/762_lir_module_declaration_type_shadow_convergence.md`
- `ideas/closed/759_lir_typed_ref_enum_foundation.md`
- `ideas/closed/760_lir_string_constructor_deprecation_migration.md`
- `ideas/closed/761_lir_call_signature_type_mirror_convergence.md`
- `ideas/closed/763_lir_composite_type_ref_model.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`, and
  `src/codegen/lir/print.cpp`

## Non-Goals

- Full type-tree replacement, wholesale `TypeSpec` lowering, or broad printer
  rewrites.
- Raw-BIR, target lowering, MIR, global-initializer semantic expansion, data
  layout, inline assembly, or template-specialization work.
- Removing a compatibility shadow before all selected producer/consumer routes
  have an equivalent structured replacement.

## Execution Rules

- Select and implement one module-level authority surface at a time.
- Preserve final LLVM output; test stale or misleading shadow text as a
  negative authority case.
- Fail closed when the selected route lacks an exact structured replacement.
- Run a fresh build and nearby LIR/frontend/backend proof for each code step;
  the supervisor selects broader/full checkpoints as shared paths require.

## Steps

### Step 1 - Audit and select one module declaration authority surface — complete

Goal: inspect extern declarations, function signatures, globals, and struct
declarations against the accepted 759/760/761/763 structured carriers; select
exactly one surface whose producer, verifier, printer, and consumer seam is
complete.

Actions:

- Trace the selected surface from module recording through verification,
  printing, and backend consumption.
- Record the structured authority fields, legacy shadow fields, stale-shadow
  positive/negative matrix, and excluded surfaces in `todo.md`.
- If no selected surface has an equivalent structured carrier, classify the
  missing carrier and route it as a separate blocker rather than parsing text.

Completion check: one bounded authority contract is selected with an exact
same-feature proof ladder, or a separately scoped blocker preserves this Step
1 return point.

Accepted in `18ef8e456`: selected only `LirModule::struct_decls` as authority,
with `type_decls` retained as its checked compatibility shadow. Extern,
function-signature, and global routes remain excluded because their current
producer/printer/consumer seams still consume text.

### Step 2 - Implement and prove the selected struct-declaration authority surface — complete

Goal: make only `LirModule::struct_decls` authoritative and retain matching
`type_decls` solely as a checked compatibility/emission shadow. Preserve the
selected structured `name_id`, fields, packed, and opaque facts through
verification, printing, and the BIR structured-layout consumer.

Completion check: fresh build; nearby positive and malformed stale-shadow
coverage; `^backend_lir_to_bir_interface$` (extended for this seam or replaced
by a registered exact struct-declaration test); and a supervisor-selected
broader checkpoint prove the row without output-compatibility change or weaker
contracts. `frontend_lir_extern_decl_type_ref` is not selected proof because
it exercises the excluded extern route.

Accepted in `51ee839bc`: `backend_lir_to_bir_interface` now proves the
structured packed/recursive declaration pair supplies the retained
`type_decls` shadow and Raw-BIR structured layout facts, while mutating only a
shadow line rejects before BIR import. Fresh builds passed the target 1/1 and
the broader `^backend_` 6/6 run; the matching equal-count regression guard
passed and supervisor direct review found no defect.

### Step 3 - Reassess remaining module-level surfaces — complete

Goal: decide whether the other extern/signature/global/struct surfaces require
another in-scope one-row route or whether the source acceptance criteria are
complete.

Completion check: close only with all source criteria and accepted proof;
otherwise repair this runbook for the next bounded surface or create/switch to
an out-of-scope blocker with an exact return point.

Reassessed by AST-backed route audit: `LirExternDecl::return_type` is an exact
structured carrier and the verifier already validates `return_type_str` only
as its shadow, but `BirFunctionLowerer::lower_extern_decl` still lowers the
rendered `decl.return_type_str` first and uses the structured carrier only as a
fallback. This is an in-scope authority defect, not a missing-carrier blocker.
Before-change proof passed: a fresh build and
`ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_extern_decl_type_ref|backend_lir_to_bir_interface)$'`
passed 2/2; the baseline is recorded in `test_before.log`.

### Step 4 - Make structured extern return type authoritative — complete

Goal: make `LirExternDecl::return_type` the primary input to
`BirFunctionLowerer::lower_extern_decl`, retaining `return_type_str` only for
the absent-structured-carrier compatibility path.

Actions:

- Lower extern return information from `decl.return_type` whenever it is
  present; never let a present structured carrier be overridden by rendered
  `decl.return_type_str`.
- Use `return_type_str` only when the structured carrier is absent, preserving
  the existing valid LLVM output and fail-closed verifier contract.
- Add same-feature stale-shadow coverage across
  `frontend_lir_extern_decl_type_ref` and `backend_lir_to_bir_interface` that
  proves misleading rendered return text cannot override the structured ref.
- Build, run both exact tests, and retain the matching after-run evidence for
  supervisor regression comparison.

Completion check: the backend consumes the structured extern return type first,
the renderer text is fallback-only when no structured carrier exists, both
focused frontend/backend tests prove stale-shadow rejection or non-authority,
and valid LLVM output remains unchanged.

Accepted in `44ca942cf`: `BirFunctionLowerer::lower_extern_decl` now lowers a
present `LirExternDecl::return_type` before consulting `return_type_str`.
Focused frontend and backend stale-shadow proof passed 2/2 before and after;
the matching regression guard was clean.

### Step 5 - Prove aggregate parameter lowering uses structured function signatures — complete

Goal: make the aggregate-parameter consumer seam in
`src/backend/bir/lir_to_bir/aggregate.cpp` demonstrably authoritative on
`LirFunction::signature_params` and `signature_param_type_refs` whenever they
are complete, with `signature_text` limited to the legacy no-metadata path.

Actions:

- Audit the aggregate parameter route through
  `BirFunctionLowerer::lower_aggregate_params` and its structured-signature
  availability predicate; retain text parsing only when the exact structured
  parameter carrier is absent or incomplete.
- Correct any path that can select parsed `signature_text` while complete
  structured signature parameters and type refs are present.
- Add a backend LIR-to-BIR stale-signature-shadow case whose rendered parameter
  spelling conflicts with complete structured parameter facts, proving the
  aggregate ABI route follows structured metadata and preserves valid LLVM
  output.
- Build and run the exact affected backend interface test before and after the
  packet; retain matching evidence for supervisor regression comparison.

Completion check: complete structured signature parameters and refs govern the
aggregate ABI route; `signature_text` is unreachable as semantic authority in
that case, legacy no-metadata fallback remains explicit, stale-shadow coverage
is nearby and non-testcase-shaped, and valid output is preserved.

Accepted in `4211a9373`: complete paired `signature_params` and
`signature_param_type_refs` now govern aggregate parameter collection; the
backend interface test uses a syntactically valid conflicting signature shadow
and the targeted build/test proof passed 1/1.

### Step 6 - Prove global type consumers use the structured global type ref — complete

Goal: close the remaining global acceptance row only if the existing
`LirGlobal::llvm_type_ref` route is authoritative whenever exact identity is
present, while keeping `llvm_type` as an explicitly checked compatibility and
emission shadow.

Actions:

- Audit `lower_minimal_global`, the aggregate-layout and initializer routes,
  and global verifier/printer seams. Distinguish exact `llvm_type_ref` cases
  from deliberately legacy/no-metadata fallbacks.
- Correct any selected global consumer that still chooses `llvm_type` while a
  complete exact `llvm_type_ref` is present; fail closed rather than accepting
  conflicting structured and rendered facts.
- Add nearby stale-global-shadow coverage in the established LIR-to-BIR or
  frontend global suite. The case must mutate only a rendered global type
  shadow while retaining valid structured identity, and prove either rejection
  or structured-authority behavior without weakening verifier contracts.
- Build and run the exact affected test before and after the packet. Select a
  broader LIR/frontend/backend checkpoint if the touched consumer is shared.

Completion check: every selected exact-identity global route consumes
`llvm_type_ref` before `llvm_type`; legacy text-only globals retain an explicit
compatibility path; misleading global type text cannot override structured
identity; and the focused proof preserves valid output. Reassess the source
acceptance criteria after this step before attempting closure.

Accepted in `cc24bfa30`: `lower_minimal_global` now derives scalar, array,
aggregate, and initializer handling from `llvm_type_ref::render_llvm()` when
present and keeps `llvm_type` as the no-metadata compatibility path. Focused
frontend/backend proof passed 2/2 before and after, the regression guard was
non-decreasing, and the broader `^backend_` checkpoint passed 6/6.

### Step 7 - Prove general function-signature lowering is structured-first — current

Goal: discharge the remaining function-signature acceptance row by proving
ordinary parameter and return lowering, including declaration and definition
entry paths, cannot take semantic type facts from `signature_text` when
complete structured signature metadata is present.

Actions:

- Audit `lower_signature_return_info`, `lower_function_params_with_layouts`,
  and their declaration/definition callers alongside
  `verify_function_signature_type_ref_shadows`. Record the exact complete and
  absent/incomplete-metadata predicates; retain the legacy parsing path only
  where the structured carrier is genuinely absent.
- Correct any path that selects a rendered return or parameter spelling while
  `signature_return_type_ref`, `signature_params`, and matching
  `signature_param_type_refs` are complete. Preserve fail-closed behavior for
  malformed mixed metadata.
- Add focused stale-signature-shadow coverage that conflicts in a general
  return or non-aggregate parameter spelling and exercises the selected
  declaration/definition lowering route. Do not rely solely on the Step 5
  aggregate ABI case.
- Build and run the exact affected frontend/backend tests before and after;
  select a broader checkpoint if shared call-ABI code changes.

Completion check: verifier and selected backend declaration/definition paths
use complete structured signature metadata before `signature_text`; the
rendered signature remains only output or absent-metadata compatibility; a
nearby stale-shadow case proves non-authority; and the final source acceptance
reassessment has explicit evidence for externs, signatures, globals, and
struct declarations.
