# Prealloc Call Plan Phase Split

## Goal

Split `call_plans.cpp` along durable call-plan subphases while preserving the
current call ABI contract and backend behavior.

## Why This Exists

The responsibility map identifies call and return ABI planning as a durable
prealloc family, but `call_plans.cpp` currently carries several distinct
planning concerns. Isolating those concerns should make later target work safer
without prematurely splitting `calls.hpp`.

## Target Files

- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/call_plans.hpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/formal_publications.cpp`
- `src/backend/prealloc/formal_publications.hpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`

## Slice Type

`.cpp` phase split with optional prepared-printer grouping alignment.

## Durable Owner

Call and return ABI plans.

## In Scope

- Extract direct and indirect call argument planning helpers if they form a
  stable subphase.
- Separate preservation, clobber derivation, boundary effects, and
  memory-return handling when the extracted APIs remain clear.
- Keep `calls.hpp` as the aggregate public contract unless the extraction
  proves a smaller independently consumed contract.
- Align `prepared_printer/calls.cpp` grouping or labels only when the new
  implementation structure makes the call subcontracts clearer.

## Out Of Scope

- Changing ABI rules, argument/result placement, clobber sets, preservation
  behavior, or memory-return behavior.
- Moving ABI policy away from `TargetProfile`, target register profile facts,
  or existing call contracts.
- Splitting `calls.hpp` into many tiny target-consumed headers before usage
  proves that boundary.
- Rewriting tests or dump expectations to mask behavior changes.

## Expected Behavior-Preservation Proof

Run:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

Also run `git diff --check`.

## Acceptance Criteria

- `call_plans.cpp` is smaller or better structured by durable subphase.
- Extracted helpers have clear inputs and do not require broad private mutable
  state leakage.
- Backend tests remain green with no intentional dump meaning changes.

## Reviewer Reject Signals

- Reject line-count-only extraction where helper boundaries do not match call
  planning responsibilities.
- Reject any behavior change to ABI policy, clobbering, preservation, indirect
  callees, or memory returns under a layout label.
- Reject target-shaped shortcuts or named-case handling added to make one call
  testcase pass.
- Reject splitting `calls.hpp` so targets must include many tiny headers to
  build one logical call plan.
- Reject printer label changes that drop fields or alter printed meaning.
