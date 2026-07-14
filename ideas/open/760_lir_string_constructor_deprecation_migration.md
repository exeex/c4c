# LIR string constructor deprecation migration

## Intent

Use targeted `[[deprecated]]` warnings to expose remaining LIR string-based ref
construction sites, then migrate them incrementally to enum constructors or
explicit runtime-text factories.

This idea depends on the LIR typed ref enum foundation being available first.

## Why This Exists

After enum constructors exist, the next risk is silent continued use of generic
string constructors. A direct removal experiment showed that string
construction is still used by several different runtime sources, so a hard
delete would turn a design migration into a large mixed rewrite.

Deprecation warnings provide a controlled way to classify each use:

- closed builtin or opcode spellings that should become enum construction;
- HIR `TypeSpec`-derived values that should flow through a typed lowering
  helper;
- dynamic aggregate/vector/struct/function spellings that should be marked as
  explicit runtime text;
- verifier compatibility code that should stop reparsing once typed authority
  exists.

## In Scope

- Add `[[deprecated]]` annotations to legacy string constructors or string-only
  factories for LIR ref classes after enum constructors are available.
- Compile with warnings visible enough to inventory call sites without treating
  the whole migration as one giant edit.
- Convert straightforward literal/closed-set call sites to enum constructors.
- Introduce explicit runtime-text construction names where dynamic text is
  still legitimate, instead of hiding those paths behind generic constructors.
- Migrate call sites in small coherent groups with local proof after each
  group.
- Keep notes of remaining runtime-text categories so later work can decide
  whether they need richer typed carriers.

## Out Of Scope

- Removing all string constructors in one step.
- Converting dynamic aggregate/vector/struct/function type spellings to a full
  typed type tree.
- Rewriting HIR `TypeSpec` lowering wholesale.
- Changing LIR verifier strictness to suppress warnings.
- Downgrading tests, expectations, or supported behavior to make migration
  appear complete.

## Acceptance Criteria

- Deprecated string construction warnings identify real remaining migration
  sites without blocking unrelated compilation prematurely.
- Closed-set uses of `LirTypeRef`, `LirBinaryOpcodeRef`, and similar wrappers
  are migrated to enum constructors where the semantic id is known at the call
  site.
- Legitimate dynamic text sources are routed through explicit runtime-text
  factories or helpers, making them searchable and auditable.
- The migration proceeds in bounded commits or packets rather than one
  repository-wide rewrite.
- The repository builds and focused LIR/frontend/backend tests pass after each
  accepted migration slice.
- Remaining warnings, if any, are documented as intentionally deferred runtime
  text boundaries rather than ignored noise.

## Initial Runtime String Inventory

A previous compile experiment found representative runtime string users:

- `src/codegen/lir/hir_to_lir/stmt.cpp`: inline asm paths construct
  `LirTypeRef(llvm_ty(...))` from `TypeSpec`-derived runtime strings.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: struct layout emits dynamic
  array padding such as `[N x i8]`.
- `src/codegen/lir/call_args.hpp`: parsed call views re-own `arg.type` text.
- `src/codegen/lir/ir.hpp`: extern declaration return types reconstruct refs
  from stored return type text.
- `src/codegen/lir/verify.cpp` and backend lowering compatibility paths:
  classification currently reparses `type.str()`.

## Reviewer Reject Signals

- The patch treats all string construction as equally wrong and deletes it
  before dynamic type sources have explicit replacements.
- Deprecation warnings are added but then globally silenced without producing
  a migration inventory or converted call sites.
- Runtime text paths remain hidden behind generic constructors after being
  classified as legitimate dynamic boundaries.
- The migration changes verifier behavior, weakens tests, or rewrites expected
  output instead of changing construction authority.
- The work mixes unrelated HIR/BIR/backend rewrites into a constructor
  migration slice.
- The accepted result still leaves closed-set literal call sites using string
  construction with no documented reason.
