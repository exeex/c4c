# Current Packet

Status: Active
Source Idea Path: ideas/open/335_textual_assembler_object_route_followup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Textual Assembler Need And Seams

## Just Finished

- Completed Step 1 inspection of textual assembler need and route seams.
- Current `c4cll` route split is already explicit: `--codegen asm` selects
  backend-native textual assembly and writes text output, while `--codegen obj`
  calls the byte-result codegen facade, requires `-o`, and writes binary bytes.
- The codegen/backend APIs keep object output separate from text: object output
  goes through `emit_module_native_object(...)` and
  `backend::emit_module_object(...)`, not through `emit_module_native(...)` or
  terminal `.s` text.
- Backend object emission is target-owned and direct: RV64 routes prepared BIR
  into `write_rv64_prepared_relocatable_elf_object(...)`; AArch64 routes
  prepared BIR through selected machine/object records into
  `write_aarch64_relocatable_elf_object(...)`.
- Test helpers preserve coexistence:
  `c4c_add_backend_codegen_route_test(...)` and
  `run_backend_rv64_runtime_case.cmake` exercise `--codegen asm` and let clang
  assemble/link `.s`; `c4c_add_backend_codegen_object_test(...)` and
  `run_backend_rv64_object_runtime_case.cmake` exercise `--codegen obj`, check
  ELF machine bytes, and link the produced `.o` directly.
- Repo seam check found no `c4c-as` app binary or CMake target. There are
  staged assembler namespaces:
  `src/backend/mir/riscv/assembler/*` has a minimal text parser/object hook for
  bounded RV64 compatibility slices, and
  `src/backend/mir/aarch64/assembler/*` is documented as external-input
  staging, not an internal bridge from printed `--codegen asm` text back into
  the backend.

## Suggested Next

- Recommendation: close idea 335 with a no-work-needed decision for now.
- Rationale: the direct object route already satisfies the architectural need
  that compiler `.o` output not depend on textual round-tripping, asm-route
  compatibility remains separately tested through emitted `.s`, and the repo
  has no active standalone `c4c-as` CLI/test seam that needs implementation to
  complete the native-object umbrella.
- If textual assembler work is reactivated later, the smallest useful subset
  should be explicitly product-driven: a standalone external-input `c4c-as`
  style route for c4c-emitted RV64/AArch64 `.text` fragments, `.globl`/symbol
  labels, simple scalar instructions, direct call relocations, and existing ELF
  object model integration. That should be a separate child and must not become
  a dependency of `--codegen obj`.
- Suggested handoff: ask the plan owner to close child 335, then resume or close
  the umbrella idea based on the already-completed direct-object and asm
  coexistence evidence.

## Watchouts

- Do not make direct `--codegen obj` depend on textual assembly parsing.
- Do not build broad GNU assembler compatibility.
- Preserve existing asm-route and direct-object tests.
- Keep any implementation scoped to c4c-emitted assembly syntax and existing
  target encoder/object model integration.
- Do not treat the staged RV64/AArch64 assembler namespaces as proof that a
  supported standalone assembler route exists; they are compatibility seams and
  target internals until a CLI/test surface is intentionally added.

## Proof

- Inspection-only packet; no build required.
- Read-only inspection covered `src/apps/c4cll.cpp`,
  `src/codegen/llvm/llvm_codegen.*`, `src/backend/backend.*`,
  `src/backend/mir/{riscv,aarch64}/assembler/*`,
  `tests/backend/CMakeLists.txt`, and backend CMake route helpers.
- `git diff --check`: passed.
