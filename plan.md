# HIR Rendered Registry Mirror Retirement Audit Runbook

Status: Active
Source Idea: ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md

## Purpose

Audit remaining HIR rendered registries and retire or fence string-keyed mirrors
that should no longer act as ordinary semantic authority.

## Goal

Classify HIR module, compile-time, and lowerer rendered registries, then remove
or narrow mirrors with complete structured replacements while preserving
explicit compatibility, display, insertion-order, diagnostic, and route-local
storage.

## Core Rule

Do not allow a rendered registry to remain ordinary semantic authority when a
complete structured key exists. Complete structured misses must not silently
fall back to rendered maps unless the path is an explicit no-metadata
compatibility boundary.

## Read First

- Source idea:
  `ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md`
- Parent idea:
  `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- Parent idea:
  `ideas/closed/162_link_name_id_backend_symbol_authority.md`
- HIR module registry definitions and lookup helpers
- `CompileTimeState` template, consteval, enum, and const-int registries
- HIR lowerer record/template/global/static-member/method/constructor/
  destructor/overload registries
- Existing focused HIR tests for records, templates, globals, functions,
  consteval, static members, and structured lookup

## Current Scope

- Inventory rendered registries in HIR module state, compile-time state, and
  lowerer state.
- Classify each registry as structured authority, rendered compatibility
  mirror, insertion-order/display storage, diagnostic support, or route-local
  generated-name storage.
- Retire rendered maps that have complete structured replacements and no
  compatibility caller.
- For retained maps, make lookup APIs prefer structured keys and limit rendered
  access to explicit no-metadata compatibility.
- Add parity or fail-closed checks where structured and rendered paths can
  resolve conflicting entities.
- Add focused tests for same-spelled records/templates/globals/functions in
  different structured domains.

## Candidate Registries

- `Module::struct_defs`
- `Module::struct_def_owner_index`
- `Module::fn_index`
- `Module::global_index`
- `CompileTimeState::template_fn_defs_`
- `CompileTimeState::template_struct_defs_`
- `CompileTimeState::template_struct_specializations_`
- `CompileTimeState::consteval_fn_defs_`
- `CompileTimeState::enum_consts_`
- `CompileTimeState::const_int_bindings_`
- lowerer `struct_def_nodes_`
- lowerer `template_struct_defs_`
- lowerer `template_global_defs_`
- lowerer `struct_static_member_decls_`
- lowerer `struct_methods_`
- lowerer constructor/destructor maps
- lowerer overload maps

## Non-Goals

- Do not rework template binding internals completed by idea 161.
- Do not rework final backend `LinkNameId` transport completed by idea 162.
- Do not replace route-local generated names such as SSA values, labels, slots,
  or dump-only strings.
- Do not remove user-facing diagnostics, final display spelling, or
  insertion-order storage that still has a real non-semantic purpose.
- Do not claim progress through dump-text-only changes, helper renames, or
  expectation weakening.

## Working Model

- Rendered registries may still be valid as display/order storage,
  diagnostics, or no-metadata compatibility.
- Structured mirrors should be semantic authority when they carry complete
  domain identity.
- Some rendered maps may be retained because external compatibility callers,
  dump surfaces, or legacy no-metadata routes still need them.
- The desired end state is explicit: each rendered registry is retired,
  narrowed, or documented with owner, limitation, and removal condition.

## Execution Rules

- Start with inventory. Do not edit implementation or tests before recording
  registry classification and first target in `todo.md`.
- Convert or retire one registry family at a time so validation can isolate
  behavior and route quality.
- Preserve display, diagnostic, and insertion-order strings unless the
  inventory proves they are unused.
- Fail closed on complete structured misses for covered semantic lookup paths.
- Keep rendered lookup only behind explicit no-metadata compatibility
  boundaries.
- Escalate validation beyond narrow focused tests when shared HIR module,
  compile-time-state, or lowerer registry APIs change.

## Ordered Steps

### Step 1: Inventory Rendered Registry Families

Goal: identify and classify every candidate rendered registry in module,
compile-time, and lowerer state.

Primary targets:
- HIR module registry fields and lookup helpers
- `CompileTimeState` registry fields and lookup helpers
- lowerer record/template/global/static-member/method/ctor/dtor/overload maps
- tests that exercise same-spelled records, templates, globals, functions, and
  static members across structured domains

Actions:
- Find insertions, lookups, iteration users, dumps, diagnostics, and fallback
  paths for each candidate registry.
- Classify each registry as structured authority, rendered compatibility
  mirror, insertion-order/display storage, diagnostic support, route-local
  generated-name storage, or unrelated.
- Identify which registries have complete structured replacements and which
  still lack metadata.
- Record the classification, retained boundaries, first implementation target,
  and proof recommendation in `todo.md`.

Completion check:
- `todo.md` lists each candidate registry family with classification and first
  target.
- No implementation or test files are edited before the inventory is recorded.

### Step 2: Define Retirement and Fence Policy

Goal: decide the allowed action for each registry before changing behavior.

Primary targets:
- registries classified as semantic authority or compatibility mirrors
- lookup APIs that can resolve both structured and rendered paths

Actions:
- For each complete structured replacement, choose retire, narrow, or
  fail-closed behavior.
- For each retained rendered mirror, document owner, limitation, and removal
  condition in `todo.md`.
- Identify conflict cases where structured and rendered lookup can disagree.
- Select focused tests that prove structured-domain separation for the first
  target family.

Completion check:
- Every candidate registry has a planned action: retire, narrow, retain/fence,
  or no-op with reason.
- First target has a concrete validation command and expected semantic proof.

### Step 3: Retire or Narrow the First Complete Registry Family

Goal: remove or restrict the first rendered registry family that has complete
structured authority and no ordinary semantic need.

Primary targets:
- first target selected in Step 1 or Step 2
- associated structured lookup helper and rendered fallback helper
- focused tests for that registry family

Actions:
- Route complete metadata callers through structured keys first.
- Remove unused rendered mirror storage or narrow access to explicit
  compatibility/display helpers.
- Fail closed on complete structured misses where rendered fallback would hide
  a mismatch.
- Add same-spelled or structured-domain collision coverage that fails under
  ordinary rendered authority.

Completion check:
- The first registry family no longer uses rendered lookup as ordinary
  authority when complete structured metadata is present.
- Focused proof passes and is recorded in `todo.md`.

### Step 4: Process Remaining Module and Compile-Time Registries

Goal: finish HIR module and `CompileTimeState` rendered registry boundaries.

Primary targets:
- `Module::struct_defs`, `struct_def_owner_index`, `fn_index`, `global_index`
- `CompileTimeState::template_fn_defs_`, `template_struct_defs_`,
  `template_struct_specializations_`, `consteval_fn_defs_`, `enum_consts_`,
  and `const_int_bindings_`

Actions:
- Apply the Step 2 policy one registry family at a time.
- Retire or narrow rendered mirrors with complete structured replacements.
- Fence compatibility and display/order registries that must remain.
- Add parity or fail-closed checks where conflicting structured/rendered
  results are observable.
- Extend focused tests for records/templates and function/global lookup paths.

Completion check:
- Module and compile-time registries are retired, narrowed, or fenced.
- Covered lookup APIs prefer structured keys and do not silently reopen
  rendered lookup after complete structured misses.

### Step 5: Process Remaining Lowerer Registries

Goal: finish lowerer rendered registry boundaries without disturbing
route-local generated names or display-only strings.

Primary targets:
- lowerer `struct_def_nodes_`
- lowerer `template_struct_defs_`
- lowerer `template_global_defs_`
- lowerer `struct_static_member_decls_`
- lowerer `struct_methods_`
- lowerer constructor/destructor maps
- lowerer overload maps

Actions:
- Apply the Step 2 policy one registry family at a time.
- Preserve route-local generated-name storage and display strings when they are
  non-semantic.
- Narrow or fence rendered maps that remain for no-metadata compatibility.
- Add or extend same-spelled structured-domain tests where reachable.

Completion check:
- Lowerer registries are classified, retired/narrowed, or fenced.
- Retained lowerer strings have owner, limitation, and removal condition
  recorded in `todo.md`.

### Step 6: Final Proof and Closure Readiness

Goal: prepare the idea for supervisor review and lifecycle closure.

Actions:
- Re-run the focused HIR proof covering all converted or fenced registry
  families.
- Run broader HIR/frontend or full-suite validation if shared registry APIs or
  lookup behavior changed broadly.
- Confirm ideas 161 and 162 were not reopened.
- Record final retired mirrors, narrowed APIs, retained bridges, tests, proof
  commands, and residual blockers in `todo.md`.

Completion check:
- Source idea acceptance criteria are satisfied or exact blockers are recorded
  for lifecycle decision.
- Tests prove structured-domain separation for at least records/templates and
  function/global lookup paths still using rendered mirrors.
