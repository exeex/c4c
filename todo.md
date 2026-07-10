Status: Active
Source Idea Path: ideas/open/668_llvm_torture_20040709_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Current Failure Boundary

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: established the current first observable
failure boundary for `llvm_gcc_c_torture_src_20040709_2_c` and
`llvm_gcc_c_torture_src_20040709_3_c` in
`docs/llvm_torture_20040709_owner_discovery/01_current_failure_boundary.md`.
Both rows reproduce as runtime boundaries: clang-built binaries exit `0`,
while generated `c2ll` binaries abort after the compile pipe succeeds. Neither
row has enough current evidence for a concrete implementation owner.

## Suggested Next

Execute Step 2: draft
`docs/llvm_torture_20040709_owner_discovery/02_owner_mapping.md` by comparing
the runtime-boundary evidence for both rows against existing generated
follow-up ideas, naming an implementation owner only if row-specific evidence
proves the same first repair layer.

## Watchouts

- This is research/documentation work only.
- Do not edit implementation files, tests, expectations, unsupported markers,
  allowlists, runtime behavior, baseline acceptance, or lifecycle history.
- Do not assign either LLVM torture row to an existing backend owner without
  focused row-specific evidence.
- Historical notes mention older possible owner routes, but the current Step 1
  evidence only proves a generated-program runtime abort. Treat those older
  notes as comparison material for Step 2, not as current ownership proof.

## Proof

Ran the supervisor-selected proof command:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_20040709_2_c|llvm_gcc_c_torture_src_20040709_3_c)$') > test_after.log 2>&1
```

Result: command exited `8`; build was up to date; both focused tests failed
with `[RUNTIME_FAIL]`, `clang_exit=0`, and `c2ll_exit=Subprocess aborted`.
This is sufficient proof for the documentation slice because the packet goal
was to establish and record the current first observable boundary, not to make
the rows pass. Proof log: `test_after.log`.
