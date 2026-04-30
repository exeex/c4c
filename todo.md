# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Remove Sema Rendered-String Owner And Consteval Lookup Routes

## Just Finished

Step 2.5 verified the parser/Sema const-int boundary. Parser-owned named
constant evaluation stays on `TextId`/structured storage through
`Parser::eval_const_int_with_parser_tables()` and direct
`binding_state_.const_int_bindings` calls. The remaining rendered-name
`eval_const_int` compatibility overload is not a parser-owned route to convert
in this packet; its live named-constant use is HIR-backed `NttpBindings`
metadata and remains parked outside this parser/Sema plan.

## Suggested Next

Delegate Step 3 to inventory and remove one Sema rendered-string
owner/member/static/consteval lookup route. Start with the consteval lookup
helpers and owner/static call sites that still compare or recover semantic
identity through rendered names after a `Node*`, declaration, owner key,
`TextId`, or structured map is available.

## Watchouts

- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- Parser `eval_const_int` callers that have named constants already use
  `std::unordered_map<TextId, long long>`; parser three-argument layout calls
  do not perform named-constant lookup.
- Route deletion of the rendered-name `eval_const_int` compatibility overload
  through `ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a
  narrower HIR metadata idea before treating it as parser/Sema closure work.
- Step 3 must not count diagnostics, display, mangle/final spelling, or
  comment-only classification as semantic lookup removal.

## Proof

Lifecycle-only reset after Step 2.5 completion. No code proof was run or
claimed for this reset. Latest code proof remains the Step 2.5 focused
NTTP/const-int subset in `test_after.log`; regenerate matching regression logs
before using `test_before.log` and `test_after.log` as a close gate.
