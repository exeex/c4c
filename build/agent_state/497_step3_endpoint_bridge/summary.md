# 497 Step 3 - Endpoint Bridge Implementation

## Decision

Step 3 is implemented as a lower-owned bridge record:
`bir::LocalArrayEndpointBridgeRecord`.

The bridge is keyed by the existing dynamic local-array
`lir_producer_lookup_key` plus producer row identity. It binds each supported
row to one ordered endpoint using this route:

- prefer a unique prepared frame-slot address materialization whose function,
  result value, source object, and ordered block/instruction coordinate match
  the producer row;
- otherwise accept a unique ordered BIR result endpoint for the same producer
  result as the equivalent BIR address-derivation endpoint.

The BIR fallback scans block/instruction order by result identity. It does not
use `lir_producer_instruction_index` as the endpoint coordinate.

## Status Surface

The bridge distinguishes:

- `missing_producer_row`
- `duplicate_producer_row`
- `invalid_producer_coordinate`
- `coordinate_confusion`
- `missing_prepared_bir_endpoint_bridge`
- `duplicate_endpoint`
- `mismatched_function`
- `mismatched_result_value`
- `mismatched_source_object`
- `mismatched_derivation_result`
- `mismatched_dynamic_index`
- `unsupported_operation_role`
- `missing_endpoint_order`

## Tests

Focused synthetic BIR tests cover the available bridge record plus missing,
duplicate, mismatched result/source/derivation/dynamic-index, coordinate
confusion, unsupported role, invalid coordinate, and missing-order cases.

Prepared stack-layout tests prove the real dynamic local-array fixture
publishes a unique ordered `bir_address_derivation` endpoint and fails closed
for missing endpoint, duplicate endpoint, coordinate-only prepared candidate,
and missing dynamic index identity.

## Non-Goals Preserved

This slice does not implement the Step 4 bounded effect scan and does not
publish available same-value/no-clobber interval facts.
