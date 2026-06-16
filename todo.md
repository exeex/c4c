Status: Active
Source Idea Path: ideas/open/292_reopen_286_288_match_load_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace the semantic local-memory admission route

# Current Packet

## Just Finished

Step 2 complete: traced the `match` semantic local-memory load admission route.

Route:
- `coordinator.cpp::lower_scalar_or_local_memory_inst` dispatches
  `LirLoadOp` to `local_slots.cpp::lower_memory_load_inst`; failure records
  `load local-memory semantic family`.
- `lower_memory_load_inst` handles scalar/pointer loads by trying, in order:
  global provenance, dynamic pointer/global arrays, dynamic local aggregates,
  `try_lower_pointer_provenance_load`, then `try_lower_local_slot_load`.
- `try_lower_pointer_provenance_load` first uses
  `try_lower_addressed_pointer_load` when `pointer_value_addresses_` has the
  pointer SSA. That helper requires scalar-subobject addressability from the
  pointer address' `value_type` and `type_text`.
- `module.cpp::seed_pointer_param_addresses` currently seeds every ordinary
  formal pointer parameter as a runtime `PointerAddress` with
  `value_type = Void` and no type text.

Rejected shape:
- Source function `match(const char **s, const char *f)` repeatedly loads
  through formal pointer parameters: `const char *p = *s`, `*f`, and `*p`.
- Focused `--dump-bir --target aarch64-linux-gnu --mir-focus-function match`
  still fails before any BIR is emitted with latest function `match` in
  `load local-memory semantic family`.
- The rejected local-memory shape is a scalar/pointer load through a formal
  runtime pointer whose structured pointee fact is absent from
  `PointerAddress`, not an aggregate load, prepared-BIR publication issue, or
  rendered call-argument ABI suffix issue.

Missing/ignored fact:
- Missing fact class: `pointer` plus its `type`/pointee shape.
- Not implicated: local slot allocation, aggregate layout, prepared metadata,
  CLI expected text, or rendered `alignstack(...)` suffix parsing.

First semantic repair target:
- Teach `seed_pointer_param_addresses` to seed pointer formal parameters with
  structured pointee/load authority from signature metadata where available,
  so `try_lower_addressed_pointer_load` can admit loads like `*s` and `*f`
  semantically instead of treating formal pointers as opaque `Void` addresses.

## Suggested Next

Execute Step 3 from `plan.md`: implement the smallest general semantic repair
for formal pointer parameter load admission. Start at
`module.cpp::seed_pointer_param_addresses`, use structured signature parameter
metadata as the owner of pointee/load facts, and keep the repair independent of
the `00204.c` or `match` names.

## Watchouts

- Closed idea 286 originally failed in `myprintf`; the reopened failure now
  reports latest function `match`. Avoid replaying a named `myprintf` fix.
- Do not use named-case logic for `00204.c`, `match`, `myprintf`, `movi`, or
  HFA struct spellings.
- Do not reintroduce rendered call-argument `alignstack(...)` parsing as
  semantic authority; idea 291's structured metadata precedence must remain
  fenced.
- A debugger probe was attempted, but the available `build/c4cll` binary does
  not expose usable source symbols for this trace. The classification above is
  from clang-tool symbol lookup, focused source ranges, HIR for `match`, and
  focused dump failure output.
- Do not close idea 291 until this blocker is resolved and its close-time proof
  can pass.

## Proof

Delegated proof command:
`rg -n "lower_scalar_or_local_memory_inst|lower_memory_load_inst|load local-memory semantic family|LirLoadOp" src/backend/bir/lir_to_bir tests/backend -S`

Proof log:
`test_after.log`.

Result: passed; output records the coordinator dispatch/failure site, the
`lower_memory_load_inst` definition and uses, and relevant backend test
references.
