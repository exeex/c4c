Status: Active
Source Idea Path: ideas/open/102_aapcs64_va_arg_payload_shape_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Validation And Close Readiness

# Current Packet

## Just Finished

Completed `plan.md` Step 5: ran final backend validation and prepared
close-readiness notes for the AAPCS64 `va_arg` payload-shape authority plan.

### Final Validation

- Command completed:
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
- Result: passed.
- Backend CTest subset: `169/169` passed, `0` failed.
- `test_after.log` contains the full delegated validation output.

### Coverage Summary

- Scalar GP `va_arg` payload ABI:
  - BIR runtime placeholders publish `va_arg_payload_abi`.
  - Prepared scalar access plans consume the explicit payload ABI and prove
    `gp_register_save_area`, `type=i32`, `size=4`, and `align=4`.
  - Frame-stack and AArch64 instruction-record tests prove the carrier survives
    planning and selection.
- Scalar FP `va_arg` payload ABI:
  - BIR runtime placeholders publish explicit `f64` payload ABI.
  - Prepared scalar access plans prove `fp_register_save_area`, `type=f64`,
    `size=8`, and `align=8`.
  - Frame-stack and AArch64 instruction-record tests prove the carrier survives
    planning and selection.
- Non-HFA aggregate `va_arg` payload shape:
  - BIR aggregate placeholders publish explicit `va_arg_payload_abi` with
    sret-style payload size/alignment.
  - Prepared aggregate access plans prove payload size/align, destination-home
    behavior, overflow source field, copy size/align, and progression stride.
  - Backend route coverage includes the AArch64 variadic aggregate overflow
    byte-copy case.
- HFA aggregate `va_arg` lane shape:
  - BIR runtime lowering publishes `va_arg_payload_abi`,
    `va_arg_hfa_lane_count`, and `va_arg_hfa_lane_size_bytes`.
  - Prepared-printer coverage proves prealloc consumes explicit lane count and
    lane size by dumping `register_save_lanes` and
    `register_save_lane_size`.

### Retained Exceptions

- Runtime helper identity still uses the existing runtime-placeholder shape plus
  `llvm.va_arg.*` / `llvm.va_arg.aggregate` spelling. That identity issue is
  separate from this source idea and remains covered by the placeholder
  identity follow-up line, not by this payload-shape plan.
- HFA lane destination-home mapping still uses existing helper-local placement
  evidence to map destination homes. Lane count and lane size are no longer
  inferred there; they come from explicit BIR payload metadata, and mismatched
  homes fail closed as a missing aggregate access plan.

### Close Readiness

- Source idea acceptance criteria are satisfied:
  - scalar payload ABI has named BIR placeholder authority;
  - aggregate/HFA payload size, lane count, lane size, and destination-home
    behavior are carried or reconstructed through the documented narrow
    contract;
  - proof covers AAPCS64 scalar, aggregate, and HFA `va_arg` cases.
- No implementation or test files changed in this Step 5 packet.
- Ready for plan-owner close review after supervisor acceptance.

## Suggested Next

Request plan-owner close review for
`ideas/open/102_aapcs64_va_arg_payload_shape_authority.md`.

## Watchouts

- Close review should treat runtime helper identity as retained separate scope,
  not an unresolved payload-shape blocker.
- Do not reopen fixed-call HFA pressure or unrelated AArch64 ABI behavior for
  this source idea.

## Proof

Command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1
```

Result: passed. `test_after.log` contains the delegated proof output.

Summary:

- Build: `ninja: no work to do.` during the captured proof command.
- CTest subset: `169/169` backend tests passed.
- No failures.
