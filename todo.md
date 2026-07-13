# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 visibility-free weak initialized-definition packet added one
  typed `GlobalObject::is_weak` fact and threaded it explicitly through the
  builder, importer, automatic object view, and foundation verifier. Only exact
  producer-authored non-extern, non-internal `weak ` definitions with nonempty
  opaque initializers and coherent ordinary/constant qualifiers are admitted.
- Nearby generic coverage proves weak ordinary and link-backed weak constant
  definitions retain their distinction from ordinary/internal rows, exact typed
  type/const/linkage facts, identity, opaque payload, and ordered initializer
  links. Decorated and `extern_weak` neighbors reject transactionally; direct
  builder proof accepts a coherent weak definition and rejects weak+internal or
  weak+extern typed combinations at publication.

## Suggested Next

- Execute one bounded Step 3 const-pointer initialized-definition packet.
  Follow the producer's structured pointer-type authority where `is_const=true`
  still uses `global ` rather than `constant `, prove exact pointer type and
  opaque initializer receipt, and leave aggregates and flexible special types
  out.

## Watchouts

- `LirGlobal` has no separate weak flag: exact `linkage_vis == "weak "` is the
  producer-authored LIR authority mapped to Raw-BIR `is_weak=true`. Raw-BIR does
  not store compatibility linkage text, and the verifier deliberately rejects
  typed weak+internal and weak+extern combinations. Visibility-decorated weak,
  `extern_weak`, aggregates, and flexible special types remain fail-closed.
- `init_text` is opaque receipt evidence, not parsed semantic or topology
  authority. `initializer_function_link_name_ids` alone supplies structured
  initializer references, and `LirGlobal.id` remains producer-default
  compatibility state rather than Raw-BIR identity.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records 4/4 backend tests
  passing. The selected proof covers the shared typed schema/builder/verifier
  seam plus importer admission and nearby interface coverage.
