# Template Scope Owner And Member Substitution Domain Keys

Status: Active
Source Idea: ideas/open/155_template_scope_owner_and_member_substitution_domain_keys.md

## Purpose

Replace remaining parser/template owner and substitution paths that use
rendered strings as semantic authority with structured template-domain keys.

Goal: make parser preserve template syntax carriers and make Sema/parser
substitution paths consume owner, parameter index, parameter kind, and
`TextId` metadata before any display-spelling fallback.

## Core Rule

Rendered template owner names, member typedef names, and NTTP parameter names
may remain as diagnostics, debug/display mirrors, final spelling, or explicit
compatibility bridges. They must not be the primary semantic key when
structured owner or parameter-domain metadata exists.

## Read First

- `ideas/open/155_template_scope_owner_and_member_substitution_domain_keys.md`
- Template scope and parameter carrier definitions:
  - `src/frontend/parser/parser_types.hpp`
  - `src/frontend/parser/impl/parser_state.hpp`
- Template declaration and instantiation surfaces:
  - `src/frontend/parser/impl/declarations.cpp`
  - `src/frontend/parser/impl/types/base.cpp`
  - `src/frontend/parser/impl/types/declarator.cpp`
- Existing structured member/NTTP carriers in:
  - `src/frontend/parser/ast.hpp`
  - `src/frontend/parser/impl/core.cpp`
  - `src/frontend/sema/consteval.cpp`
  - `src/frontend/sema/type_utils.cpp`

## Current Targets

- `ParserTemplateScopeFrame::owner_struct_tag` and all callers.
- Template member typedef cloning and registration during instantiation.
- NTTP array-size substitution paths that compare `array_size_expr->name`
  against string binding names.
- Member typedef lookup/substitution paths that can compare rendered member or
  owner names before structured owner/member metadata.
- Tests proving same-spelling template parameters or owners do not bind by
  display text.

## Non-Goals

- Do not redesign full template instantiation.
- Do not require every dependent expression to resolve in parser or Sema before
  HIR.
- Do not remove deferred expression tokens, syntax payloads, diagnostics, or
  final display spelling.
- Do not reopen structured argument key work from idea 149.
- Do not reopen parser deferred NTTP binding-set work from idea 150.
- Do not downgrade existing supported tests or weaken contracts to claim
  progress.

## Working Model

Parser owns template syntax carriers:

- owner syntax and provisional owner keys
- parameter name `TextId`s
- parameter index, kind, pack/default flags
- captured default/deferred syntax tokens
- provisional member typedef and instantiation carriers

Sema owns template-domain identity:

- owner template key
- parameter index/kind identity
- type/NTTP binding environments
- member typedef owner and member identity
- non-dependent substitution when enough metadata is available

HIR may complete dependent substitution later, but it should receive structured
binding carriers rather than rendered parameter-name strings.

## Execution Rules

- Prefer `QualifiedNameKey`, `ParserTemplateParameterBindingKey`,
  `TemplateParamDomainKind`, parameter index, and member `TextId` metadata over
  rendered strings.
- If a string fallback remains, make it an explicit compatibility or display
  path and record its removal condition in `todo.md`.
- Do not add testcase-shaped matching for a known fixture; repair the domain
  carrier or lookup rule that the fixture exercises.
- For code-changing steps, run `cmake --build --preset default` plus a focused
  CTest subset covering the touched parser/frontend template tests. Escalate
  to broader validation before closure.
- Keep source-idea edits out of routine execution unless durable source intent
  changes.

## Steps

### Step 1: Inventory Template Owner And Substitution String Authority

Goal: locate and classify the remaining string-authority paths named by the
source idea.

Primary targets:

- `ParserTemplateScopeFrame::owner_struct_tag`
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/types/declarator.cpp`
- `src/frontend/parser/impl/types/base.cpp`
- existing tests under `tests/frontend/` and `tests/cpp/internal/hir_case/`
  that mention template member typedef, NTTP, or stale owner metadata

Actions:

- Inspect every direct use of `owner_struct_tag`.
- Inspect NTTP substitution loops that compare `array_size_expr->name` or
  `ase->name` against rendered binding names.
- Inspect member typedef instantiation and lookup paths that compare member or
  owner rendered strings.
- Classify each path as semantic lookup, compatibility bridge, display/debug
  projection, or syntax payload.
- Record the first code packet target, retained compatibility paths, and the
  focused proof command in `todo.md`.

Completion check:

- `todo.md` names the owned first repair path, nearby string fallbacks, and the
  initial validation subset without changing implementation files.

### Step 2: Add Structured Owner Carrier Beside Template Scope Display Text

Goal: keep `owner_struct_tag` as display/compatibility only by preserving a
structured owner key in the active template scope frame.

Actions:

- Add or reuse a `QualifiedNameKey`/template-owner key on
  `ParserTemplateScopeFrame` beside `owner_struct_tag`.
- Populate the key where template record owner syntax is known.
- Route declaration/declarator consumers through the structured key when
  deciding owner identity.
- Leave `owner_struct_tag` only for display, debug, or bounded compatibility.
- Add focused coverage where stale rendered owner text would otherwise select
  the wrong owner.

Completion check:

- Covered template-scope owner decisions no longer depend on rendered
  `owner_struct_tag` when a valid owner key exists, and focused tests prove the
  stale-spelling case.

### Step 3: Route Member Typedef Instantiation Through Owner/Member Keys

Goal: make template member typedef cloning and binding registration use
structured owner/member metadata before rendered member names or legacy tags.

Actions:

- Inspect member typedef cloning around template instantiation in
  `src/frontend/parser/impl/types/base.cpp`.
- Preserve owner key, owner argument metadata, member `TextId`, and alias
  template member typedef info through instantiated records.
- Prefer `record_member_typedef_key_from_owner_key`,
  `register_template_instantiation_member_typedef_binding`, and
  `register_dependent_record_member_typedef_binding` with structured keys.
- Keep rendered member names only as display/compatibility mirrors.
- Add or update tests with same-spelling member typedefs under different
  template owners.

Completion check:

- The covered instantiation/member typedef path resolves by owner/member
  metadata, and tests prove rendered spelling alone cannot pick the binding.

### Step 4: Replace NTTP Array-Size Name Matching With Parameter-Domain Lookup

Goal: remove primary substitution dependence on
`std::string(array_size_expr->name) == npname` when structured NTTP metadata
exists.

Actions:

- Inspect NTTP binding structures produced from template arguments and
  parameter declarations.
- Carry parameter index, parameter kind, and parameter `TextId` into array-size
  substitution sites.
- Substitute through structured NTTP parameter metadata before any spelling
  fallback.
- Leave spelling fallback only for legacy compatibility where no structured
  parameter metadata exists.
- Add tests with same-spelling NTTP names in different template-owner domains
  or stale expression text.

Completion check:

- Covered array-size substitution uses owner/index/kind metadata when present,
  and focused tests fail if substitution falls back to rendered parameter
  spelling first.

### Step 5: Audit And Demote Remaining Template Substitution String Fallbacks

Goal: ensure the idea's covered owner, member typedef, and NTTP substitution
paths consistently prefer structured domain keys.

Actions:

- Re-scan the targets from Step 1 after the code packets.
- For each retained string path, document whether it is display, diagnostics,
  syntax payload, final spelling, or compatibility.
- Remove or demote fallbacks that can override a structured metadata miss.
- Add nearby regression coverage for any newly demoted compatibility bridge.

Completion check:

- Remaining string paths in the covered surfaces cannot override structured
  owner/parameter/member metadata misses.

### Step 6: Validation And Closure Readiness

Goal: prove the route repaired semantic authority without overfitting one
fixture.

Actions:

- Run the final build proof: `cmake --build --preset default`.
- Run focused CTest coverage for touched parser/frontend template tests.
- Run a broader parser/frontend or full CTest checkpoint if multiple narrow
  packets landed or the touched surface spans parser and Sema.
- Confirm the source idea acceptance criteria are satisfied before requesting
  closure.

Completion check:

- Build and selected tests are green, `todo.md` records the exact proof, and
  no remaining covered path uses rendered template strings as primary semantic
  authority when structured metadata exists.
