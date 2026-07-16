# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Preserve recursive aggregate facts and all aggregate forms

## Just Finished

- Lifecycle resumed 838 after closed 852 satisfied the recorded HIR producer
  return condition. Prior 838 progress is preserved: Step 1 accepted in
  `69ffa299f` and `a50d35d4e`; Step 2 declaration/store fact capture accepted
  in `8eca000c9`.

## Suggested Next

- Continue Step 2 with one bounded migration: update the
  `lir_owned_type_spec` function-signature occurrence producer to consume
  populated `QualType::aggregate_ref` through the existing HIR-ref-to-LIR-ref
  intern relation.

## Watchouts

- Do not add owner-key, tag, parser-pointer, `record_def`, rendered-text,
  runtime-string, or `Node*` reconstruction. Do not widen into unrelated
  consumer, verifier/printer, 836, or 831 work.

## Proof

- Lifecycle slice only; no validation run. Next code-bearing packet should run
  a fresh build plus focused proof for the function-signature aggregate-ref
  lowering path and preserve the accepted 838 backend proof boundary.
