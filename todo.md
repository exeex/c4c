# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory X86 Prepared Renderer And Compile Surface

## Just Finished

Step 1 inventory completed for the x86 prepared-module renderer and compile
surface. Current entry points are `c4c::backend::x86::api::emit_prepared_module`
in `src/backend/mir/x86/api/api.cpp`, which delegates directly to
`c4c::backend::x86::module::emit` in
`src/backend/mir/x86/module/module.cpp`; public BIR/LIR x86 front doors in
`src/backend/backend.cpp` legalize to `prepare::PreparedBirModule` before the
same prepared-module entry. The module renderer validates prepared control-flow
label authority, appends grouped authority comments from prepared plans, emits
data through `x86::module::Data`, and currently emits only a contract-first
function stub body (`xor eax, eax` for non-void returns plus `ret`) rather than
semantic scalar/control-flow instruction text.

The stale compile blockers are the disabled x86 handoff aggregate sources under
`tests/backend/backend_x86_handoff_boundary*.cpp`: they still include
`src/backend/mir/x86/codegen/api/x86_codegen_api.hpp`, and scalar/compare/joined
multi-defined/local-i32 files also include
`src/backend/mir/x86/codegen/abi/x86_target_abi.hpp`. Current replacements are
`src/backend/mir/x86/api/api.hpp` and `src/backend/mir/x86/abi/abi.hpp`. The
aggregate target `backend_x86_handoff_boundary_test` is only created when
`C4C_ENABLE_X86_BACKEND_TESTS=ON`, so the default build hides these stale
headers.

Test classification: `backend_x86_prepared_handoff_label_authority_test` is
already compile-compatible and validates prepared label authority over the
current stub renderer. The `backend_x86_handoff_boundary_test` aggregate is
compile-blocked first; after include repair, scalar/direct-call/local-slot
cases become renderer-blocked because they expect canonical assembly instead of
the stub body, while compare/guard/join/short-circuit/loop-countdown cases are
control-flow-blocked on top of the same renderer gap. The x86 CLI dump/trace
tests are disabled when x86 backend tests are off and exercise route reporting,
not the prepared-module assembly renderer itself.

## Suggested Next

First narrow recovery packet: update only the disabled x86 handoff boundary test
includes from the stale `codegen/api` and `codegen/abi` paths to the current
`src/backend/mir/x86/api/api.hpp` and `src/backend/mir/x86/abi/abi.hpp` paths,
without changing expectations or renderer behavior. Suggested proof:
`cmake -S . -B build-x86 -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test -j2`.

## Watchouts

Do not mark the disabled x86 handoff tests unsupported or weaken canonical asm
expectations to get a green result. Include repair should expose the real
renderer/stub failures; implementation should then recover semantic rendering
rules rather than matching named fixtures or instruction counts. The first
behavior packet should probably start with a small scalar handoff source from
the aggregate after compile compatibility is restored.

## Proof

`git diff --check` passed. This is sufficient for the inventory-only packet
because no implementation, test, or build files were edited; the first compile
proof is recorded above for the next implementation packet.
