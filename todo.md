Status: Active
Source Idea Path: ideas/open/623_rv64_cast_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement one verified cast consumer sub-family

# Current Packet

## Just Finished

Step 3 implemented semantic RV64/MIR object-route consumption for the
width-preserving i32-to-i32 CastInst GPR-copy family:
`rv64-consumer:width-preserving-zext-i32-to-i32` and the naturally covered
`rv64-consumer:width-preserving-trunc-i32-to-i32`.

The object emission route now recognizes prepared register-home
`zext i32 -> i32` and `trunc i32 -> i32` CastInst rows as a width-preserving
GPR copy before falling through to the existing generic cast helper. The same
consumer is used for normal instruction emission and select-edge dependency
producer emission. Non-i32, non-zext/trunc, missing-home, and stack-result
shapes continue to reject through the existing unsupported-instruction
diagnostic path.

Focused backend coverage now proves both accepted rows encode the same
prepared GPR copy and keeps nearby rejected shapes closed: `sext`, pointer
same-width casts, stack-source parameter rejection, and stack-result zext/trunc
guards. No unsupported markers, allowlists, BIR producer/lowering files,
runtime/accounting files, or ptrtoint behavior were changed.

## Suggested Next

Execute Step 4 breadth and guardrails: rerun the refreshed cast diagnostic
subset from Step 1, compare against the Step 2 owner buckets, include the nearby
non-cast guard rows, and record the remaining cast residuals by owner after the
width-preserving i32 GPR-copy consumer is active.

## Watchouts

- Residual cast buckets expected for Step 4: 4 `ptrtoint-*-ptr-to-i32` RV64
  consumer rows remain deliberately out of this packet, and the 2
  `sitofp -> f128` policy/floating rows remain out of the ordinary GPR cast
  consumer route.
- The nearby 60 non-cast guard rows remain guards only; they are not acceptance
  evidence for this route.
- If Step 4 finds remaining width-preserving i32 zext/trunc residual rows,
  inspect producer/home facts before widening the object consumer. Do not patch
  expectations, unsupported markers, allowlists, or named testcase shapes.

## Proof

Supervisor-selected proof command:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1
```

Result: passed. `test_after.log` is the canonical proof artifact; the backend
CTest subset completed successfully after the Step 3 code and test updates.
