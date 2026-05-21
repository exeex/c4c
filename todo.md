Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residuals by Semantic Owner

# Current Packet

## Just Finished

Step 2 - Classify Residuals by Semantic Owner: classified the 26 non-passing
Step 1 backend-regex residuals from `test_after.log` by first bad boundary or
actionable semantic/backend owner, without implementation or expectation
changes.

Classification source:
`test_after.log`, generated semantic-BIR route dumps under `build/backend_route/`,
generated AArch64 assembly under `build/c_testsuite_aarch64_backend/src/`, and
the corresponding c-testsuite source files under
`tests/c/external/c-testsuite/src/`.

Non-passing total remains 26:
- failed: 24
- timed out: 2

Owner bucket: semantic-BIR local/pointer memory observation surface, 7 failures.

These fail before runtime as `BACKEND_ROUTE_SNIPPET_MISSING`, so the first bad
boundary is the semantic-BIR route dump/observation surface rather than AArch64
or x86 machine code.

- `backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir`
  - Evidence: expected `bir.store_local %lv.p, ptr %t0`; actual dump stores
    `ptr @.str0` after `@.str0 = bir.load_local ptr %lv.p, addr .str0`.
  - Actionable owner: semantic-BIR string-literal address materialization and
    pointer local canonicalization.
- `backend_codegen_route_x86_64_nested_pointer_param_dynamic_member_array_load_observe_semantic_bir`
  - Evidence: expected an observed element load from `addr %t1+4`; actual dump
    computes `%t4.byte_offset = %i * 4`, adds static `4`, then loads from the
    computed pointer `%t4`.
  - Actionable owner: semantic-BIR dynamic member-array load canonicalization
    through pointer parameters.
- `backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`
- `backend_codegen_route_x86_64_packed_local_member_offsets_observe_semantic_bir`
  - Evidence: each expected an element-lane `bir.load_local`/observed local
    member access, while actual dumps compute a byte offset and load/store
    through the computed pointer. The packed case computes from `%lv.p.1`;
    direct local cases compute from `%lv.p.0` or `%lv.p.4`.
  - Actionable owner: semantic-BIR dynamic indexed local aggregate/member lane
    materialization, including packed-member offsets.

Rejected adjacent owners for this bucket:
- Reject runner/CTest ownership: failures are explicit snippet mismatches with
  printed actual dumps, not harness ambiguity.
- Reject target-machine backend ownership for Step 3 selection: these fail at
  semantic-BIR observation before target machine lowering.
- Reject filename/pass-count routing: the grouping uses the missing snippets
  and actual semantic-BIR shape, not the test-name pattern alone.

Owner bucket: AArch64 scalar expression value materialization, 5 failures.

- `c_testsuite_aarch64_backend_src_00112_c`
  - Evidence: source is `return "abc" == (void *)0;`; failure is
    `RUNTIME_NONZERO exit=96`; generated assembly for `main` only returns,
    with no visible compare-to-null result materialization.
  - Actionable owner: pointer equality expression lowering/result
    materialization.
- `c_testsuite_aarch64_backend_src_00119_c`
- `c_testsuite_aarch64_backend_src_00123_c`
  - Evidence: both source files return `x < 1` for a global `double x = 100`;
    failures are nonzero exits (`16` and `144`), and generated assembly loads
    `d13` from `x` then returns without visible floating comparison/result
    materialization.
  - Actionable owner: floating comparison result lowering.
- `c_testsuite_aarch64_backend_src_00174_c`
  - Evidence: float/double arithmetic and comparisons print zeros or garbage
    for compiler-computed expressions, while `sin(2)` prints the expected
    runtime-library result; generated assembly lacks the expected FP arithmetic
    sequence for those constants/operations.
  - Actionable owner: FP scalar expression lowering and FP vararg argument
    materialization.
- `c_testsuite_aarch64_backend_src_00183_c`
  - Evidence: source uses `(Count < 5) ? (Count*Count) : (Count * 3)`;
    expected `0 1 4 9 16 15...`, actual prints `0 1 2 3 4 5...`.
  - Actionable owner: conditional operator/select result materialization.

Rejected adjacent owners for this bucket:
- Reject libc/printf as first owner for `00174`: the `sin(2)` call prints
  correctly and the bad values are compiler-computed FP expressions.
- Reject runtime/exit-code harness as first owner for `00112`, `00119`, and
  `00123`: the source expressions are tiny and generated assembly lacks the
  visible compare/result path.
- Reject grouping by numeric test id: the common evidence is missing scalar
  expression result materialization.

Owner bucket: AArch64 aggregate, array, and member address materialization, 5
failures.

- `c_testsuite_aarch64_backend_src_00157_c`
  - Evidence: local `int Array[10]` indexed in loops segfaults; generated
    assembly shows dynamic indexed stack addressing through temporary slots,
    including loads from stack address temporaries before use.
  - Actionable owner: local array dynamic-index address materialization.
- `c_testsuite_aarch64_backend_src_00163_c`
  - Evidence: after `b = &(bolshevic.b)`, expected `bolshevic.b = 34`; actual
    prints `bolshevic.b = 42`, matching the earlier `b = &a` value.
  - Actionable owner: address-of global aggregate member and pointer local
    overwrite/materialization.
- `c_testsuite_aarch64_backend_src_00176_c`
  - Evidence: quicksort leaves the global array in a wrong order; generated
    `swap` materializes many global array lanes and select labels for dynamic
    indexes.
  - Actionable owner: dynamic indexed global array load/store writeback.
- `c_testsuite_aarch64_backend_src_00185_c`
  - Evidence: local arrays with initializer lists segfault; generated assembly
    repeatedly uses stack address temporaries while filling arrays.
  - Actionable owner: local array initializer storage/address materialization.
- `c_testsuite_aarch64_backend_src_00205_c`
  - Evidence: global array of structs initialized from large constant
    expressions prints no actual case rows in the mismatch block; generated
    assembly expands fixed loads from `cases+offset`.
  - Actionable owner: global aggregate initializer layout and aggregate array
    traversal/materialization.

Rejected adjacent owners for this bucket:
- Reject expectation downgrade/unsupported classification: these are supported
  runtime tests with concrete wrong values or segfaults.
- Reject treating all as generic "arrays by filename": each case has address
  materialization evidence from source plus runtime output or generated
  assembly.
- Reject local semantic-BIR route owner for the external cases until Step 3
  proves an earlier shared boundary; current evidence reaches generated
  AArch64 assembly/runtime.

Owner bucket: AArch64 switch/fallthrough and pointer-postincrement control-flow
lowering, 2 failures.

- `c_testsuite_aarch64_backend_src_00143_c`
  - Evidence: Duff's-device switch/fallthrough copy loop with `*to++ =
    *from++` segfaults before output.
  - Actionable owner: switch fallthrough plus pointer post-increment lowering.
- `c_testsuite_aarch64_backend_src_00182_c`
  - Evidence: LED printer uses multiple switch helpers and pointer-increment
    buffer writes; expected digits for `1234567`, actual renders seven zeros.
    Generated assembly shows large switch-expanded helper functions and static
    digit-array materialization.
  - Actionable owner: switch dispatch/fallthrough interaction with buffer
    writes and digit array materialization.

Rejected adjacent owners for this bucket:
- Reject grouping by "text output mismatch" alone: both cases share switch
  control flow and post-increment/write evidence in source/generated assembly.
- Reject libc/printf as first owner: both tests build their output buffers or
  copied arrays before printing.

Owner bucket: AArch64 ABI call-boundary preparation for aggregates, variadics,
HFA, and long double/f128, 2 failures.

- `c_testsuite_aarch64_backend_src_00140_c`
  - Evidence: source passes `struct foo` by value with `struct foo *`, integer,
    and variadic arguments; runtime segfaults.
  - Actionable owner: aggregate by-value argument and mixed variadic call
    boundary lowering.
- `c_testsuite_aarch64_backend_src_00204_c`
  - Evidence: frontend/assembly route fails before runtime with
    `deferred_unsupported: call-boundary move node requires prepared GPR
    registers, scalar FPR registers, or structured f128 q-register authority`.
    Source is an AArch64 calling-convention stress test with small structs,
    HFA float/double/long-double aggregates, returns, and variadics.
  - Actionable owner: prepared AArch64 call-boundary move authority for
    aggregate/HFA/f128 paths.

Rejected adjacent owners for this bucket:
- Reject test expectation ownership: `00204` reports a concrete backend
  deferred-unsupported diagnostic.
- Reject simple scalar-expression owner: both cases are dominated by aggregate
  call-boundary mechanics, not a standalone scalar operation.

Owner bucket: external libc call result/argument materialization, 1 failure.

- `c_testsuite_aarch64_backend_src_00187_c`
  - Evidence: file I/O test writes and later prints file contents, but the
    `fread(... ) != 6` check incorrectly reports `couldn't read fred.txt`.
    Generated assembly calls `fread`, then compares against a stored temporary.
  - Actionable owner: external call return-value capture/result comparison for
    libc calls with pointer/size arguments.

Rejected adjacent owners for this bucket:
- Reject filesystem/environment ownership: subsequent `fgetc`, `getc`, and
  `fgets` output shows the file contents are present.
- Reject broad libc incompatibility: the same test successfully exercises other
  file APIs after the bad `fread` check.

Owner bucket: C initializer semantics for compound/empty/anonymous/flexible and
relocated aggregates, 1 failure.

- `c_testsuite_aarch64_backend_src_00216_c`
  - Evidence: source is an initializer stress test covering empty structs,
    compound literals, nested aggregates, flexible arrays, anonymous unions,
    function-pointer initializers, range designators, and local aggregate
    copies; runtime segfaults.
  - Actionable owner: initializer lowering and aggregate relocation/copy
    materialization before AArch64 runtime.

Rejected adjacent owners for this bucket:
- Reject choosing a narrower filename-based owner now: the source combines many
  initializer features and the log only gives a segfault, so Step 3 needs a
  focused first-bad probe before choosing a narrower repair.
- Reject timeout handling: this is a normal failed runtime case, not one of the
  timeout residuals.

Owner bucket: unsigned enum bitfield load/extension semantics, 1 failure.

- `c_testsuite_aarch64_backend_src_00218_c`
  - Evidence: source assigns enum value `AMBIG_CONV` through an 8-bit enum
    bitfield and expects no output; actual prints `unsigned enum bit-fields
    broken`. Generated assembly loads the field from offset `#16` as a full
    word path before comparing with `152`.
  - Actionable owner: unsigned enum bitfield store/load width and zero-extension
    semantics.

Rejected adjacent owners for this bucket:
- Reject generic switch owner: the switch is only the observer; the source
  comment and actual output point at unsigned enum bitfield extension.
- Reject ABI owner: no call-boundary evidence is needed to explain the wrong
  branch.

Timeout bucket, kept separate from owner selection, 2 residuals.

- `c_testsuite_aarch64_backend_src_00200_c` (Timeout)
  - Evidence: source is the left-shift type/promotion stress test; generated
    assembly is a large repeated `check` expansion. The log has no mismatch
    payload, only timeout.
  - Classification blocker: timeout does not identify whether the first bad
    boundary is shift promotion, loop/control flow, or call/check result
    materialization.
- `c_testsuite_aarch64_backend_src_00207_c` (Timeout)
  - Evidence: source combines VLA/goto, jump into a block with non-VLA arrays,
    and short-circuit/conditional side effects. The log has no mismatch
    payload, only timeout.
  - Classification blocker: timeout does not distinguish VLA/goto control flow
    from short-circuit expression lowering.

Rejected adjacent owners for timeout bucket:
- Reject assigning these to any repair owner from filename, source theme, or
  pass-count movement alone.
- Keep both out of the Step 3 owner-selection pool until a focused timeout
  probe produces first-bad evidence.

## Suggested Next

Step 3 should select exactly one non-timeout owner bucket for a bounded repair
packet, using the classifications above as input. This packet intentionally did
not choose that owner.

## Watchouts

Do not treat the local backend-route snippet failures as runtime backend
failures; they fail at semantic-BIR observation. Do not merge timeout cases into
an owner based on source themes alone. Do not claim progress through
expectation rewrites, unsupported classifications, runner changes, or
pass-count movement.

## Proof

Ran:
`test -s test_after.log && rg '^The following tests FAILED:|backend_codegen_route_|c_testsuite_aarch64_backend_src_' test_after.log >/dev/null`

Result: command succeeded against existing `test_after.log`. The proof checks
that the Step 1 log exists and still contains the backend-route and
`c_testsuite_aarch64_backend_*` residual surface used for this classification.
