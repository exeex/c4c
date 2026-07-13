# LIR-to-Raw-BIR Import Contract

Status: scaffold over an existing partial importer.

This stage translates LIR semantics into builder-created `RawBir`. It may use
temporary parsing, name, layout, and forward-reference maps, but none may become
published semantic authority. Every opcode family must declare operand/result
roles and successor traversal.

The importer does not perform target ABI placement, register allocation, frame
layout, call moves, instruction selection, or undocumented canonicalization.
Raw-only forms must be enumerated and owned by the first canonical pass that
eliminates them.

Legacy coverage review must trace both `src/backend/legacy/lir_to_bir.*` and the
current files in this directory without treating either representation as the
target contract.
