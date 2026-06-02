Status: Active
Source Idea Path: ideas/open/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Printing Through the Explicit Shape

# Current Packet

## Just Finished

Step 4 routed aggregate register-lane printing through the explicit
`AggregateRegisterLanePublicationView` shape. The local aggregate-lane printer
helper now consumes the view directly, and `print_call_boundary_move` computes
`aggregate_register_lane_publication_view(move)` before calling it.

The existing early missing-provenance/source/destination checks still run before
aggregate-lane view selection. Printer-side unsupported fallbacks for scratch
selection, lane destination validation, memory addressing, and chunk
printability remain unchanged, and ordinary register/immediate/f128/frame-slot
move printing is still routed through the existing branches.

## Suggested Next

Step 5 should validate the aggregate-lane schema cleanup surface and acceptance
criteria, including confirming that construction and printing now consume the
explicit view at their boundaries without expectation rewrites or unrelated
record-schema changes.

## Watchouts

- The printer helper now assumes the explicit view already identified the
  aggregate-lane publication shape; keep any broader shape validation in the
  view boundary instead of duplicating it in the helper.
- Printer-side validation remains intentionally separate after the view
  succeeds: scratch availability, occupied-lane destination selection, memory
  addressing, and chunk printability can still reject a selected shape.
- Step 5 should watch for accidental behavior changes in ordinary
  register/immediate/f128/frame-slot call-boundary moves while reviewing the
  acceptance surface.

## Proof

Step 4 proof passed and wrote CTest output to `test_after.log`.

```sh
cmake --build build --target backend_aarch64_machine_printer_test backend_aarch64_target_instruction_records_test backend_aarch64_call_boundary_owner_test && ctest --test-dir build -R '^(backend_aarch64_(machine_printer|target_instruction_records|call_boundary_owner))$' --output-on-failure > test_after.log
```

CTest subset result: 3/3 passed.
