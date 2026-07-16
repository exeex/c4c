# LIR Nominal Function Signature and Call Composition

Status: Closed
Type: first-owner function signature schema and call/declaration migration
Matrix Rows: M8, M9
Dependencies: 838 aggregate refs; preserves 761; coordinates 795 and 829/830 without owning body-use identity

## Goal

Replace partial signature/text mirrors with a nominal function-signature store
that composes legal family refs for declarations and calls.

## In Scope

- Add `LirFunctionSignatureRef`/store for return, ordered parameters, variadic,
  and ABI facts; migrate declaration and call composition.
- Use only the later restricted value carrier at actual argument/result
  boundaries; preserve raw extern/inline-asm compatibility as named one-way
  adapters.

## Out Of Scope

- 829/830 native body-use/argument identity, unrestricted value unions, and
  scalar/vector/aggregate producer migration.

## Acceptance Criteria

- Fresh build and focused declaration/call lowering, fixed/variadic
  verification, aggregate parameter/return, raw-call compatibility, printer,
  and reference-collector proof; reject malformed signatures and wrong-module
  aggregate alternatives.
- Delete semantic `signature_text`, `args_str`, parsed-call construction, and
  duplicate `arg_type_refs` only after named declarations, calls, verifier,
  printer, and collectors consume signature/value facts.

## Reviewer Reject Signals

- Reject parser/text/signature spelling as semantic construction or a 829/830
  authority claim.
- Reject generic call rewrites, downgraded contracts, or raw compatibility that
  remains a semantic fallback after its consumer migrates.

## Closure Disposition

Closed as capability-complete for the F1 first-owner function-signature scope.
Accepted route evidence introduced `LirFunctionSignatureRef`/store authority
for declarations and direct calls, migrated fixed/variadic/raw extern/byval
call composition through stored signature facts, updated reference collection
to observe structured signature refs, and removed the migrated fixed direct-call
dependency on duplicate `arg_type_refs`.

Accepted implementation commits:

- Step 3 repair planning: `7936d5913`
- Step 3A variadic declaration admission: `56744edb4`
- Step 3B raw extern signature-store adapter: `aaad45391`
- Step 3C byval call signature-store authority: `f6dd08de0`
- Step 4 reference collection without text authority: `ca348480a`
- Step 5 migrated fixed direct-call mirror deletion gate: `c550d017c`
- Earlier accepted Step 3 call slices: `cefbf1664`, `aed6d4ad2`,
  `158cd4de8`, `bd7249d1b`

Accepted focused proof:

```text
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|frontend_lir_call_type_ref|frontend_hir_tests)$' && ctest --test-dir build -j --output-on-failure -R '^backend_lir_' ) > test_after.log 2>&1
```

Residual mirror deletion is not stranded under this closed source. Remaining
`signature_text`, `args_str`, parsed-call construction, and `arg_type_refs`
uses are compatibility or later-family gates with named open owners:

- `ideas/open/829_lir_next_body_parameter_authority_handoff.md` and
  `ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md`
  own body-use and direct-call argument identity beyond this source's
  signature-composition boundary.
- `ideas/open/842_lir_restricted_first_class_value_unions.md` owns value
  boundary carriers before remaining call argument/result mirror deletion.
- `ideas/open/845_lir_typed_reference_carriers_collector_migration.md` owns
  collector/reference-carrier replacement for exact text-scanned fields.
- `ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md` owns
  verifier/printer family-consumer contraction and generic text-classification
  deletion after all callers migrate.
- `ideas/open/813_lir_string_semantic_authority_completion_umbrella.md` and
  terminal follow-up `ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md`
  retain routing/final deletion responsibility for residual semantic string
  escape hatches after producer and consumer owners complete.
