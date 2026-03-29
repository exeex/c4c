#include "parser.hpp"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace c4c {

Parser::ParseContextGuard::ParseContextGuard(
    Parser* parser_in, const char* function_name)
    : parser(parser_in) {
    if (parser) parser->push_parse_context(function_name);
}

Parser::ParseContextGuard::~ParseContextGuard() {
    if (parser) parser->pop_parse_context();
}

Parser::Parser(std::vector<Token> tokens, Arena& arena, SourceProfile source_profile,
               const std::string& source_file)
    : tokens_(std::move(tokens)), pos_(0), arena_(arena), source_profile_(source_profile),
      source_file_(source_file),
      anon_counter_(0), had_error_(false), parsing_top_level_context_(false),
      last_enum_def_(nullptr) {
    namespace_contexts_.push_back(
        NamespaceContext{0, -1, false, arena_.strdup(""), arena_.strdup("")});
    namespace_stack_.push_back(0);
    // Pre-seed well-known typedef names from standard / system headers
    // so the parser can disambiguate type-name vs identifier.
    static const char* seed[] = {
        "size_t", "ssize_t", "ptrdiff_t", "intptr_t", "uintptr_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "int_least8_t", "int_least16_t", "int_least32_t", "int_least64_t",
        "uint_least8_t","uint_least16_t","uint_least32_t","uint_least64_t",
        "int_fast8_t", "int_fast16_t", "int_fast32_t", "int_fast64_t",
        "uint_fast8_t", "uint_fast16_t", "uint_fast32_t", "uint_fast64_t",
        "intmax_t", "uintmax_t",
        "off_t", "pid_t", "uid_t", "gid_t",
        "time_t", "clock_t", "suseconds_t",
        "FILE", "DIR", "fpos_t",
        "va_list", "__va_list", "__va_list_tag",
        "wchar_t", "wint_t", "wctype_t",
        "bool",
        "NULL",
        "__builtin_va_list", "__gnuc_va_list",
        "jmp_buf", "sigjmp_buf",
        "pthread_t", "pthread_mutex_t", "pthread_cond_t",
        "socklen_t", "in_addr_t", "in_port_t",
        "mode_t", "ino_t", "dev_t", "nlink_t", "blksize_t", "blkcnt_t",
        "div_t", "ldiv_t", "lldiv_t",
        "__int128_t", "__uint128_t",
        "__Float32x4_t", "__Float64x2_t",
        "__SVFloat32_t", "__SVFloat64_t", "__SVBool_t",
        "OSStatus", "OSErr",
        "__true_type", "__false_type",
        nullptr
    };
    for (int i = 0; seed[i]; ++i) typedefs_.insert(seed[i]);

    // Seed typedef_types_ for well-known types so they resolve to correct LLVM types
    // instead of the TB_TYPEDEF fallback (which emits i32).
    {
        TypeSpec va_ts{};
        va_ts.array_size = -1;
        va_ts.array_rank = 0;
        va_ts.is_ptr_to_array = false;
        va_ts.base = TB_VA_LIST;
        typedef_types_["va_list"]          = va_ts;
        typedef_types_["__va_list"]        = va_ts;
        typedef_types_["__builtin_va_list"]= va_ts;
        typedef_types_["__gnuc_va_list"]   = va_ts;

        // size_t / uintptr_t etc. → u64 on 64-bit platforms
        TypeSpec u64_ts{};
        u64_ts.array_size = -1;
        u64_ts.array_rank = 0;
        u64_ts.is_ptr_to_array = false;
        u64_ts.base = TB_ULONGLONG;  // 64-bit unsigned
        typedef_types_["size_t"]    = u64_ts;
        typedef_types_["uintptr_t"] = u64_ts;
        typedef_types_["uintmax_t"] = u64_ts;
        typedef_types_["uint64_t"]  = u64_ts;
        typedef_types_["uint_least64_t"] = u64_ts;
        typedef_types_["uint_fast64_t"]  = u64_ts;

        TypeSpec i64_ts{};
        i64_ts.array_size = -1;
        i64_ts.array_rank = 0;
        i64_ts.is_ptr_to_array = false;
        i64_ts.base = TB_LONGLONG;
        typedef_types_["ssize_t"]   = i64_ts;
        typedef_types_["ptrdiff_t"] = i64_ts;
        typedef_types_["intptr_t"]  = i64_ts;
        typedef_types_["intmax_t"]  = i64_ts;
        typedef_types_["int64_t"]   = i64_ts;
        typedef_types_["int_least64_t"] = i64_ts;
        typedef_types_["int_fast64_t"]  = i64_ts;
        typedef_types_["off_t"]     = i64_ts;

        TypeSpec u32_ts{};
        u32_ts.array_size = -1;
        u32_ts.array_rank = 0;
        u32_ts.is_ptr_to_array = false;
        u32_ts.base = TB_UINT;
        typedef_types_["uint32_t"]  = u32_ts;
        typedef_types_["uint_least32_t"] = u32_ts;
        typedef_types_["uint_fast32_t"]  = u32_ts;

        TypeSpec i32_ts{};
        i32_ts.array_size = -1;
        i32_ts.array_rank = 0;
        i32_ts.is_ptr_to_array = false;
        i32_ts.base = TB_INT;
        typedef_types_["int32_t"]   = i32_ts;
        typedef_types_["int_least32_t"] = i32_ts;
        typedef_types_["int_fast32_t"]  = i32_ts;

        TypeSpec u16_ts{};
        u16_ts.array_size = -1;
        u16_ts.array_rank = 0;
        u16_ts.is_ptr_to_array = false;
        u16_ts.base = TB_USHORT;
        typedef_types_["uint16_t"]  = u16_ts;
        typedef_types_["uint_least16_t"] = u16_ts;
        typedef_types_["uint_fast16_t"]  = u16_ts;

        TypeSpec i16_ts{};
        i16_ts.array_size = -1;
        i16_ts.array_rank = 0;
        i16_ts.is_ptr_to_array = false;
        i16_ts.base = TB_SHORT;
        typedef_types_["int16_t"]   = i16_ts;
        typedef_types_["int_least16_t"] = i16_ts;
        typedef_types_["int_fast16_t"]  = i16_ts;

        TypeSpec u8_ts{};
        u8_ts.array_size = -1;
        u8_ts.array_rank = 0;
        u8_ts.is_ptr_to_array = false;
        u8_ts.base = TB_UCHAR;
        typedef_types_["uint8_t"]   = u8_ts;
        typedef_types_["uint_least8_t"] = u8_ts;
        typedef_types_["uint_fast8_t"]  = u8_ts;

        TypeSpec i8_ts{};
        i8_ts.array_size = -1;
        i8_ts.array_rank = 0;
        i8_ts.is_ptr_to_array = false;
        i8_ts.base = TB_SCHAR;
        typedef_types_["int8_t"]    = i8_ts;
        typedef_types_["int_least8_t"] = i8_ts;
        typedef_types_["int_fast8_t"]  = i8_ts;

        // wchar_t on macOS/ARM64 is i32
        TypeSpec wchar_ts{};
        wchar_ts.array_size = -1;
        wchar_ts.array_rank = 0;
        wchar_ts.is_ptr_to_array = false;
        wchar_ts.base = TB_INT;
        typedef_types_["wchar_t"]  = wchar_ts;
        typedef_types_["wint_t"]   = wchar_ts;

        TypeSpec true_ts{};
        true_ts.array_size = -1;
        true_ts.array_rank = 0;
        true_ts.is_ptr_to_array = false;
        true_ts.base = TB_STRUCT;
        true_ts.tag = arena_.strdup("__true_type");
        typedef_types_["__true_type"] = true_ts;
        typedef_types_["std::__true_type"] = true_ts;
        typedef_types_["std::__8::__true_type"] = true_ts;

        TypeSpec false_ts{};
        false_ts.array_size = -1;
        false_ts.array_rank = 0;
        false_ts.is_ptr_to_array = false;
        false_ts.base = TB_STRUCT;
        false_ts.tag = arena_.strdup("__false_type");
        typedef_types_["__false_type"] = false_ts;
        typedef_types_["std::__false_type"] = false_ts;
        typedef_types_["std::__8::__false_type"] = false_ts;
    }
    refresh_current_namespace_bridge();
}

// ── pragma helpers ────────────────────────────────────────────────────────────

void Parser::handle_pragma_pack(const std::string& args) {
    // #pragma pack(N)        — set pack alignment to N
    // #pragma pack()         — reset to default (0)
    // #pragma pack(push)     — push current alignment
    // #pragma pack(push,N)   — push current alignment, set to N
    // #pragma pack(pop)      — pop previous alignment
    // #pragma pack(pop,N)    — pop and set to N
    // The lexeme has whitespace stripped and contains just the args, e.g. "1", "push,2", "pop", ""

    if (args.empty()) {
        pack_alignment_ = 0;
        return;
    }

    if (args.substr(0, 4) == "push") {
        pack_stack_.push_back(pack_alignment_);
        if (args.size() > 4 && args[4] == ',') {
            pack_alignment_ = std::stoi(args.substr(5));
        }
    } else if (args.substr(0, 3) == "pop") {
        if (!pack_stack_.empty()) {
            pack_alignment_ = pack_stack_.back();
            pack_stack_.pop_back();
        } else {
            pack_alignment_ = 0;
        }
        if (args.size() > 3 && args[3] == ',') {
            pack_alignment_ = std::stoi(args.substr(4));
        }
    } else {
        // Simple numeric value
        pack_alignment_ = std::stoi(args);
    }
}

void Parser::set_parser_debug(bool enabled) {
    parser_debug_enabled_ = enabled;
}

bool Parser::parser_debug_enabled() const {
    return parser_debug_enabled_;
}

void Parser::clear_parse_debug_state() {
    parse_context_stack_.clear();
    parse_debug_events_.clear();
    best_parse_failure_ = ParseFailure{};
}

void Parser::push_parse_context(const char* function_name) {
    ParseContextFrame frame;
    frame.function_name = function_name ? function_name : "";
    frame.token_index = pos_;
    parse_context_stack_.push_back(std::move(frame));
    note_parse_debug_event("enter");
}

void Parser::pop_parse_context() {
    note_parse_debug_event("leave");
    if (!parse_context_stack_.empty()) parse_context_stack_.pop_back();
}

void Parser::note_parse_debug_event(const char* kind, const char* detail) {
    if (!parser_debug_enabled_) return;

    ParseDebugEvent event;
    event.kind = kind ? kind : "";
    event.detail = detail ? detail : "";
    event.token_index = !at_end() ? pos_ : (pos_ > 0 ? pos_ - 1 : -1);
    event.line = !at_end() ? cur().line : (pos_ > 0 ? tokens_[pos_ - 1].line : 1);
    event.column = !at_end() ? cur().column : 1;
    if (!parse_context_stack_.empty()) {
        event.function_name = parse_context_stack_.back().function_name;
    }
    parse_debug_events_.push_back(std::move(event));
    if (static_cast<int>(parse_debug_events_.size()) > max_parse_debug_events_) {
        parse_debug_events_.erase(parse_debug_events_.begin());
    }
}

void Parser::note_parse_failure(const char* expected,
                                const char* detail,
                                bool committed) {
    ParseFailure failure;
    failure.active = true;
    failure.committed = committed;
    failure.token_index = !at_end() ? pos_ : (pos_ > 0 ? pos_ - 1 : -1);
    failure.line = !at_end() ? cur().line : (pos_ > 0 ? tokens_[pos_ - 1].line : 1);
    failure.column = !at_end() ? cur().column : 1;
    failure.expected = expected ? expected : "";
    failure.got = at_end() ? "<eof>" : cur().lexeme;
    failure.detail = detail ? detail : "";
    if (!parse_context_stack_.empty()) {
        failure.function_name = parse_context_stack_.back().function_name;
    }
    for (const ParseContextFrame& frame : parse_context_stack_) {
        failure.stack_trace.push_back(frame.function_name);
    }

    const bool replace =
        !best_parse_failure_.active ||
        failure.token_index > best_parse_failure_.token_index ||
        (failure.token_index == best_parse_failure_.token_index &&
         failure.committed && !best_parse_failure_.committed);
    if (replace) best_parse_failure_ = std::move(failure);

    note_parse_debug_event(committed ? "fail" : "soft_fail", detail);
}

void Parser::note_parse_failure_message(const char* detail, bool committed) {
    note_parse_failure(nullptr, detail, committed);
}

std::string Parser::format_best_parse_failure() const {
    if (!best_parse_failure_.active) return {};

    std::ostringstream oss;
    if (!best_parse_failure_.function_name.empty()) {
        oss << "parse_fn=" << best_parse_failure_.function_name;
    }
    if (best_parse_failure_.committed) {
        if (oss.tellp() > 0) oss << " ";
        oss << "phase=committed";
    }
    if (!best_parse_failure_.expected.empty()) {
        if (oss.tellp() > 0) oss << " ";
        oss << "expected=" << best_parse_failure_.expected;
        oss << " got='" << best_parse_failure_.got << "'";
    } else {
        if (oss.tellp() > 0) oss << " ";
        oss << "got='" << best_parse_failure_.got << "'";
    }
    if (!best_parse_failure_.detail.empty()) {
        if (oss.tellp() > 0) oss << " ";
        oss << "detail=\"" << best_parse_failure_.detail << "\"";
    }
    return oss.str();
}

void Parser::dump_parse_debug_trace() const {
    if (!parser_debug_enabled_ || parse_debug_events_.empty()) return;
    fprintf(stderr, "%s:%d:%d: note: parser debug trace follows\n",
            best_parse_failure_.active ? diag_file_at(best_parse_failure_.token_index)
                                       : source_file_.c_str(),
            best_parse_failure_.active ? best_parse_failure_.line : 1,
            best_parse_failure_.active ? best_parse_failure_.column : 1);
    for (const ParseDebugEvent& event : parse_debug_events_) {
        fprintf(stderr, "[pdebug] kind=%s",
                event.kind.c_str());
        if (!event.function_name.empty())
            fprintf(stderr, " fn=%s", event.function_name.c_str());
        fprintf(stderr, " line=%d col=%d", event.line, event.column);
        if (!event.detail.empty()) fprintf(stderr, " detail=\"%s\"", event.detail.c_str());
        fprintf(stderr, "\n");
    }
    if (best_parse_failure_.active && !best_parse_failure_.stack_trace.empty()) {
        fprintf(stderr, "[pdebug] stack:");
        for (const std::string& fn : best_parse_failure_.stack_trace) {
            fprintf(stderr, " -> %s", fn.c_str());
        }
        fprintf(stderr, "\n");
    }
}

const char* Parser::diag_file_at(int token_index) const {
    if (token_index >= 0 && token_index < static_cast<int>(tokens_.size())) {
        const std::string& file = tokens_[token_index].file;
        if (!file.empty()) return file.c_str();
    }
    return source_file_.c_str();
}

void Parser::handle_pragma_gcc_visibility(const std::string& args) {
    // Lexeme format: "push,<visibility>" or "pop"
    if (args == "pop") {
        if (!visibility_stack_.empty()) {
            visibility_ = visibility_stack_.back();
            visibility_stack_.pop_back();
        } else {
            visibility_ = 0;  // default
        }
    } else if (args.substr(0, 5) == "push,") {
        visibility_stack_.push_back(visibility_);
        const std::string vis = args.substr(5);
        if (vis == "hidden") visibility_ = 1;
        else if (vis == "protected") visibility_ = 2;
        else visibility_ = 0;  // "default" or unknown → default
    }
}

// ── token cursor helpers ──────────────────────────────────────────────────────

const Token& Parser::cur() const {
    return tokens_[pos_];
}

const Token& Parser::peek(int offset) const {
    int idx = pos_ + offset;
    if (idx < 0 || idx >= (int)tokens_.size()) {
        static Token eof{TokenKind::EndOfFile, "", 0, 0};
        return eof;
    }
    return tokens_[idx];
}

const Token& Parser::consume() {
    const Token& t = tokens_[pos_];
    if (pos_ + 1 < (int)tokens_.size()) ++pos_;
    return t;
}

bool Parser::at_end() const {
    if (pos_ < 0 || pos_ >= static_cast<int>(tokens_.size())) return true;
    return tokens_[pos_].kind == TokenKind::EndOfFile;
}

bool Parser::check(TokenKind k) const {
    if (pos_ < 0 || pos_ >= static_cast<int>(tokens_.size())) return false;
    return tokens_[pos_].kind == k;
}

bool Parser::check2(TokenKind k) const {
    int idx = pos_ + 1;
    if (idx >= (int)tokens_.size()) return false;
    return tokens_[idx].kind == k;
}

bool Parser::match(TokenKind k) {
    if (check(k)) { consume(); return true; }
    return false;
}

std::string Parser::qualify_name(const std::string& name) const {
    if (name.empty()) return name;
    const int context_id = current_namespace_context_id();
    if (context_id <= 0) return name;
    if (name.find("::") != std::string::npos) return name;
    return canonical_name_in_context(context_id, name);
}

const char* Parser::qualify_name_arena(const char* name) {
    if (!name || !name[0]) return name;
    std::string qualified = qualify_name(name);
    if (qualified == name) return name;
    return arena_.strdup(qualified.c_str());
}

std::string Parser::resolve_visible_value_name(const std::string& name) const {
    std::string resolved;
    for (int i = static_cast<int>(namespace_stack_.size()) - 1; i >= 0; --i) {
        const int context_id = namespace_stack_[i];
        auto alias_it = using_value_aliases_.find(context_id);
        if (alias_it != using_value_aliases_.end()) {
            auto value_it = alias_it->second.find(name);
            if (value_it != alias_it->second.end()) return value_it->second;
        }
        if (lookup_value_in_context(context_id, name, &resolved)) return resolved;
    }
    return name;
}

std::string Parser::resolve_visible_type_name(const std::string& name) const {
    std::string resolved;
    for (int i = static_cast<int>(namespace_stack_.size()) - 1; i >= 0; --i) {
        const int context_id = namespace_stack_[i];
        auto alias_it = using_value_aliases_.find(context_id);
        if (alias_it != using_value_aliases_.end()) {
            auto value_it = alias_it->second.find(name);
            if (value_it != alias_it->second.end() && typedef_types_.count(value_it->second)) {
                return value_it->second;
            }
        }
        if (lookup_type_in_context(context_id, name, &resolved)) return resolved;
    }
    return name;
}

void Parser::refresh_current_namespace_bridge() {
    current_namespace_.clear();
    for (size_t i = 1; i < namespace_stack_.size(); ++i) {
        const NamespaceContext& ctx = namespace_contexts_[namespace_stack_[i]];
        if (!ctx.canonical_name || !ctx.canonical_name[0]) continue;
        current_namespace_ = ctx.canonical_name;
    }
}

int Parser::current_namespace_context_id() const {
    if (namespace_stack_.empty()) return 0;
    return namespace_stack_.back();
}

int Parser::ensure_named_namespace_context(int parent_id, const std::string& name) {
    const std::string key = std::to_string(parent_id) + "::" + name;
    auto it = named_namespace_contexts_.find(key);
    if (it != named_namespace_contexts_.end()) return it->second;

    const NamespaceContext& parent = namespace_contexts_[parent_id];
    std::string canonical = parent.canonical_name ? parent.canonical_name : "";
    if (!canonical.empty()) canonical += "::";
    canonical += name;

    const int id = static_cast<int>(namespace_contexts_.size());
    namespace_contexts_.push_back(
        NamespaceContext{id, parent_id, false, arena_.strdup(name.c_str()),
                         arena_.strdup(canonical.c_str())});
    named_namespace_contexts_[key] = id;
    return id;
}

int Parser::create_anonymous_namespace_context(int parent_id) {
    const NamespaceContext& parent = namespace_contexts_[parent_id];
    std::string canonical = parent.canonical_name ? parent.canonical_name : "";
    if (!canonical.empty()) canonical += "::";
    canonical += "__anon_ns_";
    canonical += std::to_string(anon_counter_++);

    const int id = static_cast<int>(namespace_contexts_.size());
    namespace_contexts_.push_back(
        NamespaceContext{id, parent_id, true, arena_.strdup(""),
                         arena_.strdup(canonical.c_str())});
    anonymous_namespace_children_[parent_id].push_back(id);
    return id;
}

void Parser::push_namespace_context(int context_id) {
    namespace_stack_.push_back(context_id);
    refresh_current_namespace_bridge();
}

void Parser::pop_namespace_context() {
    if (namespace_stack_.size() > 1) namespace_stack_.pop_back();
    refresh_current_namespace_bridge();
}

std::string Parser::canonical_name_in_context(int context_id, const std::string& name) const {
    if (name.empty()) return name;
    if (context_id <= 0) return name;
    const NamespaceContext& ctx = namespace_contexts_[context_id];
    if (!ctx.canonical_name || !ctx.canonical_name[0]) return name;
    return std::string(ctx.canonical_name) + "::" + name;
}

int Parser::resolve_namespace_context(const QualifiedNameRef& name) const {
    auto follow_path = [&](int start_id) -> int {
        int context_id = start_id;
        for (const std::string& segment : name.qualifier_segments) {
            const std::string key = std::to_string(context_id) + "::" + segment;
            auto it = named_namespace_contexts_.find(key);
            if (it == named_namespace_contexts_.end()) return -1;
            context_id = it->second;
        }
        return context_id;
    };

    if (name.is_global_qualified) return follow_path(0);

    for (int i = static_cast<int>(namespace_stack_.size()) - 1; i >= 0; --i) {
        int resolved = follow_path(namespace_stack_[i]);
        if (resolved >= 0) return resolved;
    }
    return follow_path(0);
}

int Parser::resolve_namespace_name(const QualifiedNameRef& name) const {
    auto follow_name = [&](int start_id) -> int {
        int context_id = start_id;
        for (const std::string& segment : name.qualifier_segments) {
            const std::string key = std::to_string(context_id) + "::" + segment;
            auto it = named_namespace_contexts_.find(key);
            if (it == named_namespace_contexts_.end()) return -1;
            context_id = it->second;
        }
        const std::string final_key = std::to_string(context_id) + "::" + name.base_name;
        auto it = named_namespace_contexts_.find(final_key);
        if (it == named_namespace_contexts_.end()) return -1;
        return it->second;
    };

    if (name.is_global_qualified) return follow_name(0);

    for (int i = static_cast<int>(namespace_stack_.size()) - 1; i >= 0; --i) {
        int resolved = follow_name(namespace_stack_[i]);
        if (resolved >= 0) return resolved;
    }
    return follow_name(0);
}

bool Parser::lookup_value_in_context(int context_id, const std::string& name,
                                     std::string* resolved) const {
    const std::string candidate = canonical_name_in_context(context_id, name);
    if (var_types_.count(candidate) || known_fn_names_.count(candidate)) {
        *resolved = candidate;
        return true;
    }
    if (context_id == 0 && (var_types_.count(name) || known_fn_names_.count(name))) {
        *resolved = name;
        return true;
    }

    auto anon_it = anonymous_namespace_children_.find(context_id);
    if (anon_it != anonymous_namespace_children_.end()) {
        for (int anon_id : anon_it->second) {
            if (lookup_value_in_context(anon_id, name, resolved)) return true;
        }
    }

    auto using_it = using_namespace_contexts_.find(context_id);
    if (using_it != using_namespace_contexts_.end()) {
        for (int imported_id : using_it->second) {
            if (lookup_value_in_context(imported_id, name, resolved)) return true;
        }
    }
    return false;
}

bool Parser::lookup_type_in_context(int context_id, const std::string& name,
                                    std::string* resolved) const {
    const std::string candidate = canonical_name_in_context(context_id, name);
    if (typedef_types_.count(candidate)) {
        *resolved = candidate;
        return true;
    }
    if (context_id == 0 && typedef_types_.count(name)) {
        *resolved = name;
        return true;
    }

    auto anon_it = anonymous_namespace_children_.find(context_id);
    if (anon_it != anonymous_namespace_children_.end()) {
        for (int anon_id : anon_it->second) {
            if (lookup_type_in_context(anon_id, name, resolved)) return true;
        }
    }

    auto using_it = using_namespace_contexts_.find(context_id);
    if (using_it != using_namespace_contexts_.end()) {
        for (int imported_id : using_it->second) {
            if (lookup_type_in_context(imported_id, name, resolved)) return true;
        }
    }
    return false;
}

bool Parser::peek_qualified_name(QualifiedNameRef* out, bool allow_global) const {
    if (!out) return false;
    QualifiedNameRef result;
    int p = pos_;
    if (allow_global && p < static_cast<int>(tokens_.size()) &&
        tokens_[p].kind == TokenKind::ColonColon) {
        result.is_global_qualified = true;
        ++p;
    }
    if (p >= static_cast<int>(tokens_.size()) || tokens_[p].kind != TokenKind::Identifier) {
        return false;
    }
    result.base_name = tokens_[p].lexeme;
    ++p;
    while (p + 1 < static_cast<int>(tokens_.size()) &&
           tokens_[p].kind == TokenKind::ColonColon &&
           tokens_[p + 1].kind == TokenKind::Identifier) {
        result.qualifier_segments.push_back(result.base_name);
        result.base_name = tokens_[p + 1].lexeme;
        p += 2;
    }
    *out = std::move(result);
    return true;
}

Parser::QualifiedNameRef Parser::parse_qualified_name(bool allow_global) {
    QualifiedNameRef result;
    if (!peek_qualified_name(&result, allow_global)) {
        throw std::runtime_error("expected qualified name");
    }
    if (result.is_global_qualified) expect(TokenKind::ColonColon);
    expect(TokenKind::Identifier);
    for (size_t i = 0; i < result.qualifier_segments.size(); ++i) {
        expect(TokenKind::ColonColon);
        expect(TokenKind::Identifier);
    }
    return result;
}

void Parser::apply_qualified_name(Node* node, const QualifiedNameRef& qn,
                                  const char* resolved_name) {
    if (!node) return;
    node->is_global_qualified = qn.is_global_qualified;
    node->unqualified_name = arena_.strdup(qn.base_name.c_str());
    node->n_qualifier_segments = static_cast<int>(qn.qualifier_segments.size());
    if (node->n_qualifier_segments > 0) {
        node->qualifier_segments = arena_.alloc_array<const char*>(node->n_qualifier_segments);
        for (int i = 0; i < node->n_qualifier_segments; ++i) {
            node->qualifier_segments[i] = arena_.strdup(qn.qualifier_segments[i].c_str());
        }
    }
    if (resolved_name && resolved_name[0]) {
        QualifiedNameRef qualifier_only = qn;
        node->namespace_context_id = resolve_namespace_context(qualifier_only);
    }
}

void Parser::apply_decl_namespace(Node* node, int context_id, const char* unqualified_name) {
    if (!node) return;
    node->namespace_context_id = context_id;
    node->unqualified_name = unqualified_name;

    std::vector<const char*> segments;
    for (int walk = context_id; walk > 0; walk = namespace_contexts_[walk].parent_id) {
        const NamespaceContext& ctx = namespace_contexts_[walk];
        if (ctx.is_anonymous || !ctx.display_name || !ctx.display_name[0]) continue;
        segments.push_back(ctx.display_name);
    }
    node->n_qualifier_segments = static_cast<int>(segments.size());
    if (node->n_qualifier_segments > 0) {
        node->qualifier_segments = arena_.alloc_array<const char*>(node->n_qualifier_segments);
        for (int i = 0; i < node->n_qualifier_segments; ++i) {
            node->qualifier_segments[i] = segments[node->n_qualifier_segments - 1 - i];
        }
    }
}

void Parser::expect(TokenKind k) {
    if (!check(k)) {
        note_parse_failure(token_kind_name(k));
        std::ostringstream msg;
        msg << "expected " << token_kind_name(k) << " but got '"
            << cur().lexeme << "' at line " << cur().line;
        throw std::runtime_error(msg.str());
    }
    consume();
}

bool Parser::check_template_close() const {
    return check(TokenKind::Greater) ||
           check(TokenKind::GreaterGreater) ||
           check(TokenKind::GreaterEqual) ||
           check(TokenKind::GreaterGreaterAssign);
}

bool Parser::parse_greater_than_in_template_list(bool consume_last_token) {
    if (check(TokenKind::Greater)) {
        if (consume_last_token) consume();
        return true;
    }

    if (check(TokenKind::GreaterGreater)) {
        // Consume one > and leave the second > in the token stream.
        tokens_[pos_].kind = TokenKind::Greater;
        tokens_[pos_].lexeme = ">";
        return true;
    }

    if (check(TokenKind::GreaterEqual)) {
        // Consume one > and leave = in the token stream.
        tokens_[pos_].kind = TokenKind::Assign;
        tokens_[pos_].lexeme = "=";
        return true;
    }

    if (check(TokenKind::GreaterGreaterAssign)) {
        // Consume one > and leave >= in the token stream.
        tokens_[pos_].kind = TokenKind::GreaterEqual;
        tokens_[pos_].lexeme = ">=";
        return true;
    }

    return false;
}

bool Parser::match_template_close() {
    return parse_greater_than_in_template_list(true);
}

void Parser::expect_template_close() {
    if (!match_template_close()) {
        std::ostringstream msg;
        msg << "expected " << token_kind_name(TokenKind::Greater) << " but got '"
            << cur().lexeme << "' at line " << cur().line;
        throw std::runtime_error(msg.str());
    }
}

void Parser::skip_until(TokenKind k) {
    while (!at_end() && !check(k)) consume();
    if (!at_end()) consume();  // consume the terminator
}

Node* Parser::parse() {
    std::vector<Node*> items;
    had_error_ = false;
    parse_error_count_ = 0;
    int no_progress_steps = 0;

    while (!at_end()) {
        Node* item = nullptr;
        int loop_start_pos = pos_;
        clear_parse_debug_state();
        try {
            item = parse_top_level();
        } catch (const std::exception& e) {
            // Parse error: emit stable diagnostic and try to recover.
            int err_idx = best_parse_failure_.active
                              ? best_parse_failure_.token_index
                              : (!at_end() ? pos_ : (pos_ > 0 ? pos_ - 1 : -1));
            int err_line = best_parse_failure_.active
                               ? best_parse_failure_.line
                               : ((!at_end()) ? cur().line : (pos_ > 0 ? tokens_[pos_-1].line : 1));
            int err_col  = best_parse_failure_.active
                               ? best_parse_failure_.column
                               : ((!at_end()) ? cur().column : 1);
            std::string diag = format_best_parse_failure();
            fprintf(stderr, "%s:%d:%d: error: %s\n",
                    diag_file_at(err_idx), err_line, err_col,
                    diag.empty() ? e.what() : diag.c_str());
            dump_parse_debug_trace();
            had_error_ = true;
            ++parse_error_count_;
            if (parse_error_count_ >= max_parse_errors_) {
                fprintf(stderr, "%s:%d:%d: error: too many errors emitted, stopping now\n",
                        diag_file_at(err_idx), err_line, err_col);
                break;
            }
            if (pos_ == loop_start_pos && !at_end()) consume();
            while (!at_end() && !check(TokenKind::Semi) && !check(TokenKind::RBrace)) {
                consume();
            }
            if (!at_end()) consume();
            no_progress_steps = 0;
            continue;
        }
        if (pos_ == loop_start_pos && !at_end()) {
            had_error_ = true;
            ++no_progress_steps;
            note_parse_failure_message("unexpected token with no parse progress");
            std::string diag = format_best_parse_failure();
            fprintf(stderr, "%s:%d:%d: error: unexpected token '%s'\n",
                    diag_file_at(pos_), cur().line, cur().column,
                    cur().lexeme.c_str());
            if (!diag.empty()) {
                fprintf(stderr, "%s:%d:%d: note: %s\n",
                        diag_file_at(pos_), cur().line, cur().column,
                        diag.c_str());
            }
            dump_parse_debug_trace();
            consume();
            ++parse_error_count_;
            if (no_progress_steps >= max_no_progress_steps_) {
                fprintf(stderr, "%s:%d:%d: error: too many errors emitted, stopping now\n",
                        diag_file_at(!at_end() ? pos_ : static_cast<int>(tokens_.size()) - 1),
                        !at_end() ? cur().line : tokens_.back().line,
                        !at_end() ? cur().column : 1);
                break;
            }
            continue;
        }
        no_progress_steps = 0;
        if (item) items.push_back(item);
    }

    // Prepend collected struct/enum definitions (so IR builder sees them first)
    std::vector<Node*> all;
    for (Node* sd : struct_defs_) all.push_back(sd);
    for (Node* it : items)        all.push_back(it);

    Node* prog = make_node(NK_PROGRAM, 0);
    prog->n_children = (int)all.size();
    if (prog->n_children > 0) {
        prog->children = arena_.alloc_array<Node*>(prog->n_children);
        for (int i = 0; i < prog->n_children; ++i) prog->children[i] = all[i];
    }
    return prog;
}

// ── node constructors ─────────────────────────────────────────────────────────

Node* Parser::make_node(NodeKind k, int line) {
    Node* n = arena_.alloc_array<Node>(1);
    std::memset(n, 0, sizeof(Node));
    n->kind = k;
    n->line = line;
    n->ival = -1;  // -1 = not a bitfield (for struct field declarations)
    n->builtin_id = BuiltinId::Unknown;
    n->type.array_size = -1;
    n->type.array_rank = 0;
    for (int i = 0; i < 8; ++i) n->type.array_dims[i] = -1;
    n->type.inner_rank = -1;
    n->type.is_ptr_to_array = false;
    return n;
}

Node* Parser::make_int_lit(long long v, int line) {
    Node* n = make_node(NK_INT_LIT, line);
    n->ival = v;
    return n;
}

Node* Parser::make_float_lit(double v, const char* raw, int line) {
    Node* n = make_node(NK_FLOAT_LIT, line);
    n->fval = v;
    n->sval = raw;
    return n;
}

Node* Parser::make_str_lit(const char* raw, int line) {
    Node* n = make_node(NK_STR_LIT, line);
    n->sval = raw;
    return n;
}

Node* Parser::make_var(const char* name, int line) {
    Node* n = make_node(NK_VAR, line);
    n->name = name;
    return n;
}

Node* Parser::make_binop(const char* op, Node* l, Node* r, int line) {
    Node* n = make_node(NK_BINOP, line);
    n->op    = arena_.strdup(op);
    n->left  = l;
    n->right = r;
    return n;
}

Node* Parser::make_unary(const char* op, Node* operand, int line) {
    Node* n = make_node(NK_UNARY, line);
    n->op   = arena_.strdup(op);
    n->left = operand;
    return n;
}

Node* Parser::make_assign(const char* op, Node* lhs, Node* rhs, int line) {
    Node* n = make_node(NK_ASSIGN, line);
    n->op    = arena_.strdup(op);
    n->left  = lhs;
    n->right = rhs;
    return n;
}

Node* Parser::make_block(Node** stmts, int n_stmts, int line) {
    Node* n = make_node(NK_BLOCK, line);
    n->n_children = n_stmts;
    if (n_stmts > 0 && stmts) {
        n->children = arena_.alloc_array<Node*>(n_stmts);
        for (int i = 0; i < n_stmts; ++i) n->children[i] = stmts[i];
    }
    return n;
}

// ── AST dump ─────────────────────────────────────────────────────────────────

const char* operator_kind_mangled_name(OperatorKind ok) {
    switch (ok) {
        case OP_NONE:      return nullptr;
        case OP_SUBSCRIPT: return "operator_subscript";
        case OP_DEREF:     return "operator_deref";
        case OP_ARROW:     return "operator_arrow";
        case OP_PRE_INC:   return "operator_preinc";
        case OP_POST_INC:  return "operator_postinc";
        case OP_EQ:        return "operator_eq";
        case OP_NEQ:       return "operator_neq";
        case OP_BOOL:      return "operator_bool";
        case OP_PLUS:      return "operator_plus";
        case OP_MINUS:     return "operator_minus";
        case OP_ASSIGN:    return "operator_assign";
        case OP_CALL:      return "operator_call";
        case OP_LT:        return "operator_lt";
        case OP_GT:        return "operator_gt";
        case OP_LE:        return "operator_le";
        case OP_GE:        return "operator_ge";
    }
    return nullptr;
}

const char* node_kind_name(NodeKind k) {
    switch (k) {
        case NK_INT_LIT:      return "IntLit";
        case NK_FLOAT_LIT:    return "FloatLit";
        case NK_STR_LIT:      return "StrLit";
        case NK_CHAR_LIT:     return "CharLit";
        case NK_VAR:          return "Var";
        case NK_BINOP:        return "BinOp";
        case NK_UNARY:        return "Unary";
        case NK_POSTFIX:      return "Postfix";
        case NK_ADDR:         return "Addr";
        case NK_DEREF:        return "Deref";
        case NK_ASSIGN:       return "Assign";
        case NK_COMPOUND_ASSIGN: return "CompoundAssign";
        case NK_CALL:         return "Call";
        case NK_BUILTIN_CALL: return "BuiltinCall";
        case NK_INDEX:        return "Index";
        case NK_MEMBER:       return "Member";
        case NK_CAST:         return "Cast";
        case NK_TERNARY:      return "Ternary";
        case NK_SIZEOF_EXPR:  return "SizeofExpr";
        case NK_SIZEOF_TYPE:  return "SizeofType";
        case NK_SIZEOF_PACK:  return "SizeofPack";
        case NK_ALIGNOF_TYPE: return "AlignofType";
        case NK_ALIGNOF_EXPR: return "AlignofExpr";
        case NK_COMMA_EXPR:   return "CommaExpr";
        case NK_STMT_EXPR:    return "StmtExpr";
        case NK_COMPOUND_LIT: return "CompoundLit";
        case NK_VA_ARG:       return "VaArg";
        case NK_GENERIC:      return "Generic";
        case NK_INIT_LIST:    return "InitList";
        case NK_INIT_ITEM:    return "InitItem";
        case NK_BLOCK:        return "Block";
        case NK_EXPR_STMT:    return "ExprStmt";
        case NK_RETURN:       return "Return";
        case NK_IF:           return "If";
        case NK_WHILE:        return "While";
        case NK_FOR:          return "For";
        case NK_RANGE_FOR:    return "RangeFor";
        case NK_DO_WHILE:     return "DoWhile";
        case NK_SWITCH:       return "Switch";
        case NK_CASE:         return "Case";
        case NK_CASE_RANGE:   return "CaseRange";
        case NK_DEFAULT:      return "Default";
        case NK_BREAK:        return "Break";
        case NK_CONTINUE:     return "Continue";
        case NK_GOTO:         return "Goto";
        case NK_LABEL:        return "Label";
        case NK_ASM:          return "Asm";
        case NK_EMPTY:        return "Empty";
        case NK_DECL:         return "Decl";
        case NK_FUNCTION:     return "Function";
        case NK_GLOBAL_VAR:   return "GlobalVar";
        case NK_STRUCT_DEF:   return "StructDef";
        case NK_ENUM_DEF:     return "EnumDef";
        case NK_OFFSETOF:     return "Offsetof";
        case NK_PRAGMA_WEAK:  return "PragmaWeak";
        case NK_NEW_EXPR:     return "NewExpr";
        case NK_DELETE_EXPR:  return "DeleteExpr";
        case NK_REAL_PART:    return "RealPart";
        case NK_IMAG_PART:    return "ImagPart";
        case NK_INVALID_EXPR: return "InvalidExpr";
        case NK_INVALID_STMT: return "InvalidStmt";
        case NK_PROGRAM:      return "Program";
        default:              return "Unknown";
    }
}

static void indent_print(int depth) {
    for (int i = 0; i < depth * 2; ++i) putchar(' ');
}

void ast_dump(const Node* n, int indent) {
    if (!n) { indent_print(indent); printf("<null>\n"); return; }
    indent_print(indent);
    printf("%s", node_kind_name(n->kind));
    switch (n->kind) {
        case NK_INT_LIT:   printf("(%lld)", n->ival); break;
        case NK_FLOAT_LIT: printf("(%g)", n->fval); break;
        case NK_CHAR_LIT:  printf("(%lld)", n->ival); break;
        case NK_STR_LIT:   printf("(...)"); break;
        case NK_VAR:       printf("(%s)", n->name ? n->name : "?"); break;
        case NK_BINOP:     printf("(%s)", n->op ? n->op : "?"); break;
        case NK_UNARY:     printf("(%s)", n->op ? n->op : "?"); break;
        case NK_POSTFIX:   printf("(%s)", n->op ? n->op : "?"); break;
        case NK_ASSIGN:    printf("(%s)", n->op ? n->op : "?"); break;
        case NK_MEMBER:    printf("(%s%s)", n->is_arrow ? "->" : ".", n->name ? n->name : "?"); break;
        case NK_FUNCTION:  printf("(%s)", n->name ? n->name : "?"); break;
        case NK_DECL:      printf("(%s)", n->name ? n->name : "?"); break;
        case NK_GLOBAL_VAR: printf("(%s)", n->name ? n->name : "?"); break;
        case NK_STRUCT_DEF: printf("(%s%s)", n->is_union ? "union " : "struct ", n->name ? n->name : "?"); break;
        case NK_ENUM_DEF:  printf("(enum %s)", n->name ? n->name : "?"); break;
        case NK_GOTO:      printf("(%s)", n->name ? n->name : "?"); break;
        case NK_LABEL:     printf("(%s)", n->name ? n->name : "?"); break;
        case NK_ASM:       printf("(%s)", n->is_volatile_asm ? "volatile" : "plain"); break;
        default: break;
    }
    if (n->n_template_params > 0) {
        printf(" template<");
        for (int i = 0; i < n->n_template_params; ++i) {
            if (i) printf(", ");
            printf("%s", n->template_param_names[i] ? n->template_param_names[i] : "?");
        }
        printf(">");
    }
    if (n->n_template_args > 0) {
        printf(" specialize<%d>", n->n_template_args);
    }
    if (n->is_consteval) printf(" consteval");
    else if (n->is_constexpr) printf(" constexpr");
    if (n->linkage_spec) printf(" linkage=\"%s\"", n->linkage_spec);
    printf("\n");

    // Recurse into children
    if (n->left)   ast_dump(n->left,   indent + 1);
    if (n->right)  ast_dump(n->right,  indent + 1);
    if (n->cond)   ast_dump(n->cond,   indent + 1);
    if (n->then_)  ast_dump(n->then_,  indent + 1);
    if (n->else_)  ast_dump(n->else_,  indent + 1);
    if (n->init)   ast_dump(n->init,   indent + 1);
    if (n->update) ast_dump(n->update, indent + 1);
    if (n->body)   ast_dump(n->body,   indent + 1);

    for (int i = 0; i < n->n_params; ++i) ast_dump(n->params[i], indent + 1);
    for (int i = 0; i < n->n_children; ++i) ast_dump(n->children[i], indent + 1);
    for (int i = 0; i < n->n_fields; ++i) ast_dump(n->fields[i], indent + 1);
}


}  // namespace c4c
