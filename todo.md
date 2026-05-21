Status: Active
Source Idea Path: ideas/open/374_aarch64_local_aggregate_address_call_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Local Aggregate Address Publication Gap

# Current Packet

## Just Finished

Executed idea 374 Step 1 localization for local aggregate address publication
to scalar pointer call arguments. The delegated proof refreshed
`test_after.log` and reproduced both representatives:

- `00218`: `RUNTIME_MISMATCH`, expected no output, actual
  `unsigned enum bit-fields broken`.
- `00216`: `RUNTIME_NONZERO`, segmentation fault before visible output.

First bad facts:

- `00218`: prepared BIR for `main` still represents the call as
  `bir.call i32 convert_like_real(ptr %lv.convs)`, and the prepared call plan
  binds arg0 from value `%lv.convs` in `x21` to ABI `x0`. Generated AArch64
  stores `AMBIG_CONV` into the local union at `sp+2`, and the callee
  `convert_like_real` correctly loads/masks `[x0,#16]` with `255` and compares
  to `152`. The first bad instruction is the callsite publication:
  `mov x0, x21; bl convert_like_real`; `x21` has only its saved-register
  restore slot and was never materialized as `&convs`. Prepared addressing has
  frame-slot address materialization for `%lv.convs.0`, but the scalar pointer
  value used by the call is `%lv.convs`, so the address dies between prepared
  address/value representation and AArch64 call-argument emission.
- `00216`: inside `foo`, prepared BIR creates `%lv.ls = bir.add ptr %lv.ls.0,
  0` and then calls `print_(ptr @.str3, ptr %lv.ls, i64 4)`. The prepared call
  plan binds arg1 from `%lv.ls` in `x13` to ABI `x1`. Generated AArch64
  initializes `ls` bytes at the start of the frame, then emits the first local
  aggregate print as `adrp/add x0, .str3; mov x1, x13; mov x2, #4; bl print_`.
  No `add x13, sp, #0` or equivalent materialization precedes the call, so
  `print_` dereferences stale `x13` and crashes. Later local aggregate
  print calls mostly use stack-slot or global address encodings, but the first
  crash is the register-backed zero-offset local aggregate address.

Owning backend boundary: scalar pointer call-argument preparation/publication
for address-valued local aggregate operands. The prepared representation can
name a local aggregate address either directly (`%lv.convs`) or through a
zero-offset frame-slot pointer add (`%lv.ls = %lv.ls.0 + 0`); AArch64 lowering
must materialize that address into the planned source carrier before the ABI
argument move. This is not a bit-field mask/sign-extension failure in `00218`
and not a global/string symbol call-argument problem in `00216`.

## Suggested Next

Execute Step 2/3 as a focused implementation packet: add backend coverage and
repair local aggregate address materialization before call-argument ABI moves.
Coverage should include both shapes:

1. a direct address-of local aggregate value used as a scalar pointer call
   argument, like `callee(&local_union_or_struct)`, where the aggregate base
   is the call operand rather than a scalar local pointer variable;
2. a zero-offset computed local aggregate address value feeding a nonzero ABI
   argument, like `print_(name, (u8 *)&local_struct, sizeof local_struct)`.

The test should assert semantic call-argument publication of the local frame
address, not a named testcase, one stack offset, one register, or one emitted
instruction spelling.

## Watchouts

Do not reopen closed direct-call or address-valued publication owners from
counts alone. Do not special-case `00218`, `00216`, `convs`, `ls`, one
function, one register, one stack offset, or one emitted instruction sequence.
Keep scalar constant/`sizeof` publication (`00205`), external call-result
publication (`00187`), scalar FP (`00174`), and timeout buckets (`00200`,
`00207`) parked under idea 295 unless fresh localization proves a handoff.
`00216` is a crash guard and may advance to later compound-initializer or
function-pointer selected-table facts after the first local-address call
publication gap is repaired.

## Proof

Ran exact Step 1 localization command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00218_c|c_testsuite_aarch64_backend_src_00216_c)$' > test_after.log 2>&1
```

Result: build completed; CTest returned nonzero for the expected two
representatives. `test_after.log` records `00218` as `RUNTIME_MISMATCH` with
actual `unsigned enum bit-fields broken`, and `00216` as `RUNTIME_NONZERO`
segmentation fault. Read-only localization used generated assembly for both
representatives and prepared-BIR dumps focused on `00218` `main` and `00216`
`foo`.
