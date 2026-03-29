# AArch64 Local Memory Addressing Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/open/02_backend_aarch64_port_plan.md`

Should follow:

- the initial hello-world-level AArch64 `.s -> .o` bring-up tracked in `ideas/open/02_backend_aarch64_port_plan.md`

## Goal

Extend the AArch64 backend from the current narrow bring-up seam into explicit backend-owned lowering for local stack-slot addressing, starting with frontend-emitted local array shapes that require `alloca` plus `getelementptr`-based address formation instead of exact single-slot load/store collapse.

## Why This Is Separate

- the current bring-up runbook already proved the minimal AArch64 `.s -> .o` path and several bounded register-only or direct-call/branch seams
- the next remaining local-memory frontend case, `tests/c/internal/backend_case/local_array.c`, is not another exact `local_temp` rewrite
- its emitted LIR introduces array-typed stack allocas, base-pointer materialization, indexed `getelementptr`, and repeated address-based loads/stores
- widening the active bring-up runbook to absorb that work would silently expand the adapter and emitter contract from a narrow single-function seam into general local-memory and pointer/address lowering

## Scope

### Initial target

- support one bounded single-function local-array stack case through the backend-owned AArch64 asm path
- keep the first slice limited to stack-local storage with integer elements and straightforward indexed addressing
- make the stack-slot/addressing boundary explicit in backend code instead of hiding it inside ad hoc LIR pattern collapse

### Follow-on targets

- local address-taken scalars that need explicit stack-slot addressing
- repeated load/store traffic through derived local addresses
- small adjacent array/member-addressing cases only after the initial local-array slice is stable

## Explicit Non-Goals

- broad pointer arithmetic lowering
- global-address materialization
- struct/member-array generalization across unrelated shapes
- built-in assembler or linker work
- regalloc or peephole cleanup beyond what the first local-memory slice strictly requires

## Suggested Execution Order

1. capture the real frontend-emitted AArch64 LIR for `local_array.c` and the smallest synthetic backend test that matches it
2. define the narrowest backend-owned representation needed for stack-local base address plus indexed element addressing
3. add targeted adapter and/or emitter tests before implementation
4. route `backend_runtime_local_array` through `BACKEND_OUTPUT_KIND=asm` only when the backend-owned path is explicit and test-backed
5. stop and split again if the first implementation attempt pulls in broader pointer or aggregate lowering than the bounded local-array seam requires

## Validation

- `tests/backend/backend_lir_adapter_tests.cpp` covers the exact local-array addressing seam being implemented
- `backend_runtime_local_array` runs with `BACKEND_OUTPUT_KIND=asm`
- the backend-owned asm output assembles and preserves the expected runtime result for the bounded local-memory case
- the active AArch64 bring-up runbook remains narrow and does not silently absorb broader local-memory lowering

## Good First Patch

Promote `tests/c/internal/backend_case/local_array.c` onto the AArch64 asm path with the smallest explicit stack-slot/addressing seam that can be validated in both synthetic backend tests and the real backend runtime test.

## Completion

Status: Complete

Completed on: 2026-03-29

Completed work:

- captured the concrete `local_array.c` AArch64 LIR contract used for the first bounded local-memory slice
- added a synthetic backend assertion that requires explicit stack-slot base-address formation plus folded `#0`/`#4` local element offsets on the asm path
- taught the AArch64 emitter to recognize the bounded single-function local-array pattern and lower it to `sp`-relative `str/ldr` plus the final scalar `add`
- promoted `backend_runtime_local_array` to `BACKEND_OUTPUT_KIND=asm` and verified the expected exit code still holds

Leftover issues:

- broader local/member/global address lowering remains intentionally out of scope for this closed idea
- unrelated full-suite failures still remain in `positive_sema_ok_fn_returns_variadic_fn_ptr_c`, `cpp_positive_sema_decltype_bf16_builtin_cpp`, `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`, and `cpp_llvm_initializer_list_runtime_materialization`
