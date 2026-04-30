# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 const-int string overload cleanup was inspected but left blocked and
out of scope for this parser packet: parser-owned named constants already flow
through `std::unordered_map<TextId, long long>`, but deleting the rendered-name
`eval_const_int(...)` overload still requires non-parser HIR callers that pass
`NttpBindings`, currently `std::unordered_map<std::string, long long>`.

## Suggested Next

Next parser-only Step 2 packet should continue removing rendered-string
semantic lookup routes that are wholly owned by parser support code, while
leaving the const-int overload in place until a separate HIR
`NttpBindings`/metadata-carrier initiative is authorized.

## Watchouts

Exact blocker: `src/frontend/hir/impl/templates/templates.cpp` calls
`eval_const_int(..., const NttpBindings*)` at line 184 and
`eval_const_int(..., &kEmptyConsts)` at line 189; `NttpBindings` is declared as
`std::unordered_map<std::string, long long>` in `src/frontend/hir/hir_ir.hpp`.
Those files are outside this packet's ownership, so this parser packet must not
pull in HIR carrier migration work.

## Proof

Not run. No build or test was required for this todo-only cleanup, and this
blocked probe does not claim a fresh `test_after.log`.
