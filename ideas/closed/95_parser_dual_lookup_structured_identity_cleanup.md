# Parser Dual-Lookup Structured Identity Cleanup

Status: Closed
Created: 2026-04-25
Last Updated: 2026-04-25

Parent Ideas:
- [84_parser_qualified_name_structured_lookup.md](/workspaces/c4c/ideas/closed/84_parser_qualified_name_structured_lookup.md)
- [86_parser_alias_template_structured_identity.md](/workspaces/c4c/ideas/closed/86_parser_alias_template_structured_identity.md)
- [87_parser_visible_name_resolution_structured_result.md](/workspaces/c4c/ideas/closed/87_parser_visible_name_resolution_structured_result.md)

## Goal

Finish the parser-side transition from rendered-string lookup identity to
structured `TextId` / `QualifiedNameKey` identity by first running old and new
lookup paths in parallel, then gradually removing parser-owned string lookup
fallbacks once the paired results are proven stable.

This idea intentionally starts with dual-read / dual-write compatibility.
The first acceptance target is not deleting all old tables. The first target is
making the structured path authoritative enough that baseline drift is visible
before any legacy lookup path is removed.

## Why This Idea Exists

Recent parser lookup work moved the main lexical and qualified lookup model
toward `TextId`, `NamePathId`, and `QualifiedNameKey`:

- local lexical typedef/value visibility now has `TextId`-native scope tables
- qualified function/type/concept lookup has structured key tables
- template and alias lookup already has structured maps next to older string
  maps

However, several parser-owned paths still perform semantic lookup by rendering
qualified names back to strings or by using string-composed cache keys. Some of
those strings are unavoidable while AST, sema, HIR, and codegen still consume
`Node::name`, `TypeSpec::tag`, and rendered symbol names. Those bridge strings
should remain for now.

The cleanup target is narrower:

- remove parser-owned string fallback as a primary lookup mechanism
- keep downstream-facing string surfaces intact
- introduce paired structured lookup beside the old path before deleting either
  path
- prove that old and new paths agree before each removal

## Main Objective

Make parser-owned lookup tables and caches use structured identity as the
primary semantic key, while preserving rendered names only as compatibility
output for AST/HIR/sema/codegen and diagnostics.

The migration rule is:

1. Add the structured table or structured lookup helper.
2. Populate both the structured path and the legacy string path.
3. Query both paths in focused debug/proof points and detect mismatches.
4. Keep behavior unchanged while mismatches are possible.
5. Remove or demote the string path only after matching proof is stable.

## Primary Parser Scope

- `src/frontend/parser/impl/parser_state.hpp`
- `src/frontend/parser/impl/core.cpp`
- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/parser/parser.hpp`
- focused parser tests in `tests/frontend/frontend_parser_tests.cpp`

## Parser-Owned Cleanup Targets

### A. Structured Value Binding Lookup

The `using` value import path still renders a structured key back to text and
then probes value bindings through `find_var_type(std::string)`.

Expected direction:

- add a structured value-binding lookup/register path keyed by
  `QualifiedNameKey`
- keep the existing string-backed value table populated during migration
- in import/alias paths, compute the structured key once and query the
  structured table first
- optionally compare structured and legacy string results during focused proof
- keep rendered names only for AST compatibility and diagnostics

### B. `using` Value Alias Compatibility Names

`UsingValueAlias` already stores a `QualifiedNameKey`, but it also stores a
rendered compatibility name.

Expected direction:

- treat `target_key` as the semantic identity
- keep `compatibility_name` as a bridge/output field only
- ensure alias resolution does not require the rendered compatibility spelling
  when the structured target can resolve
- add mismatch checks before demoting any string-based alias result

### C. Structured NTTP Default Expression Cache Keys

`nttp_default_expr_tokens` currently uses a string key shaped like
`template-name:param-index`.

Expected direction:

- introduce an explicit structured key such as
  `QualifiedNameKey + parameter_index`
- populate the structured cache beside the string cache
- read both caches during migration and prove they agree
- keep the string cache only as a temporary compatibility mirror until the
  parser path no longer needs it

### D. Structured Template Instantiation De-Dup Keys

`instantiated_template_struct_keys` is parser-owned de-dup state and should not
need to remain keyed by rendered strings forever.

Expected direction:

- introduce a structured instantiation key that captures primary template
  identity plus structured type/value arguments
- populate the structured de-dup set beside the legacy string set
- use paired checks to ensure no duplicate or missing instantiations appear
- keep the final instantiated AST/HIR-facing names rendered as strings

### E. Parser API Surface Cleanup

The parser still exposes and internally uses string-first lookup/register
helpers such as:

- `find_typedef_type(std::string_view)`
- `find_var_type(const std::string&)`
- `register_typedef_binding(const std::string&, ...)`
- `register_var_type_binding(const std::string&, ...)`
- string overloads for known function names

Expected direction:

- add or prefer ID-first alternatives for parser-owned call sites
- keep string overloads as explicit bridge helpers while downstream users still
  need rendered names
- update parser tests to exercise both paths during transition
- do not delete public compatibility overloads until callers are migrated

## Downstream Bridge Scope

Some string-keyed structures remain because later stages still consume
rendered identity. These should be extended to dual lookup where practical, but
not cleaned up in the first parser-only pass:

- `Node::name`, `TypeSpec::tag`, and AST member typedef spellings
- parser `struct_tag_def_map` / `defined_struct_tags` used by parser constant
  evaluation and later HIR/sema expectations
- template primary/specialization string mirrors consumed by HIR lowerer and
  compile-time engine
- HIR module maps keyed by struct/function/global rendered names
- sema/consteval maps keyed by rendered variable/type/function names
- codegen/link names and diagnostics

For these downstream-bound paths, the intended first move is dual lookup or
dual metadata, not removal:

- preserve rendered-name behavior exactly
- add structured IDs beside rendered names where the owning data model can
  carry them
- compare structured and string lookups in focused proof points
- defer deletion of string maps until the downstream stage has its own cleanup
  idea

## Validation Strategy

Baseline stability is part of the feature.

At minimum for each slice:

- run `cmake --build --preset default`
- run `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

For any slice that changes lookup behavior beyond parser-only tables:

- run matching before/after regression logs with the same CTest command
- compare with `c4c-regression-guard`
- escalate to broader `ctest --test-dir build -j --output-on-failure` when the
  slice touches HIR/sema-facing data models or template instantiation behavior

Proof should include focused cases for:

- `using` value imports and namespace aliases
- qualified value/type lookup with namespace and local shadowing
- template primary/specialization lookup
- NTTP default argument evaluation
- template instantiation de-duplication
- bridge fallback paths where HIR/sema still expects strings

## Non-Goals

- no immediate removal of AST/HIR/sema/codegen string identity
- no broad HIR or sema data model rewrite in the parser cleanup slice
- no expectation downgrades to make lookup mismatch tests pass
- no testcase-shaped special cases for individual qualified names
- no change to generated symbol/link names except where explicitly proved
  equivalent
- no single large switch from old lookup to new lookup without a dual-lookup
  proof window

## Suggested Execution Decomposition

1. Add structured value binding storage and dual-write registration.
2. Retarget `using` value import lookup to structured-first / legacy-checked
   behavior.
3. Add structured `UsingValueAlias` resolution and demote
   `compatibility_name` to bridge-only use.
4. Add structured NTTP default-expression cache keys and dual-read checks.
5. Add structured template instantiation de-dup keys and dual-read checks.
6. Add ID-first parser helper overloads and migrate parser-owned call sites.
7. Extend downstream-facing AST/HIR bridge metadata with dual lookup where
   practical, but leave cleanup/removal to follow-on ideas.
8. Remove parser-owned legacy string lookup paths only after matching proof is
   stable and regression guard shows no baseline drift.

## Acceptance Criteria

- Parser-owned value binding lookup has a structured primary path with legacy
  string lookup used only as compatibility/mismatch proof.
- `using` value alias resolution can succeed through structured identity
  without depending on rendered compatibility names.
- NTTP default-expression cache lookup has a structured key path and paired
  legacy proof.
- Template instantiation de-duplication has a structured key path and paired
  legacy proof.
- Parser-owned call sites prefer ID-first helpers where structured identity is
  already available.
- Downstream string consumers remain behaviorally unchanged.
- Focused parser proof passes.
- Any broader touched baseline has matching before/after regression logs with
  no unexpected drift.

## Closure Summary

Closed after the active runbook completed the parser-owned dual-lookup
migration through Step 8. Parser-owned value binding lookup, `using` value
aliases, NTTP default-expression caching, template instantiation
de-duplication, ID-first parser helper use, and known-function registration now
prefer structured `TextId` / `QualifiedNameKey` identity where that identity is
available.

Remaining rendered-string paths are explicit compatibility bridges for public
string-facing APIs, TextId-less callers, downstream AST/HIR/sema/codegen
surfaces, diagnostics, and mismatch proof. Those downstream rendered-identity
surfaces are outside this parser-only idea and remain follow-on cleanup scope,
not blockers for idea 95.

Focused parser proof passed during the final Step 8 packet. Close validation
used matching full-suite CTest logs: `test_before.log` and `test_after.log`
both report 2974/2974 passing tests, and the regression guard passed in
documented non-decreasing refactor mode with no new failures.
