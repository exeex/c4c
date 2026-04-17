Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Investigated the expanded `plan.md` Step 3 `00210` packet across
`x86_codegen.hpp`, `calls.cpp`, and `mod.cpp` and confirmed it is still
blocked before a coherent owned-file code change. The direct-call multi-function
piece is only part of the lane: in the prepared-module handoff, `00210`'s
external `printf` calls arrive with pointer args as named temps (`%t2`, `%t7`)
rather than explicit global or string-constant addresses, and the owned x86
prepared-module surface does not carry enough provenance to recover which
string/global symbol those pointer values denote. The split x86 call/data
helpers can cover call ABI setup and function/data preludes, but they do not
solve the missing prepared-BIR address-origin representation needed to emit the
`printf` string arguments honestly.

## Suggested Next

Implement the repaired `plan.md` Step 3 route with `00210` as the proving
anchor. The next executor packet should own the prepared-BIR or lowering
surface that preserves string/global-address provenance for direct-call pointer
args plus the canonical x86 prepared-module consumer and its shared call/data
helpers. Keep `00189`, `00057`, and `00124` out of scope unless the packet
explicitly re-opens the route.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- `00210` still does not prove global function-pointer plumbing: semantic BIR
  folds its casted function-pointer calls to direct `@actual_function` calls.
- The external `printf` calls in `00210` still require string/global-address
  provenance for pointer args even though the same-module helper call is direct.
- The x86 split codegen already has generic call ABI and symbol-prelude helpers
  in sibling translation units, but the canonical prepared-module consumer does
  not receive enough address-origin information to use them honestly for
  `00210` today.
- Do not satisfy the next packet by merely deleting the
  `functions.size() != 1` rejection or by adding a testcase-shaped `00210`
  matcher. The route must admit a reusable direct-call prepared-module family.
- Keep `00189`, `00057`, and `00124` out of this packet; they remain adjacent
  indirect-runtime, emitter/control-flow, and scalar-control-flow families.

## Proof

Baseline capture for the narrowed `00210` packet is in `test_before.log`:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00210_c)$' | tee test_before.log`

Step 3 blocker investigation reused the existing semantic-BIR dumps for
`00189` and `00210` plus owned-file inspection of the x86 prepared-module
consumer and sibling call/data helpers. The delegated proof command was not
rerun because the packet blocked before any owned-file code change. No
`test_after.log` was produced for this blocked packet.
