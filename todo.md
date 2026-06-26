Status: Active
Source Idea Path: ideas/open/378_rv64_object_route_20000112_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun Representative Case

# Current Packet

## Just Finished

Step 5 completed for the representative `src/20000112-1.c` rerun after
same-width integer `ZExt` lowering.

The previously audited first unsupported instruction fragment no longer fails
first:

```text
%t8 = bir.zext i32 %t7 to i32
```

The representative now compiles and links far enough to execute under qemu, but
the progress script reports `RV64_BACKEND_RUNTIME_MISMATCH`:

```text
clang_exit=0 c4c_exit=Segmentation fault
```

## Suggested Next

Ask the plan owner to decide whether idea 378 should close or hand off to a new
runtime-owner idea. The remaining failure is no longer the audited
instruction-fragment compile blocker and appears outside this idea's direct
scope.

## Watchouts

- Do not reopen idea 369 terminator-fragment lowering or CFG reconstruction.
- Do not treat diagnostic-only changes, allowlist edits, expectation rewrites,
  or named-case shortcuts as capability progress.
- `build/agent_state/378_step5_20000112.c4c_objdump.log` confirms the
  same-width `ZExt` publication in `main` as `mv s2,t0` around the former
  `%t8` result.
- The same object dump suggests a distinct join/select publication issue in
  `special_format`: skip blocks write `t0=1`, while later join blocks read
  `s2` before comparing.
- Keep any follow-up semantic and backed by prepared CFG/value-home facts; do
  not route this through testcase-shaped fixes for `src/20000112-1.c`.

## Proof

Delegated proof command run exactly:

```sh
mkdir -p build/agent_state && printf '%s\n' 'src/20000112-1.c' > build/agent_state/378_step5_20000112.allowlist.txt && ALLOWLIST=build/agent_state/378_step5_20000112.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1
```

Result: exited nonzero with `total=1 passed=0 failed=1` because the
representative now reaches runtime and segfaults under qemu. This is sufficient
for this proof-only classification packet because the audited unsupported
instruction-fragment blocker no longer fails first.

Evidence paths:

- `test_after.log`
- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
- `build/agent_state/378_step5_20000112.allowlist.txt`
- `build/agent_state/378_step5_20000112.c4c_objdump.log`
- `build/agent_state/378_step5_20000112.qemu_strace.err`
- `build/agent_state/378_step5_20000112.qemu_strace.out`
