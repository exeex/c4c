Status: Active
Source Idea Path: ideas/open/621_rv64_prepared_global_value_location_consumer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Prepared Global Value-Location Consumption

# Current Packet

## Just Finished

Step 3 implemented prepared RV64 global-memory consumption for the in-scope
prepared value-location shapes.

Implementation completed:

- `fragment_for_prepared_load_global()` now accepts complete F64 prepared
  global facts whose result value home is either a prepared FPR register or a
  validated prepared frame-slot home.
- F64 frame-slot loads materialize the global address from the prepared access,
  emit the global `fld` into a temporary FPR when no FPR home exists, then emit
  `fsd` into the prepared stack slot using the existing prepared frame-slot
  offset validator.
- Existing integer prepared-global load/store support now proves the covered
  GPR/frame-slot aggregate lane flow once the Step 2 fixture publishes the
  required `stored_value_name` for the store access. That fixture correction was
  a genuine missing producer fact, not an expectation downgrade.
- Fail-closed behavior still rejects missing prepared accesses, incomplete
  prepared global addressing, unsupported homes, and unsupported widths through
  the existing `unsupported_global_data` diagnostics.

## Suggested Next

Proceed to Step 4 validation/classification: rerun any supervisor-selected
representative row probes for `src/pr36034-1.c` and `src/pr91137.c`, keep the
guard rows classified, and decide whether the current semantic consumer slice
is enough for close-readiness or needs a narrower follow-up packet.

## Watchouts

- Do not weaken prepared/global producer authority gates.
- Do not special-case named source files, global names, value ids, or final
  assembly shapes.
- This slice does not reconstruct producer authority in RV64; it consumes the
  existing prepared memory access and prepared value-home/stack-layout facts.
- The aggregate test fixture now publishes `stored_value_name` on the store
  prepared access so the covered store has complete prepared facts.
- Keep `src/ieee/20001122-1.c` guard-only unless the width-policy diagnostic is
  separately resolved; keep `src/991030-1.c` out because it already passes.

## Proof

Proof command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed. The build completed; CTest reported
`100% tests passed, 0 tests failed out of 347`. Proof log: `test_after.log`.
