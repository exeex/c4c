# Current Packet

Status: Active
Source Idea Path: ideas/open/221_aarch64_markdown_driven_backend_case_bringup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Validate Existing Public Smoke Route

## Just Finished

Completed plan Step 3, `Validate Existing Public Smoke Route`, as a route
validation packet.

The existing narrow AArch64 backend smoke route is registered and green:

- `backend_cli_aarch64_asm_external_return_zero_smoke` is the positive public
  route.
- `backend_cli_aarch64_asm_no_machine_nodes_fails` is the negative guard route.

Positive route facts:

- Invokes public `c4cll --codegen asm --target aarch64-linux-gnu` output for
  `tests/backend/case/aarch64_return_zero_smoke.c`.
- Checks emitted assembly snippets: `.globl main`, `main:`, `mov w0, #0`, and
  `ret`.
- Assembles emitted assembly with `aarch64-linux-gnu-as`.
- Links the emitted assembly with `clang`.
- Runs the linked binary only when `CMAKE_HOST_SYSTEM_PROCESSOR` is `aarch64`
  or `arm64`; otherwise the runtime portion is skipped after assembly and link
  proof.

Negative route facts:

- Invokes public `c4cll --codegen asm --target aarch64-linux-gnu` for
  `tests/backend/case/aarch64_no_selected_machine_nodes.c`.
- Requires diagnostics containing `AArch64 backend assembly route reached the
  machine-node printer` and `no selected printable machine nodes`.
- Fails closed before output by asserting the compiler exits nonzero and
  `aarch64_no_selected_machine_nodes.s` is not written.

## Suggested Next

Execute the next supervisor-selected packet for Step 4. A coherent next packet
is to choose the first scalar return/ALU case from the bring-up matrix and
prepare a semantic implementation route before enabling any new AArch64 case.

## Watchouts

- Do not enable cases before the corresponding markdown feature has a structured implementation route.
- Do not weaken expectations or add testcase-shaped backend shortcuts.
- Keep public smoke validation on the c4cll route with external assembler/compiler proof where applicable.
- `run_backend_aarch64_external_smoke.cmake` compiles the emitted assembly with
  `clang` and assembles with `aarch64-linux-gnu-as`; it does not use the
  internal assembler, encoder, object writer, linker, or binary output route.
- The positive smoke's runtime execution is host-gated; non-AArch64 hosts still
  prove emission, snippet shape, external assembly, and external link.
- The first next implementation candidate named by the matrix is scalar
  return/ALU, but route design should come before case enablement.

## Proof

Delegated route proof:

```bash
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -N -R "^(backend_cli_aarch64_asm_no_machine_nodes_fails|backend_cli_aarch64_asm_external_return_zero_smoke)$" && ctest --test-dir build -j --output-on-failure -R "^(backend_cli_aarch64_asm_no_machine_nodes_fails|backend_cli_aarch64_asm_external_return_zero_smoke)$"' > test_after.log 2>&1
```

Proof log path: `test_after.log`. The proof is sufficient for Step 3 because it
builds the default preset, confirms both narrow AArch64 smoke CTest names are
registered, and runs both tests successfully.
