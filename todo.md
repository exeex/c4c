Status: Active
Source Idea Path: ideas/open/128_parser_ast_handoff_role_labeling.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Label `Node`

# Current Packet

## Just Finished

Step 2 added behavior-preserving boundary-role labels for `TypeSpec` and
`TemplateArgRef` in `src/frontend/parser/ast.hpp`.

The comment groups now distinguish cross-stage type/template contract payload,
parser-produced hints, compatibility/display spelling, debug/recovery payload,
and legacy bridge spelling without changing declarations or behavior.

## Suggested Next

Delegate Step 3 to an executor: add behavior-preserving role labels for `Node`
fields in `src/frontend/parser/ast.hpp`, explicitly marking cross-stage
payload, compatibility/display spelling, parser recovery/debug payload, and
legacy bridge fields.

## Watchouts

- Keep this runbook behavior-preserving.
- Do not replace string lookup paths or change parser/Sema/HIR/codegen behavior.
- Record suspicious cross-stage string authority as follow-up idea work instead
  of fixing it ad hoc.
- Preserve existing declaration order unless the supervisor explicitly delegates
  a layout-neutral regrouping.

## Proof

Ran `cmake --build --preset default > test_after.log 2>&1`; it completed
successfully with exit code 0. `test_after.log` is the proof log path.
