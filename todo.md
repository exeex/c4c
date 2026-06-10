Status: Active
Source Idea Path: ideas/open/150_edge_publication_lookup_declaration_owner.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit Remaining Facade Includes

# Current Packet

## Just Finished

Completed plan Step 4 for idea 150, Audit Remaining Facade Includes. Re-ran
`rg -n '#include .*prepared_lookups\.hpp' src/backend` and classified every
remaining direct include:

- `src/backend/prealloc/prepared_lookups.cpp`: owning implementation use. This
  file defines the aggregate lookup builders and query helpers declared by
  `prepared_lookups.hpp`, including `PreparedFunctionLookups` construction and
  the remaining return-chain helpers.
- `src/backend/mir/x86/x86.hpp`: aggregate facade use. `ConsumedPlans` stores
  `std::optional<prepare::PreparedFunctionLookups>`, exposes shared call-plan
  lookups through that aggregate, and builds the aggregate with
  `make_prepared_function_lookups`.
- `src/backend/mir/riscv/codegen/emit.hpp`: architecture-specific prepared
  lookup facade use. The public RISC-V edge-publication helpers accept a
  `PreparedFunctionLookups*` and expose `PreparedEdgePublication`-based intent
  data.
- `src/backend/mir/aarch64/module/module.hpp`: aggregate facade use. The
  AArch64 lowering context carries `PreparedFunctionLookups*` plus pointers to
  its aggregate sub-lookups (`PreparedCallPlanLookups`,
  `PreparedAddressMaterializationLookups`, `PreparedMoveBundleLookups`, and
  `PreparedValueHomeLookups`).
- `src/backend/mir/aarch64/codegen/alu.cpp`: architecture-specific prepared
  lookup facade use. Scalar lowering reads architecture-specific facts through
  `context.function.prepared_lookups` sub-lookups, and one local fallback still
  materializes `PreparedFunctionLookups` with `make_prepared_function_lookups`.

Confirmed `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` is no
longer a remaining direct `prepared_lookups.hpp` include. It includes
`publication_plans.hpp` directly, and its
`make_prepared_edge_publication_lookups` call sites resolve through that narrow
declaration owner rather than depending on the old facade by accident.

## Suggested Next

Execute plan Step 5 as the final proof packet: run the required build and
backend CTest, then record the proof result. Do not start another include split
inside this runbook.

## Watchouts

No separately recorded blocker was found during the audit. Keep
`prepared_lookups.cpp` as the owning implementation unless a later lifecycle
packet explicitly moves definitions. `dispatch_producers.cpp` intentionally
keeps `select_chain_lookups.hpp`; do not treat that as part of this facade
include audit.

## Proof

Inspection-only proof; no build or tests were required and no `test_after.log`
was updated for this packet.

Commands recorded:

- `rg -n '#include .*prepared_lookups\.hpp' src/backend`
- `rg -n 'Prepared(FunctionLookups|CallPlanLookups|AddressMaterializationLookups|MoveBundleLookups|ValueHomeLookups|EdgePublication|MoveBundle)|make_prepared|find_prepared|resolve_prepared|prepared_' src/backend/prealloc/prepared_lookups.cpp`
- `rg -n 'Prepared(FunctionLookups|CallPlanLookups|AddressMaterializationLookups|MoveBundleLookups|ValueHomeLookups|EdgePublication|MoveBundle)|make_prepared|find_prepared|resolve_prepared|prepared_' src/backend/mir/x86/x86.hpp`
- `rg -n 'Prepared(FunctionLookups|CallPlanLookups|AddressMaterializationLookups|MoveBundleLookups|ValueHomeLookups|EdgePublication|MoveBundle)|make_prepared|find_prepared|resolve_prepared|prepared_' src/backend/mir/riscv/codegen/emit.hpp`
- `rg -n 'Prepared(FunctionLookups|CallPlanLookups|AddressMaterializationLookups|MoveBundleLookups|ValueHomeLookups|EdgePublication|MoveBundle)|make_prepared|find_prepared|resolve_prepared|prepared_' src/backend/mir/aarch64/module/module.hpp`
- `rg -n 'Prepared(FunctionLookups|CallPlanLookups|AddressMaterializationLookups|MoveBundleLookups|ValueHomeLookups|EdgePublication|MoveBundle)|make_prepared|find_prepared|resolve_prepared|prepared_' src/backend/mir/aarch64/codegen/alu.cpp`
- `rg -n 'prepared_lookups\.hpp|make_prepared_edge_publication_lookups|PreparedEdgePublicationLookups|PreparedFunctionLookups|PreparedCallPlanLookups|PreparedAddressMaterializationLookups|PreparedMoveBundleLookups|PreparedValueHomeLookups' src/backend/mir/aarch64/codegen/dispatch_producers.cpp src/backend/mir/aarch64/codegen src/backend/mir/aarch64/module src/backend/mir/riscv src/backend/mir/x86`
