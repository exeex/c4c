Status: Active
Source Idea Path: ideas/open/112_aarch64_00216_00204_post_closure_regression.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Protect The Idea-110 Recovered Target Set

# Current Packet

## Just Finished

Step 4 `Protect The Idea-110 Recovered Target Set` repaired the
`backend_aarch64_instruction_dispatch` validation fallout from the accepted
outgoing stack-reservation semantics.

The internal dispatch tests now assert the new call-boundary lowering contract:
outgoing stack argument space is reserved with `sub sp, sp, #N`, `x16` is
published from the adjusted `sp`, outgoing stores target `x16`, the call
executes, and `sp` is restored afterwards. Helper-level byval and f128 checks
also account for the explicit reservation/base-publication records while
preserving source/destination fact checks.

## Suggested Next

Supervisor review/commit of the Step 4 test-protection slice is the next
coherent packet. If more confidence is desired before commit, rerun the same
recovered-target subset or escalate to the supervisor's chosen broader AArch64
backend validation.

## Watchouts

- Do not revert the accepted outgoing stack-reservation order in
  `calls.cpp`/`machine_printer.cpp`; the protected contract is `sub sp`,
  `mov x16, sp`, stores through `x16`, `bl`, then `add sp`.
- Do not weaken, skip, or reclassify the recovered C tests
  `00172`, `00180`, `00216`, `00220`, or `00204`.
- The f128 carrier-backed stack handoff still reads from its prepared carrier
  offset while storing through the reserved outgoing stack base; that is
  distinct from generic frame-slot source offset adjustment.

## Proof

Ran delegated proof:

```sh
cmake --build --preset default && ctest --test-dir build -j1 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(172|180|216|220|204)_c)$' > test_after.log 2>&1
```

Result: build passed; focused CTest passed all selected tests:
`backend_aarch64_instruction_dispatch`,
`c_testsuite_aarch64_backend_src_00172_c`,
`c_testsuite_aarch64_backend_src_00180_c`,
`c_testsuite_aarch64_backend_src_00216_c`,
`c_testsuite_aarch64_backend_src_00220_c`, and
`c_testsuite_aarch64_backend_src_00204_c`. Proof log path: `test_after.log`.
