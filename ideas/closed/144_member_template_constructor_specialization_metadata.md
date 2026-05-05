# Member-Template Constructor Specialization Metadata

Status: Closed
Created: 2026-05-05
Closed: 2026-05-05

Parent Idea:
- `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`

## Goal

Give constructor calls and delegating constructor lowering a coherent
member-template specialization carrier so constructor bodies can be lowered
under both the instantiated owner record and the selected constructor
method-template bindings.

The target capability is member-template constructor specialization metadata,
including type-pack and NTTP-pack bindings. It is not a one-test repair for
piecewise pair construction.

## Why This Idea Exists

Step 6 of the TypeSpec-tag field-removal runbook repeatedly reached the same
final frontend/Sema failure family after two constructor packets. The first bad
fact improved from missing constructor lookup to missing method-template and
NTTP-pack specialization, but the family did not shrink:

- `ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_'`
  remained at 883/884 passing.
- The only remaining `^cpp_positive_sema_` failure was
  `cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`.

The latest blocked packet proved that generated specialization records can
point back to the primary `eastl::pair` constructors, but lowering the selected
constructor body still used unspecialized `Args1`/`Args2` and `I1`/`I2`.
Consequently calls such as `get<I1>(first_args)` instantiated as
`eastl::get<I=?,T=?>`, `first_args` remained `struct eastl::tuple_T_T`, and
the constructed object still deduced as `void`.

This route should be split out from TypeSpec-tag deletion because the blocker
is a separable constructor member-template specialization capability. The
parent deletion route should resume only after this carrier boundary has a
semantic repair or a smaller validated split.

## In Scope

- Inventory the constructor specialization path from parser/Sema constructor
  selection through HIR lowering.
- Establish and preserve the current 883/884 frontend positive-Sema baseline.
- Attach or recover record-member template prelude metadata for constructor
  `Node`s when the constructor originates from a member template.
- Deduce method type packs from constructor call arguments and delegating
  constructor arguments.
- Bind NTTP packs from template patterns such as `index_sequence<I...>` and
  thread those bindings into selected constructor specialization metadata.
- Lower selected constructor bodies under the instantiated owner record and
  selected method-template bindings.
- Add focused probes or frontend/HIR tests only when they prove a separable
  semantic seam and are not weaker replacements for existing coverage.
- Reintegrate the repaired capability into the parent TypeSpec-tag deletion
  validation route.

## Out Of Scope

- Reintroducing `TypeSpec::tag` or rendered-string semantic lookup.
- Broad parser, Sema, HIR, LIR, BIR, backend, or codegen rewrites unrelated to
  constructor member-template specialization metadata.
- Lowering expectation quality, marking supported constructor cases
  unsupported, or making the known piecewise test easier to pass.
- Debug-text, rendered-name, `_t`, or `tag_ctx` recovery for method-template or
  NTTP-pack identity.
- Claiming TypeSpec-tag deletion complete from this idea. This idea only
  unblocks the constructor specialization capability needed by the parent.

## Separable Seams

1. Constructor `Node` record-member template prelude carrier.
2. Method type-pack deduction from constructor and delegating constructor
   arguments.
3. NTTP-pack binding from patterns such as `index_sequence<I...>`.
4. Lowering selected constructor bodies under the instantiated owner and
   method-template specialization bindings.
5. Reintegration into the parent TypeSpec-tag deletion validation path.

## Acceptance Criteria

- The current `^cpp_positive_sema_` baseline is recorded before code changes
  and remains the comparison point for the first implementation packet.
- The constructor selection/lowering path has an explicit structured carrier
  for owner instantiation plus method-template type-pack and NTTP-pack
  bindings.
- Focused probes or tests demonstrate the separable carrier seam repaired, not
  only the named final positive-Sema fixture.
- `ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_'`
  no longer fails because selected constructor bodies lower with
  unspecialized `Args1`/`Args2` or `I1`/`I2`.
- The parent TypeSpec-tag deletion runbook can resume Step 6 validation without
  relying on rendered TypeSpec identity or constructor lookup shortcuts.

## Completion Notes

- Closed after commit `0d653f97d` implemented structured member-template
  constructor specialization from parser/HIR carriers.
- Regression guard accepted the canonical positive-Sema before/after logs:
  `test_before.log` had 883/884 passing with only
  `cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
  failing, and `test_after.log` had 884/884 passing with no new failures.
- The close proof command was:
  `cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`.
- The full-suite candidate remained red with 24 known failures and was
  rejected as uncanonical. Remaining broad validation belongs to the parent
  TypeSpec-tag deletion route or a separate follow-up idea, not this completed
  decomposition idea.

## Reviewer Reject Signals

- A change special-cases
  `cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`,
  `eastl::pair`, `eastl::tuple`, `index_sequence`, or `get` by name instead of
  repairing constructor member-template specialization metadata.
- A route claims progress while constructor bodies still lower with
  unspecialized method packs such as `Args1`/`Args2` or NTTP packs such as
  `I1`/`I2`.
- A fix recovers bindings from rendered debug text, tag spelling, `_t`
  suffixes, or `tag_ctx` instead of structured owner/member/template metadata.
- Tests are weakened, marked unsupported, or expectation-rewritten to hide the
  missing specialization carrier.
- The implementation broadens into unrelated TypeSpec-tag deletion, LIR/BIR,
  backend, or codegen rewrites without first splitting a separate open idea.
- The route retains the exact old failure mode behind a new helper or metadata
  name: constructor lookup reaches a primary body, but selected lowering still
  has no coherent method-template specialization bindings.
