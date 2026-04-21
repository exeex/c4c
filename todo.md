# Execution State

Status: Active
Source Idea Path: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Prepared-Module Or Call-Bundle Seam
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Plan Step 2.1 inspection confirmed that `c_testsuite_x86_backend_src_00204_c`
still never enters the bounded multi-defined helper-prefix route. The current
prepared helper renderer remains limited to `ptr`, `i32`, and immediate-`i64`
direct-extern call lanes, so the helper-prefix builder falls out once the
module reaches the HFA/float helper family and x86 drops back to the old
fallback `minimal i32 return function` diagnostic before any downstream
`myprintf` / `va_arg` ownership can apply.

## Suggested Next

Take a fresh idea-61 packet that adds generic prepared helper-prefix support
for floating/HFA direct-extern helper calls in the bounded same-module route,
with boundary coverage proving the helper lane consumes that family without
reopening local ABI inference. Then rerun `00204` to see whether the next real
blocker becomes the later `sret` or `myprintf` / `va_arg` family.

## Watchouts

- Do not paper over this by skipping unsupported helpers or adding another
  named-case route matcher; the helper-prefix contract is still all-or-nothing
  for non-entry defined functions in the bounded multi-defined lane.
- The repaired byval helper path still holds, but it is no longer the first
  rejector. The next packet should start from the earliest unsupported helper
  family after that repair, not from the already-landed byval route.
- Expect another downstream blocker after floating helper support lands. The
  semantic BIR for `00204` still contains later `sret` copy helpers and
  `myprintf` / `llvm.va_arg.aggregate` ownership that this packet did not try
  to solve.

## Proof

Investigated with the delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'; } > test_after.log 2>&1`.
Current proof still leaves `backend_x86_handoff_boundary` passing while
`c_testsuite_x86_backend_src_00204_c` fails with the old
`minimal i32 return function` diagnostic because the bounded helper-prefix lane
never activates for the module. Log path: `test_after.log`.
