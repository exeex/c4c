Status: Active
Source Idea Path: ideas/open/391_rv64_variadic_prologue_save_area_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Narrow Save-Area Publication

# Current Packet

## Just Finished

Step 4: Implement Narrow Save-Area Publication is complete.

Added explicit RV64 incoming variadic GPR publication facts to the prepared
variadic entry schema. The prealloc route now sizes the RV64 overflow/save
backing slot for post-named incoming GPRs, publishes one fact per saved
argument register, removes the missing publication fact only after the table is
prepared, and prints the facts in prepared-BIR dumps.

RV64 object admission now validates the publication table before accepting a
variadic helper route, including source register, post-named GPR range,
destination slot/range, payload size, duplicate source/destination, and backing
slot shape. Object emission stores the incoming GPR payloads into the prepared
backing area in the callee prologue before `va_start` exposes the overflow
area.

Focused backend coverage now includes the accepted saved-GPR publication route
plus missing, duplicate, source-mismatched, destination-mismatched, and backing
slot bounds fail-closed shapes.

## Suggested Next

Step 5 should rerun the representative `va-arg-13.c` RV64 object route and
classify the next boundary if the runtime still aborts.

## Watchouts

- The implementation is semantic over the prepared variadic entry facts; it
  does not hard-code `va-arg-13.c`, `test`, `dummy`, literal `1234`, a specific
  incoming register, or offset 72.
- Manual object-emission tests that are meant to exercise later helper
  diagnostics use `named_gp=8` so the new publication layer is explicitly
  complete with zero post-named GPR publications.
- The accepted route deliberately uses a `va_start` destination-address
  register that is not one of the saved incoming payload registers.

## Proof

Delegated proof command:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Result: PASS, 326/326 backend tests.
- Proof log: `test_after.log`.
