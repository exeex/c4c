Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Byval Aggregate Slot Accounting Residual

# Current Packet

## Just Finished

Step 1 localized the fixed non-HFA byval aggregate residual for
`fa1(s8, s9, s10, s11, s12, s13)`.

The semantic BIR source bytes are still correct: `%t103` through `%t108`
load `s8` through `s13` byte-for-byte before the call. The prepared caller
plan is also mostly the first good state:

- expected and prepared caller starts:
  `%t103/s8` -> `x0`, `%t104/s9` -> `x1/x2`,
  `%t105/s10` -> `x3/x4`, `%t106/s11` -> `x5/x6`,
  `%t107/s12` -> outgoing stack `+0`, `%t108/s13` -> outgoing stack `+16`
- observed generated caller publication in `build/c_testsuite_aarch64_backend/src/00204.c.s`:
  `s8` starts at `x0`, `s9` at `x1` with tail in `x2`, `s10` at `x3`
  with tail in `x4`, `s11` at `x5` with tail in `x6`; no stack stores for
  `s12` or `s13` are emitted before `bl fa1`
- observed generated callee entry unpack in `fa1`:
  `%p.a` starts at `x0`, `%p.b` at `x1`, `%p.c` at `x2`, `%p.d` at `x3`,
  `%p.e` at `x4`, and `%p.f` at `x5`, with two-slot byval formals reading
  the following register lane when their size exceeds 8 bytes

The exact first disagreement is after prepared call-plan/move publication
and before final AArch64 text. `src/backend/prealloc/call_plans.cpp`
produces the intended aggregate slot starts for this call, and
`src/backend/prealloc/regalloc.cpp` has move records for stack-bound
`%t107` and `%t108`. The residual is a combination in AArch64 codegen:

- caller publication: `src/backend/mir/aarch64/codegen/calls.cpp`
  treats `call_arg_byval_aggregate_register_lanes` as a register-lane path
  and does not lower the prepared stack-slot moves for the stack-transition
  byval aggregates into outgoing stack stores
- callee entry formal unpack: `src/backend/mir/aarch64/codegen/dispatch.cpp`
  `entry_formal_abi_register_index` increments by one previous same-bank
  formal instead of by rounded byval slot width and does not force the
  whole aggregate to incoming stack when there are not enough remaining GPRs

Do not classify this as semantic byte materialization, call-plan slot
accounting, or ordinary regalloc move construction. The call-plan surface is
useful as the oracle for the repair; the owning repair surfaces are AArch64
caller publication plus AArch64 entry formal unpack. One minor watch item:
`regalloc.cpp` still prints an auxiliary register ABI binding for the first
stack-transition byval arg (`abi_index=4` / `x7`), but the finalized call
argument plan records stack `+0`, so that binding is secondary unless a
repair path consumes it directly.

## Suggested Next

Execute Step 2: repair AArch64 byval aggregate caller publication and callee
entry formal unpack so both consume the same rounded slot starts.

Smallest credible proof command after code changes:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_runtime_byval_helper_payload_8_to_13|backend_runtime_byval_helper_payload_9_to_14|c_testsuite_aarch64_backend_src_00204_c)$'`.

Focused generated-state probes to reuse:
`./build/c4cll --target aarch64-linux-gnu --dump-bir --mir-focus-function arg tests/c/external/c-testsuite/src/00204.c | sed -n '520,660p'`,
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function arg --mir-focus-value t103 tests/c/external/c-testsuite/src/00204.c | sed -n '1,220p'`,
`./build/c4cll --target aarch64-linux-gnu --dump-prepared-bir --mir-focus-function fa1 tests/c/external/c-testsuite/src/00204.c | sed -n '1,220p'`, and
`sed -n '1188,1276p' build/c_testsuite_aarch64_backend/src/00204.c.s && sed -n '/^fa1:/,/^fa2:/p' build/c_testsuite_aarch64_backend/src/00204.c.s | sed -n '1,180p'`.

## Watchouts

Do not continue this residual under the HFA/floating idea. The new bad line
involves non-HFA `char[]` structs and matches a fixed aggregate byval
call-boundary placement problem.

Do not repair this with one function, one string literal, one register, one
stack offset, or one emitted instruction sequence. The real contract is the
general AAPCS64 rule for small non-HFA aggregates: each aggregate consumes its
own rounded register/stack slots, and an aggregate that cannot fit in the
remaining GPR argument registers must transition coherently to stack passing
for both caller publication and callee entry materialization.

Focused tests to repair next should assert both sides of the boundary:
prepared-call dump expectations for rounded slot starts, machine/instruction
dispatch coverage for outgoing stack publication and entry formal unpack, and
runtime coverage through `backend_runtime_byval_helper_payload_8_to_13`,
`backend_runtime_byval_helper_payload_9_to_14`, and
`c_testsuite_aarch64_backend_src_00204_c`.

The previous singleton HFA owner remains fixed; do not reopen HFA/floating
return publication, stdarg cursor/format, `va_start` overflow cursor
initialization, non-HFA aggregate `va_arg` materialization, fixed-formal entry
publication, local/value-home publication, or frame/formal publication unless
fresh generated evidence moves the first bad fact back to one of those owners.

## Proof

Todo-only localization packet. No implementation validation was run and no
`test_after.log` was updated.

Required final check for this packet:
`git diff --check`.
