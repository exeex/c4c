Status: Active
Source Idea Path: ideas/open/117_bir_printer_structured_render_context.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Inventory Remaining Fallbacks For Idea 118

# Current Packet

## Just Finished

Completed `plan.md` Step 6 by inventorying the remaining BIR printer
raw-string fallback surfaces for idea 118 and running the supervisor-selected
broad backend proof.

Concrete inventory:
- `bir::CallInst::return_type_name` remains as a compatibility fallback in
  `render_call_type_name`; it is not active authority for covered structured
  aggregate direct call return rendering when `structured_return_type_name` and
  `Module::structured_types` contain the aggregate declaration.
- Covered aggregate sret call paths now render the byte-stable ABI `void`
  spelling through structured metadata and `result_abi`, while preserving the
  legacy `return_type_name` field for fallback compatibility.
- Scalar call returns and calls without structured return metadata still fall
  back to `return_type_name` or `render_type(result.type)` and are deferred to
  idea 118 only after direct structured-only proof exists for those families.
- Explicit runtime helper placeholders that still set `.return_type_name =
  "void"` remain compatibility/output fallbacks for intrinsic-style calls such
  as varargs helpers, stack restore, and aggregate helper calls.
- Inline asm call return text remains raw fallback/output data through
  `return_type_name = trim(ret_type)`; inline asm has no structured return
  spelling route in this runbook.
- Inline asm `InlineAsmMetadata::{asm_text,constraints,args_text}` remain raw
  printer payloads. Parsed scalar inline-asm args clear `args_text` and print
  through normal BIR `args`; unparsed or non-scalar inline asm args still print
  as `raw_args`.

## Suggested Next

Supervisor can ask the plan owner to decide whether the active idea 117
runbook should close, remain open for more fallback coverage, or split any
uncovered fallback family into a later idea 118 packet.

## Watchouts

- Do not claim `return_type_name` or inline asm raw fields were removed; this
  packet only classified their current fallback status.
- Idea 118 should remove BIR printer fallback fields only per semantic family
  after structured-only tests prove the family no longer needs legacy text.
- Inline asm template and constraint strings may be final output payload, not
  type-text authority; separate those from type removal work before deleting
  anything.

## Proof

`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`
passed: build succeeded and CTest reported 107/107 runnable `backend_` tests
passed, with 12 disabled backend MIR-focus tests not run by CTest. Proof log:
`test_after.log`.
