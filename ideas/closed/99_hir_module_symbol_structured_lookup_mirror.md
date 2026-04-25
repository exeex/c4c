# HIR Module Symbol Structured Lookup Mirror

Status: Closed
Created: 2026-04-25
Last Updated: 2026-04-25
Closed: 2026-04-25

Parent Ideas:
- [97_structured_identity_completion_audit_and_hir_plan.md](/workspaces/c4c/ideas/closed/97_structured_identity_completion_audit_and_hir_plan.md)

## Goal

Start HIR structured-identity migration with a narrow, behavior-preserving
module function/global lookup mirror.

The first slice should add HIR-owned structured lookup beside existing
rendered-name maps, prove parity, and preserve rendered names for codegen,
diagnostics, mangled/template names, and link-name output.

## Why This Idea Exists

The idea 97 audit found many HIR string-keyed identity surfaces, but only the
module function/global lookup surface is narrow enough to start without a
struct/type identity rewrite. Existing AST and HIR metadata already carry
namespace qualifier text IDs, namespace context IDs, unqualified name text IDs,
concrete declaration IDs, and `LinkNameId`.

That makes module function/global lookup the right first HIR dual-write /
dual-read slice. It exercises the migration strategy while avoiding the broad
`TypeSpec::tag`, struct layout, method/member, consteval constant, and template
registry surfaces until one HIR mirror is proven stable.

## First Slice

Add a HIR declaration lookup key for module functions and globals.

Suggested key components:

- declaration kind: function or global
- namespace context ID
- global-qualified bit
- qualifier segment text IDs
- unqualified declaration `TextId`

Add declaration-side metadata:

- `Function::name_text_id`
- `GlobalVar::name_text_id`

Populate those fields from existing AST/HIR metadata during:

- normal function lowering
- consteval or bodyless callable registration
- global variable lowering
- template/global registration paths that already preserve rendered names

Add module mirrors beside the existing rendered-name maps:

- structured function lookup mirror beside `Module::fn_index`
- structured global lookup mirror beside `Module::global_index`

Add lookup helpers that preserve current behavior:

- concrete IDs win first when present
- `LinkNameId` remains a bridge for ABI/link-visible identity
- structured source-key lookup runs when `DeclRef::name_text_id` and namespace
  metadata are available
- legacy rendered `fn_index` / `global_index` lookup remains as fallback and
  parity proof during migration

## Rendered Name Preservation

Do not remove or reinterpret:

- `Function::name`
- `GlobalVar::name`
- `LinkNameId`
- rendered and mangled template names
- codegen-facing names
- diagnostic spellings

The structured key is for HIR semantic declaration lookup. Rendered strings and
link IDs remain required bridge output for emission, diagnostics, and ABI/link
behavior.

## Out Of Scope

- `Module::struct_defs`, `struct_def_order`, and `TypeSpec::tag` migration.
- Struct layout, base tags, codegen type declarations, and LLVM type names.
- Member, static-member, and method owner identity.
- Function-local symbol maps, labels, params, locals, and local const bindings.
- Compile-time engine template/consteval definition registries.
- Enum, const-int, and consteval runtime environment maps.
- Parser or sema rewrites. The first HIR slice should use metadata already
  available from AST/HIR lowering.
- Deleting legacy rendered-name maps before parity proof is stable.

## Validation Strategy

At minimum for the first implementation slice:

- run `cmake --build --preset default`
- run `ctest --test-dir build -R "frontend_hir_tests|cpp_hir_template_global_specialization|cpp_hir_if_constexpr_branch_unlocks_later|cpp_hir_multistage_shape_chain" --output-on-failure`

Focused proof should cover:

- namespace-qualified function lookup
- namespace-qualified global lookup
- concrete `DeclRef` IDs still winning over names
- `LinkNameId` fallback and preservation
- template global specialization lookup
- consteval or bodyless callable materialization
- HIR dump parity where both structured and legacy lookup are available

Escalate to LIR/codegen subsets only if emitted names, link names, struct
layout, or codegen type output changes unexpectedly.

## Suggested Execution Decomposition

1. Add the HIR module declaration lookup key and hashing/equality helpers.
2. Add `name_text_id` fields to `Function` and `GlobalVar`.
3. Populate function/global `name_text_id` during lowering and registration.
4. Dual-write structured module function/global mirrors beside rendered maps.
5. Add structured lookup helpers for `DeclRef` while preserving concrete ID and
   `LinkNameId` precedence.
6. Add parity checks between structured and rendered lookup where both are
   available, returning existing behavior during the proof window.
7. Add or update focused HIR tests only when needed to cover existing supported
   behavior, without weakening expectations.
8. Demote legacy fallback only after mismatch-free proof and supervisor-approved
   regression stability.

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
  environment cleanup remain outside the first slice.
- Focused HIR proof passes without expectation downgrades or testcase-shaped
  shortcuts.

## Closure Notes

Closed after Step 9 audit in `review/99_step9_legacy_fallback_readiness.md`.
The first HIR module function/global structured lookup mirror is complete:
declaration-side name metadata exists, structured mirrors are dual-written,
structured lookup is used when complete source identity metadata is available,
and concrete declaration IDs, `LinkNameId`, rendered-name fallback, diagnostics,
codegen names, and ABI/link-visible names remain preserved.

Legacy rendered fallback demotion is intentionally parked. The Step 9 audit
classified the remaining legacy-rendered hits as compatibility cases for
missing declaration metadata or incomplete qualifier metadata, not as safe
metadata propagation gaps available to remediate from existing AST/HIR metadata.
Supervisor approval for Step 10 fallback demotion was not granted, and no
fallback precedence changes were made.

Close-time regression guard used the existing focused HIR before/after logs:
`test_before.log` and `test_after.log` both passed 73/73 tests, and
`check_monotonic_regression.py --allow-non-decreasing-passed` reported PASS.
