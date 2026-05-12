# Current Packet

Status: Active
Source Idea Path: ideas/open/176_lir_type_ref_structured_equality.md
Source Plan Path: plan.md
Current Step ID: 2/3
Current Step Title: Add Structured Equality Probe Coverage and Compare Structured Payload for selected `LirTypeRef::operator==`

## Just Finished

Step 2/3 added focused `LirTypeRef` equality collision probes across the
function signature, call, extern declaration, and global type-ref tests, then
updated `c4c::codegen::lir::LirTypeRef::operator==` so two metadata-rich struct
refs compare by `StructNameId` when both sides carry one. The probes assert
that identical rendered `%struct.Pair` text with different `StructNameId`
payloads is not equal, while legacy no-id `LirTypeRef("%struct.Pair")` values
still compare by rendered text.

## Suggested Next

Supervisor should review the completed Step 2/3 slice for acceptance and decide
whether the active plan has another implementation packet or is ready for
plan-owner lifecycle handling.

## Watchouts

- The equality path does not synthesize ids from rendered text.
- Mixed id/no-id comparisons intentionally keep the legacy rendered-text path.
- No printer text, template record-owner implementation, backend lowering, or
  verifier expectation downgrade was touched.

## Proof

Ran the delegated proof command successfully:

```sh
cmake --build build --target frontend_lir_function_signature_type_ref_test frontend_lir_call_type_ref_test frontend_lir_extern_decl_type_ref_test frontend_lir_global_type_ref_test && ctest --test-dir build -R 'frontend_lir_(function_signature|call|extern_decl|global)_type_ref' --output-on-failure > test_after.log
```

Proof log: `test_after.log`
