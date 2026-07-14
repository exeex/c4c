# Current Packet

Status: Active
Source Idea Path: ideas/open/774_raw_bir_gep_function_label_address_base.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Specify the structured Raw-BIR GEP-base variant

## Just Finished

- Plan Step 1: introduced the explicit Raw-BIR GEP-base authority variant for
  the legacy global object or an exact current-function `LabelAddressConstant`.
  The public builder preserves global-array validation and rejects missing,
  foreign, undefined, and non-label identities before staging state; focused
  tests cover both alternatives.

## Suggested Next

- Execute Step 2: carry the structured GEP-base authority through Raw-BIR GEP
  lowering consumers, preserving global-array behavior and without text
  recovery, synthetic values, or global coercion.

## Watchouts

- The function-owned direct label address remains a dedicated base alternative
  carrying its exact ValueId; current global-only consumers still receive an
  invalid global projection for that alternative and must be updated in Step 2.

## Proof

- Passed `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`.
  Canonical regression logs were not touched because they remain
  supervisor-owned.
