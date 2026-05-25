Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove The Selected Publication Decision

# Current Packet

## Just Finished

Step 2 removed the selected aggregate-address publication decision from the
AArch64 dispatch bridge.

- Added `PreparedCallArgumentPlan::allows_local_aggregate_address_publication`
  as the per-argument prepared authority for this gate.
- Populated the fact while building prepared call plans from the existing call
  semantic inputs: non-`llvm.*`, pointer-typed publication argument, and not a
  byval copy.
- Replaced the dispatch-bridge `CallInst::arg_abi`/`arg_types` decision with a
  prepared-plan lookup that reads only the prepared argument fact.
- Updated the two positive manual MIR fixtures to publish the prepared
  authority explicitly; the byval and `llvm.va_start` guards keep the default
  false fact.

## Suggested Next

Delegate the next Step 2 cleanup packet, if desired, to tighten any remaining
manual prepared-call test fixtures or prepared debug output around the new
argument fact. The selected dispatch decision itself has been removed.

## Watchouts

- `lower_scalar_call_argument_producers` now finds the prepared call plan from
  the existing block context and instruction index, keeping the public helper
  signature unchanged because `dispatch.cpp` was outside this packet's owned
  files.
- The fact is intentionally semantic. Do not replace it later with inference
  from source/destination placement fields.

## Proof

Proof passed:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

`test_after.log` records `100% tests passed, 0 tests failed out of 162`.
