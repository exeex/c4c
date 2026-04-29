Status: Active
Source Idea Path: ideas/open/127_lir_structured_function_signature_metadata_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Audit And Follow-Up Queue

# Current Packet

## Just Finished

Completed `plan.md` Step 6, `Final Audit And Follow-Up Queue`, from the
executor perspective.

Re-ran the raw `rg` inventory for `signature_text` reads and signature-parser
helpers after Steps 2-5. Remaining runtime `src/` uses classify as:

- `src/codegen/lir/ir.hpp`: carrier field for final LLVM header spelling.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: producer writes final header
  text; `collect_fn_refs` scans it only as a compatibility carrier for embedded
  function references in attributes/metadata, not as signature semantic
  authority.
- `src/codegen/lir/lir_printer.cpp`: render path, including link-name
  replacement in final header spelling.
- `src/codegen/lir/verify.cpp`: consistency check that a renderable
  `define`/`declare` header exists; return, parameter, aggregate, variadic, and
  void-param facts are verified from structured signature metadata.
- `src/backend/bir/lir_to_bir/call_abi.cpp`: documented fallback parsing only
  when hand-built legacy LIR lacks `signature_return_type_ref`,
  `signature_params`, `signature_param_type_refs`, `signature_is_variadic`, and
  `signature_has_void_param_list`.
- `src/backend/bir/lir_to_bir/aggregate.cpp`: documented fallback parsing only
  when hand-built legacy LIR lacks structured signature params.
- `src/backend/bir/lir_to_bir/calling.cpp` and
  `src/codegen/lir/call_args.hpp`: call-operand/callee type-string parsers,
  not `LirFunction::signature_text` consumers.
- `src/backend/mir/aarch64/codegen/emit.cpp`: no remaining
  `function.signature_text` use in fast-path signature gates.

No remaining runtime `signature_text` use is classified as suspicious semantic
authority for generated LIR. No follow-up idea is required for this runbook's
signature metadata boundary.

## Suggested Next

Supervisor should run the broader validation checkpoint selected for closure,
then route to plan-owner to judge whether the source idea can close.

## Watchouts

BIR retains `signature_text` parsers as compatibility fallback for hand-built
legacy LIR fixtures with missing structured metadata. The HIR-to-LIR global-ref
scan still treats `signature_text` as a possible compatibility carrier for
embedded references, but that is not function-signature semantic authority.

## Proof

No build or test command was required for this documentation-only audit packet.
No proof logs were created or modified.
