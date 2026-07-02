Status: Active
Source Idea Path: ideas/open/520_bir_render_owner_preservation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Choose Preserve, Printer Move, Or Narrow Render TU

# Current Packet

## Just Finished

Step 2 - Choose Preserve, Printer Move, Or Narrow Render TU is complete for
idea 520.

Selected disposition:

- Move only the public BIR render helper definitions for `render_type`,
  `render_binary_opcode`, and `render_cast_opcode` into a new narrow public
  render translation unit in Step 3.
- Preserve the existing declarations in `bir.hpp`.
- Do not move these helper bodies into `bir_printer.cpp`.

Dependency rationale:

- The committed Step 1 audit found mixed public consumers, not printer-only
  consumers. `render_type` is used by BIR printer code, BIR lowering
  diagnostics/state text, prealloc legalize/prepared-printer diagnostics, x86
  debug, and RISC-V object-emission diagnostics.
- `render_binary_opcode` and `render_cast_opcode` are also public render
  helpers used outside the BIR printer by prealloc prepared-printer diagnostic
  code.
- Moving the bodies to `bir_printer.cpp` would force non-printer backend
  diagnostics and object-emission code to link through a printer-owned
  implementation file, creating ownership and dependency coupling that the
  cleanup is trying to avoid.
- A narrow render TU is clearer than preserving the bodies in central `bir.cpp`
  because these helpers are public text-rendering APIs with no recorded
  `bir.cpp`-local callees. Isolating their definitions keeps behavior and
  public signatures unchanged while separating render ownership from the large
  BIR model implementation.

## Suggested Next

Delegate Step 3 - Apply Minimal Ownership Change Or Preserve Decision. Move
only the three selected helper definitions from `bir.cpp` into a narrow public
render TU, preserve declarations in `bir.hpp`, and make only the build metadata
changes strictly required to compile that new TU.

## Watchouts

`bir_printer.cpp` is explicitly rejected for this route because the audited
consumer map is mixed, not printer-only. Step 3 should move only
`render_type`, `render_binary_opcode`, and `render_cast_opcode`; it should not
move route declarations, route records, validation behavior, printer output
spelling, tests, expectations, or unrelated build behavior. Preserving the
bodies in `bir.cpp` remains a fallback only if the narrow render TU exposes an
unexpected compile/link blocker.

## Proof

Decision-only packet. No build or ctest proof was required or run. The fixed
validation command `git diff --check -- todo.md` passed; no `test_after.log`
was created for this no-code decision.
