# LIR Global, Extern Declaration, and Initializer Family Facts

Status: Closed
Type: first-owner global/extern type fact migration
Matrix Rows: M15
Dependencies: 841, 838, 840, 839, 843; preserves 760 and 762

## Goal

Migrate global and extern type facts from parallel `TypeSpec`/text/optional-ref
mirrors to family refs while leaving initializer-text semantics separately owned.

## In Scope

- Migrate global/extern lowering, verifier, printer, and family-ref collection
  for type facts.

## Out Of Scope

- Initializer-text semantic redesign, legacy scanner deletion without its named
  producer/carrier migration, or 812/813 non-type string routing.

## Acceptance Criteria

- Fresh build plus global/extern lowering, verifier/printer, collection, and
  legacy initializer compatibility proof.
- Delete semantic `llvm_type` and extern runtime-text only after all global/
  extern consumers use family refs; retain final output and initializer scanner
  until named migration proves deletion.

## 866 Reconciliation And 734 Return

Idea 866 keeps this as the ordered producer/schema successor for global and
extern type facts. Return to 734 only after one exact typed global or extern
handoff is accepted; initializer-text semantics and non-type global policy stay
outside this return and remain delegated to separate evidence routes.

## Closure

Closed as capability complete for the bounded M15 global/extern type-fact route.
The accepted route migrated the selected declared aggregate global, extern
aggregate return, and extern fixed aggregate byval parameter forms to structured
family refs while retaining compatibility/output text where unselected forms or
fallback paths still need it.

Accepted evidence:

- `17924cbbf` and `ab59b41d5` require and render the selected aggregate global
  type facts from structured refs; `6149ed4a4` records the no-code retirement
  conclusion that `LirGlobal.llvm_type` remains only for compatibility/output
  and unselected paths.
- `553a24dd7`, `3323d25e2`, and `17c5aaf88` prove and render selected extern
  aggregate returns from structured return refs; `091e1e5c3` records the
  bounded no-code retirement conclusion for remaining return text mirrors.
- `a355ff913` and `97b6eabfd` require and render selected extern fixed
  aggregate byval parameters from signature-store refs; `bf231e5b6` records the
  bounded no-code retirement conclusion for remaining parameter text mirrors.
- Supervisor-accepted proof for the final code-bearing parameter slice was
  `{ cmake --build build && ctest --test-dir build -R '^backend_lir_to_bir_interface$|^frontend_lir_extern_decl_type_ref$|^frontend_lir_global_label_address_initializer$' --output-on-failure; } > test_after.log 2>&1`,
  with a successful build and 3/3 passing tests.

No successor is required for idea 844. Initializer-text semantics, scanner
deletion, Raw-BIR receiver work, collector-only migration, varargs policy,
nonaggregate runtime-text declaration forms, and 812/813 non-type string routing
remain explicitly outside this closed idea.

## Reviewer Reject Signals

- Reject treating final rendering or initializer text as type authority,
  deleting the scanner prematurely, testcase-only extern handling, or 762
  scope expansion.
