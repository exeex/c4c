# Stage Diagnostics and Rendering

Status: scaffold.

Each stage may have a read-only renderer over its public view: Raw/Canonical
BIR, PreparedBir, and MIR. Renderers cannot own lookups, repair missing facts,
or influence compilation. Structured verifier/pass diagnostics are the source;
text is presentation only.

Legacy coverage: BIR printer/render paths, prepared printer and all its family
files, target machine printers, names, label identity, lookup agreement, and
debug-only publication views.
