Status: Active
Source Idea Path: ideas/open/40_riscv_prepared_edge_publication_typed_stack_source_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Typed Stack-Source Policy

# Current Packet

## Just Finished

Step 4 validated the typed scalar RISC-V `StackSlot -> Register`
edge-publication route as a documented fail-closed prepared-authority blocker.

The backend bucket is green after the explicit fail-closed guard and focused
negative coverage from the previous packet. A focused search of the RISC-V
emitter confirmed that no typed stack-source load opcodes were added for the
unsupported forms named by the source idea: `lb`, `lbu`, `lh`, `lhu`, `lwu`,
or `flw`.

Closure-ready: yes. The source idea is complete as a policy decision and proof
that typed scalar stack-source publication remains fail-closed until shared
prepared authority provides signedness/extension and destination
register-bank/view facts.

## Suggested Next

Supervisor should hand this active lifecycle state to the plan owner for
closure review of idea 40.

## Watchouts

This route intentionally does not add RISC-V typed load support. Future support
still needs shared prepared facts such as explicit stack-source extension kind
and destination register bank/view before target-local opcode selection. Do not
infer typed behavior from size, names, offsets, ids, or raw register spelling.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed, 163/163 tests. Proof log: `test_after.log`.

Additional checks:
`rg -n "\b(lwu|flw|lb|lbu|lh|lhu)\b" src/backend/mir/riscv/codegen/emit.cpp || true`
returned no matches.

`git diff --check -- todo.md` passed.
