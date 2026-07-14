# Current Packet

Status: Active
Source Idea Path: ideas/open/774_raw_bir_gep_function_label_address_base.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Carry the variant through Raw-BIR GEP lowering consumers

## Just Finished

- Plan Step 2: updated the Raw-BIR verifier to consume the typed GEP-base
  authority directly. Global bases retain their exact global-array contract;
  label bases must resolve to an exact current-function pointer
  `LabelAddressConstant` with a current-function target. Focused coverage now
  publishes and inspects the label-base Raw-BIR path without a global coercion.

## Suggested Next

- Execute Step 3: prove the bounded Raw-BIR boundary and prepare the exact
  handoff for 773 Step 2; do not perform printer, dispatch, or 772 work.

## Watchouts

- `GetElementPtrBase` no longer exposes a legacy global projection. Downstream
  consumers must dispatch on `authority`; no text, generic value operand, or
  synthetic global is permitted.

## Proof

- Passed `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`.
  Canonical regression logs were not touched because they remain
  supervisor-owned.
