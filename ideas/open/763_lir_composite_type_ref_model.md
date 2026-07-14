# LIR composite type ref model

## Intent

Add a composite `LirTypeRef` model so non-builtin LIR types can be carried as
structured type nodes instead of runtime strings, while still rendering to LLVM
type text at emission time.

This idea depends on:

- `ideas/open/759_lir_typed_ref_enum_foundation.md`
- `ideas/open/760_lir_string_constructor_deprecation_migration.md`

It should run before the call/signature and module-level string-shadow cleanup
ideas, because those later ideas need a type carrier that can represent more
than closed builtin scalars.

## Why This Exists

The current enum foundation covers closed builtin spellings such as `i32`,
`ptr`, and floating scalar types. Commit
`a1d6fd79a6be9670ee3bcc7b55230286e1adeeca` showed the remaining gap clearly:
va_list builtin fields can use `LirBuiltinType`, but struct padding and storage
fields such as `[N x i8]` must still be marked as `LirTypeRef::runtime_text`.

That is a useful transition marker, but it is not the final model. LIR should
be able to carry composite types as structured data:

- array types such as `[N x T]`
- vector types such as `<N x T>`
- aggregate literal types such as `{ T, U }` and packed `<{ T, U }>`
- named struct or union references backed by `StructNameId`
- function type fragments used by call/signature surfaces

The implementation attitude is: if LIR can represent the type, it should store
the type as structured ids/enums/nodes, not as a string. Rendering to string is
allowed at the printer/emission boundary.

## In Scope

- Extend `src/codegen/lir/types.hpp` so `LirTypeRef` can carry composite type
  structure in addition to builtin enum ids and existing struct-name ids.
- Add constructors/factories for common composite forms, starting with array
  and named struct/union references needed by struct layout and va_list-like
  records.
- Design the model so vector, aggregate literal, packed aggregate, and function
  type forms can be added without returning to string-primary storage.
- Provide an emission/render helper that converts structured `LirTypeRef`
  values to LLVM type text only at output boundaries.
- Keep `runtime_text` available only as an explicit compatibility escape hatch
  for forms not yet represented by the composite model.
- Migrate representative struct-layout fields such as padding arrays
  `[N x i8]` away from `runtime_text` and into structured composite refs.
- Add focused coverage proving composite refs compare/query/render from
  structure, not from ad hoc string parsing.

## Out Of Scope

- Removing `runtime_text` in this idea.
- Migrating every call/signature/global/module type shadow in one step; ideas
  761 and 762 own those later field-level convergence routes after this model
  exists.
- Rewriting HIR `TypeSpec` lowering wholesale.
- Replacing all aggregate/vector/function type producers at once.
- Changing LLVM IR output text, ABI layout, struct layout, verifier strictness,
  BIR semantics, target lowering, or MIR behavior except where needed to carry
  the same type through structured refs.
- Parsing rendered LLVM type strings as the primary way to reconstruct
  composite type structure.

## Acceptance Criteria

- `LirTypeRef` can represent at least builtin, integer-width, named
  struct/union, and array composite types without string-primary storage.
- `[N x i8]` and similar struct-layout padding/storage fields can be built with
  structured composite factories rather than `runtime_text`.
- Structured composite refs render to the same LLVM type text at the emission
  boundary.
- Type kind, builtin id, integer width, struct name id, array element type, and
  array length are queryable without reparsing `str()`.
- `runtime_text` remains explicitly available for deferred composite forms, but
  new supported forms do not use it.
- Focused tests cover structured construction, equality/query behavior,
  rendering, and at least one migrated struct-layout path.
- The repository builds and focused LIR/frontend/backend tests for struct type
  declarations and LIR type refs pass.

## Suggested Starting Points

- `src/codegen/lir/types.hpp`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/print.cpp`
- `src/codegen/lir/verify.cpp`
- frontend tests around LIR struct declarations and global/type refs
- backend tests that consume `LirStructDecl` and `LirStructField`

## Follow-Up Ordering

After this idea is accepted, the planned follow-up order is:

1. `ideas/open/761_lir_call_signature_type_mirror_convergence.md`
2. `ideas/open/762_lir_module_declaration_type_shadow_convergence.md`

Those ideas should be able to use the composite carrier from this work instead
of classifying array, aggregate, vector, or function types as unavoidable
runtime text.

## Reviewer Reject Signals

- The patch merely renames runtime strings or wraps rendered text without
  storing composite type structure.
- Supported composite forms still require parsing `str()` for semantic queries
  such as kind, array length, element type, or struct identity.
- The implementation removes `runtime_text` before all current dynamic forms
  have structured replacements.
- Rendering behavior changes LLVM output or ABI layout for existing programs.
- The patch broadens into call/signature/module shadow cleanup that belongs to
  ideas 761 or 762 instead of first providing the composite type model.
- Tests are weakened, expectations are downgraded, or verifier rules are
  relaxed to hide remaining string-primary composite type paths.
