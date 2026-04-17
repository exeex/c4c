#include "parser.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace c4c {

namespace {

std::vector<const std::string*> collapse_adjacent_stack_frames(
    const std::vector<std::string>& stack_trace) {
    std::vector<const std::string*> compact;
    compact.reserve(stack_trace.size());
    const std::string* previous = nullptr;
    for (const std::string& fn : stack_trace) {
        if (previous && *previous == fn) continue;
        compact.push_back(&fn);
        previous = &fn;
    }
    return compact;
}

bool should_replace_best_parse_failure(const Parser::ParseFailure& current,
                                       const Parser::ParseFailure& candidate) {
    if (!current.active) return true;
    if (candidate.token_index != current.token_index) {
        const bool wrapper_unwind_prefix =
            candidate.token_index > current.token_index &&
            candidate.token_index <= current.token_index + 1 &&
            candidate.committed == current.committed &&
            candidate.stack_trace.size() < current.stack_trace.size() &&
            !candidate.stack_trace.empty() &&
            std::equal(candidate.stack_trace.begin(), candidate.stack_trace.end(),
                       current.stack_trace.begin());
        if (wrapper_unwind_prefix) return false;
        return candidate.token_index > current.token_index;
    }
    if (candidate.committed != current.committed) {
        return candidate.committed && !current.committed;
    }
    if (candidate.stack_trace.size() != current.stack_trace.size()) {
        return candidate.stack_trace.size() > current.stack_trace.size();
    }
    if (candidate.column != current.column) {
        return candidate.column > current.column;
    }
    return false;
}

bool is_top_level_wrapper_failure(const std::string& function_name) {
    return function_name == "parse_top_level" ||
           function_name == "parse_top_level_parameter_list";
}

bool is_qualified_type_trace_wrapper(const std::string& function_name) {
    return function_name == "parse_next_template_argument" ||
           function_name == "consume_qualified_type_spelling" ||
           function_name == "consume_qualified_type_spelling_with_typename" ||
           function_name == "parse_dependent_typename_specifier";
}

bool is_qualified_type_trace_leaf(const std::string& function_name) {
    return function_name == "try_parse_cpp_scoped_base_type" ||
           function_name == "try_parse_qualified_base_type";
}

bool is_qualified_type_trace_anchor(const std::string& function_name) {
    return is_qualified_type_trace_leaf(function_name) ||
           function_name == "try_parse_template_type_arg";
}

bool is_qualified_type_trace_frame(const std::string& function_name) {
    return is_qualified_type_trace_anchor(function_name) ||
           is_qualified_type_trace_wrapper(function_name);
}

bool is_summary_only_parse_helper(const std::string& function_name) {
    return function_name == "consume_qualified_type_spelling" ||
           function_name == "consume_qualified_type_spelling_with_typename";
}

bool is_param_qualified_type_probe_frame(const std::string& function_name) {
    return function_name == "parse_param" ||
           function_name == "try_parse_cpp_scoped_base_type" ||
           function_name == "try_parse_qualified_base_type" ||
           is_summary_only_parse_helper(function_name);
}

bool stack_contains_qualified_type_trace(
    const std::vector<std::string>& stack_trace) {
    return std::any_of(stack_trace.begin(), stack_trace.end(),
                       [](const std::string& function_name) {
                           return is_qualified_type_trace_anchor(function_name);
                       });
}

bool current_stack_is_prefix_of_best(
    const std::vector<Parser::ParseContextFrame>& current_stack,
    const std::vector<std::string>& best_stack_trace) {
    if (current_stack.size() >= best_stack_trace.size()) return false;
    for (size_t i = 0; i < current_stack.size(); ++i) {
        if (current_stack[i].function_name != best_stack_trace[i]) return false;
    }
    return true;
}

std::vector<std::string> normalize_summary_stack(
    const std::vector<std::string>& stack_trace) {
    std::vector<std::string> normalized;
    normalized.reserve(stack_trace.size());
    for (size_t i = 0; i < stack_trace.size();) {
        if (i + 1 < stack_trace.size() &&
            stack_trace[i] == "parse_next_template_argument" &&
            stack_trace[i + 1] == "try_parse_template_type_arg") {
            normalized.push_back(stack_trace[i]);
            normalized.push_back(stack_trace[i + 1]);
            i += 2;
            while (i + 1 < stack_trace.size() &&
                   stack_trace[i] == "parse_next_template_argument" &&
                   stack_trace[i + 1] == "try_parse_template_type_arg") {
                i += 2;
            }
            continue;
        }
        normalized.push_back(stack_trace[i]);
        ++i;
    }
    return normalized;
}

bool is_top_level_qualified_probe_merge_target(
    const std::string& function_name) {
    return function_name == "parse_next_template_argument" ||
           function_name == "parse_top_level_parameter_list";
}

bool is_identifier_start(char ch) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalpha(uch) || ch == '_';
}

bool is_identifier_continue(char ch) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalnum(uch) || ch == '_';
}

bool uses_symbol_identity(std::string_view name) {
    if (name.empty() || !is_identifier_start(name.front())) return false;
    for (size_t i = 1; i < name.size(); ++i) {
        if (!is_identifier_continue(name[i])) return false;
    }
    return true;
}

std::vector<std::string> merge_leading_top_level_qualified_probe(
    const std::vector<Parser::ParseDebugEvent>& parse_debug_events,
    const std::vector<std::string>& summary_stack) {
    if (summary_stack.size() < 2 || summary_stack.front() != "parse_top_level" ||
        !is_top_level_qualified_probe_merge_target(summary_stack[1])) {
        return summary_stack;
    }

    std::vector<std::string> leading_probe_frames;
    bool inside_top_level = false;
    const std::string& merge_anchor = summary_stack[1];
    for (const Parser::ParseDebugEvent& event : parse_debug_events) {
        if (event.kind != "enter") continue;
        if (event.function_name == "parse_top_level") {
            inside_top_level = true;
            continue;
        }
        if (!inside_top_level) continue;
        if (event.function_name == merge_anchor) break;
        if (!is_qualified_type_trace_frame(event.function_name)) continue;
        if (leading_probe_frames.empty() ||
            leading_probe_frames.back() != event.function_name) {
            leading_probe_frames.push_back(event.function_name);
        }
    }

    if (leading_probe_frames.empty()) return summary_stack;
    if (summary_stack.size() > leading_probe_frames.size() &&
        std::equal(leading_probe_frames.begin(), leading_probe_frames.end(),
                   summary_stack.begin() + 1)) {
        return summary_stack;
    }

    std::vector<std::string> merged;
    merged.reserve(summary_stack.size() + leading_probe_frames.size());
    merged.push_back(summary_stack.front());
    merged.insert(merged.end(), leading_probe_frames.begin(),
                  leading_probe_frames.end());
    merged.insert(merged.end(), summary_stack.begin() + 1, summary_stack.end());
    return merged;
}

std::string quote_debug_lexeme(const std::string& lexeme) {
    std::string escaped;
    escaped.reserve(lexeme.size());
    for (char ch : lexeme) {
        if (ch == '\\' || ch == '\'') escaped.push_back('\\');
        escaped.push_back(ch);
    }
    return escaped;
}

std::string format_debug_token_entry(const Token& token,
                                     std::string_view spelling,
                                     int index,
                                     bool highlight) {
    std::ostringstream oss;
    if (highlight) oss << ">>";
    oss << "[" << index << "] " << token_kind_name(token.kind)
        << " '" << quote_debug_lexeme(std::string(spelling)) << "'";
    return oss.str();
}

const std::string* select_best_parse_summary_leaf(
    const std::vector<std::string>& summary_stack,
    const Parser::ParseFailure& failure) {
    if (!failure.function_name.empty()) {
        for (size_t i = summary_stack.size(); i > 0; --i) {
            if (summary_stack[i - 1] != failure.function_name) continue;
            const bool trailing_param_probe_only =
                failure.function_name == "parse_top_level_parameter_list" &&
                std::all_of(summary_stack.begin() + i, summary_stack.end(),
                            [](const std::string& function_name) {
                                return is_param_qualified_type_probe_frame(
                                    function_name);
                            });
            if (trailing_param_probe_only) {
                return &summary_stack[i - 1];
            }
            const bool trailing_probe_only =
                std::all_of(summary_stack.begin() + i, summary_stack.end(),
                            [](const std::string& function_name) {
                                return is_qualified_type_trace_frame(function_name);
                            });
            if (trailing_probe_only &&
                failure.function_name != "parse_top_level") {
                return &summary_stack[i - 1];
            }
            break;
        }
    }

    for (auto it = summary_stack.rbegin(); it != summary_stack.rend(); ++it) {
        if (!is_summary_only_parse_helper(*it)) return &*it;
    }
    if (!summary_stack.empty()) return &summary_stack.back();
    return nullptr;
}

std::string make_namespace_context_key(int parent_id, const std::string& name) {
    return std::to_string(parent_id) + "::" + name;
}

std::string build_canonical_namespace_name(const char* parent_name,
                                           const std::string& name) {
    std::string canonical = parent_name ? parent_name : "";
    if (!canonical.empty()) canonical += "::";
    canonical += name;
    return canonical;
}

template <typename LookupFn>
std::string resolve_visible_name_from_namespace_stack(
    const std::vector<int>& namespace_stack,
    const std::string& name,
    LookupFn&& lookup) {
    std::string resolved;
    for (int i = static_cast<int>(namespace_stack.size()) - 1; i >= 0; --i) {
        if (lookup(namespace_stack[i], &resolved)) return resolved;
    }
    return name;
}

template <typename FollowFn>
int resolve_namespace_id_from_stack(const std::vector<int>& namespace_stack,
                                    bool is_global_qualified,
                                    FollowFn&& follow) {
    if (is_global_qualified) return follow(0);
    for (int i = static_cast<int>(namespace_stack.size()) - 1; i >= 0; --i) {
        int resolved = follow(namespace_stack[i]);
        if (resolved >= 0) return resolved;
    }
    return follow(0);
}

}  // namespace

Parser::SymbolId Parser::symbol_id_for_token(const Token& token) {
    if (token.kind != TokenKind::Identifier) return kInvalidSymbol;
    return symbol_id_for_token_text(token.text_id);
}

std::string Parser::token_spelling(const Token& token) const {
    if (token.kind == TokenKind::EndOfFile) return {};
    if (token_texts_ && token.text_id != kInvalidText) {
        return std::string(token_texts_->lookup(token.text_id));
    }
    throw std::runtime_error("token spelling requested without valid text_id");
}

void Parser::set_parser_owned_spelling(Token& token, std::string_view spelling) {
    if (!token_texts_) {
        throw std::runtime_error("parser-owned token spelling requested without text table");
    }
    token.text_id = token_texts_->intern(spelling);
}

Token Parser::make_injected_token(const Token& seed, TokenKind kind,
                                  std::string_view spelling) {
    Token token = seed;
    token.kind = kind;
    set_parser_owned_spelling(token, spelling);
    return token;
}

void Parser::populate_qualified_name_symbol_ids(QualifiedNameRef* name) {
    if (!name) return;
    name->qualifier_symbol_ids.clear();
    name->qualifier_symbol_ids.reserve(name->qualifier_segments.size());
    for (const std::string& segment : name->qualifier_segments) {
        name->qualifier_symbol_ids.push_back(
            parser_name_tables_.intern_identifier(segment));
    }
    name->base_symbol_id =
        name->base_name.empty()
            ? kInvalidSymbol
            : parser_name_tables_.intern_identifier(name->base_name);
}

bool Parser::has_typedef_name(std::string_view name) const {
    if (!uses_symbol_identity(name)) {
        return non_atom_typedefs_.count(std::string(name)) > 0;
    }
    return parser_name_tables_.is_typedef(
        parser_name_tables_.find_identifier(name));
}

bool Parser::has_typedef_type(std::string_view name) const {
    if (!uses_symbol_identity(name)) {
        return non_atom_typedef_types_.count(std::string(name)) > 0;
    }
    return parser_name_tables_.has_typedef_type(
        parser_name_tables_.find_identifier(name));
}

const TypeSpec* Parser::find_typedef_type(std::string_view name) const {
    if (!uses_symbol_identity(name)) {
        const auto it = non_atom_typedef_types_.find(std::string(name));
        return it == non_atom_typedef_types_.end() ? nullptr : &it->second;
    }
    return parser_name_tables_.lookup_typedef_type(
        parser_name_tables_.find_identifier(name));
}

bool Parser::has_visible_typedef_type(std::string_view name) const {
    if (has_typedef_type(name)) return true;
    const std::string resolved = resolve_visible_type_name(name);
    return resolved != name && has_typedef_type(resolved);
}

const TypeSpec* Parser::find_visible_typedef_type(std::string_view name) const {
    if (const TypeSpec* type = find_typedef_type(name)) return type;
    const std::string resolved = resolve_visible_type_name(name);
    if (resolved.empty() || resolved == name) return nullptr;
    return find_typedef_type(resolved);
}

TypeSpec Parser::resolve_typedef_type_chain(TypeSpec ts) const {
    for (int depth = 0; depth < 16; ++depth) {
        if (ts.base != TB_TYPEDEF || ts.ptr_level > 0 || ts.array_rank > 0) {
            break;
        }
        if (!ts.tag) break;
        const TypeSpec* next = find_typedef_type(ts.tag);
        if (!next) break;
        const bool is_const = ts.is_const;
        const bool is_volatile = ts.is_volatile;
        ts = *next;
        ts.is_const |= is_const;
        ts.is_volatile |= is_volatile;
    }
    return ts;
}

TypeSpec Parser::resolve_struct_like_typedef_type(TypeSpec ts) const {
    ts = resolve_typedef_type_chain(ts);
    if (ts.base == TB_TYPEDEF && ts.tag) {
        if (const TypeSpec* typedef_type = find_typedef_type(ts.tag)) {
            ts = *typedef_type;
        }
    }
    return ts;
}

bool Parser::are_types_compatible(const TypeSpec& lhs,
                                  const TypeSpec& rhs) const {
    TypeSpec a = resolve_typedef_type_chain(lhs);
    TypeSpec b = resolve_typedef_type_chain(rhs);
    if (a.ptr_level == 0 && a.array_rank == 0) {
        a.is_const = false;
        a.is_volatile = false;
    }
    if (b.ptr_level == 0 && b.array_rank == 0) {
        b.is_const = false;
        b.is_volatile = false;
    }
    if (a.is_vector != b.is_vector) return false;
    if (a.is_vector &&
        (a.vector_lanes != b.vector_lanes || a.vector_bytes != b.vector_bytes)) {
        return false;
    }
    if (a.base != b.base) return false;
    if (a.ptr_level != b.ptr_level) return false;
    if (a.is_const != b.is_const || a.is_volatile != b.is_volatile) {
        return false;
    }
    if (a.array_rank != b.array_rank) return false;
    for (int i = 0; i < a.array_rank; ++i) {
        const long long lhs_dim = a.array_dims[i];
        const long long rhs_dim = b.array_dims[i];
        if (lhs_dim != rhs_dim && lhs_dim != -2 && rhs_dim != -2) return false;
    }
    if (a.base == TB_STRUCT || a.base == TB_UNION || a.base == TB_ENUM) {
        if (!a.tag || !b.tag || std::strcmp(a.tag, b.tag) != 0) return false;
    }
    return true;
}

bool Parser::resolves_to_record_ctor_type(TypeSpec ts) const {
    ts = resolve_struct_like_typedef_type(ts);
    if (ts.base == TB_TYPEDEF && ts.tag &&
        (defined_struct_tags_.count(ts.tag) > 0 ||
         template_struct_defs_.count(ts.tag) > 0)) {
        return true;
    }
    return ts.base == TB_STRUCT || ts.base == TB_UNION;
}

bool Parser::is_user_typedef_name(const std::string& name) const {
    if (!uses_symbol_identity(name)) {
        return non_atom_user_typedefs_.count(name) > 0;
    }
    const SymbolId id = parser_name_tables_.find_identifier(name);
    return id != kInvalidSymbol && parser_name_tables_.user_typedefs.count(id) > 0;
}

bool Parser::has_conflicting_user_typedef_binding(const std::string& name,
                                                  const TypeSpec& type) const {
    const TypeSpec* existing_typedef = find_typedef_type(name);
    return is_user_typedef_name(name) && existing_typedef &&
           !are_types_compatible(*existing_typedef, type);
}

void Parser::register_typedef_name(const std::string& name,
                                   bool is_user_typedef) {
    if (!uses_symbol_identity(name)) {
        non_atom_typedefs_.insert(name);
        if (is_user_typedef) non_atom_user_typedefs_.insert(name);
        return;
    }
    const SymbolId id = parser_name_tables_.intern_identifier(name);
    if (id == kInvalidSymbol) return;
    parser_name_tables_.typedefs.insert(id);
    if (is_user_typedef) parser_name_tables_.user_typedefs.insert(id);
}

void Parser::register_typedef_binding(const std::string& name,
                                      const TypeSpec& type,
                                      bool is_user_typedef) {
    if (!uses_symbol_identity(name)) {
        non_atom_typedefs_.insert(name);
        if (is_user_typedef) non_atom_user_typedefs_.insert(name);
        non_atom_typedef_types_[name] = type;
        return;
    }
    const SymbolId id = parser_name_tables_.intern_identifier(name);
    if (id == kInvalidSymbol) return;
    parser_name_tables_.typedefs.insert(id);
    if (is_user_typedef) parser_name_tables_.user_typedefs.insert(id);
    parser_name_tables_.typedef_types[id] = type;
}

void Parser::unregister_typedef_binding(const std::string& name) {
    if (!uses_symbol_identity(name)) {
        non_atom_typedefs_.erase(name);
        non_atom_user_typedefs_.erase(name);
        non_atom_typedef_types_.erase(name);
        return;
    }
    const SymbolId id = parser_name_tables_.find_identifier(name);
    if (id == kInvalidSymbol) return;
    parser_name_tables_.typedefs.erase(id);
    parser_name_tables_.user_typedefs.erase(id);
    parser_name_tables_.typedef_types.erase(id);
}

void Parser::register_synthesized_typedef_binding(const std::string& name) {
    TypeSpec synthesized_ts{};
    synthesized_ts.array_size = -1;
    synthesized_ts.inner_rank = -1;
    synthesized_ts.base = TB_TYPEDEF;
    synthesized_ts.tag = arena_.strdup(name.c_str());
    register_typedef_binding(name, synthesized_ts, false);
}

void Parser::register_tag_type_binding(const std::string& name,
                                       TypeBase base,
                                       const char* tag,
                                       TypeBase enum_underlying_base) {
    if (name.empty() || !tag || !tag[0]) return;

    TypeSpec tagged_ts{};
    tagged_ts.array_size = -1;
    tagged_ts.array_rank = 0;
    tagged_ts.base = base;
    tagged_ts.tag = tag;
    if (base == TB_ENUM) {
        tagged_ts.inner_rank = -1;
        for (int i = 0; i < 8; ++i) tagged_ts.array_dims[i] = -1;
        tagged_ts.enum_underlying_base = enum_underlying_base;
    }
    register_typedef_binding(name, tagged_ts, false);
}

void Parser::cache_typedef_type(const std::string& name, const TypeSpec& type) {
    if (!uses_symbol_identity(name)) {
        non_atom_typedef_types_[name] = type;
        return;
    }
    const SymbolId id = parser_name_tables_.intern_identifier(name);
    if (id == kInvalidSymbol) return;
    parser_name_tables_.typedef_types[id] = type;
}

void Parser::register_struct_member_typedef_binding(
    const std::string& scoped_name, const TypeSpec& type) {
    struct_typedefs_[scoped_name] = type;
    register_typedef_binding(scoped_name, type, false);
}

bool Parser::has_var_type(const std::string& name) const {
    if (!uses_symbol_identity(name)) {
        return non_atom_var_types_.count(name) > 0;
    }
    const SymbolId id = parser_name_tables_.find_identifier(name);
    return id != kInvalidSymbol && parser_name_tables_.var_types.count(id) > 0;
}

const TypeSpec* Parser::find_var_type(const std::string& name) const {
    if (!uses_symbol_identity(name)) {
        const auto it = non_atom_var_types_.find(name);
        return it == non_atom_var_types_.end() ? nullptr : &it->second;
    }
    const SymbolId id = parser_name_tables_.find_identifier(name);
    if (id == kInvalidSymbol) return nullptr;
    const auto it = parser_name_tables_.var_types.find(id);
    if (it == parser_name_tables_.var_types.end()) return nullptr;
    return &it->second;
}

const TypeSpec* Parser::find_visible_var_type(const std::string& name) const {
    if (const TypeSpec* type = find_var_type(name)) return type;
    const std::string resolved = resolve_visible_value_name(name);
    if (resolved.empty() || resolved == name) return nullptr;
    return find_var_type(resolved);
}

void Parser::register_var_type_binding(const std::string& name,
                                       const TypeSpec& type) {
    if (!uses_symbol_identity(name)) {
        non_atom_var_types_[name] = type;
        return;
    }
    const SymbolId id = parser_name_tables_.intern_identifier(name);
    if (id == kInvalidSymbol) return;
    parser_name_tables_.var_types[id] = type;
}

bool Parser::has_known_fn_name(const std::string& name) const {
    return known_fn_names_.count(name) > 0;
}

void Parser::register_known_fn_name(const std::string& name) {
    known_fn_names_.insert(name);
}

Parser::ParseContextGuard::ParseContextGuard(
    Parser* parser_in, const char* function_name)
    : parser(parser_in) {
    if (parser) parser->push_parse_context(function_name);
}

Parser::ParseContextGuard::~ParseContextGuard() {
    if (parser) parser->pop_parse_context();
}

Parser::Parser(std::vector<Token> tokens, Arena& arena,
               TextTable* token_texts,
               FileTable* token_files,
               SourceProfile source_profile,
               const std::string& source_file)
    : tokens_(std::move(tokens)), pos_(0), arena_(arena), source_profile_(source_profile),
      source_file_(source_file), token_texts_(token_texts), token_files_(token_files),
      anon_counter_(0), had_error_(false), parsing_top_level_context_(false),
      last_enum_def_(nullptr) {
    parser_symbols_.attach_text_table(token_texts_);
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
    for (int i = 0; seed[i]; ++i) register_typedef_name(seed[i], false);

    // Seed grouped typedef type storage for well-known names so they resolve
    // to correct LLVM types.
    // instead of the TB_TYPEDEF fallback (which emits i32).
    {
        TypeSpec va_ts{};
        va_ts.array_size = -1;
        va_ts.array_rank = 0;
        va_ts.is_ptr_to_array = false;
        va_ts.base = TB_VA_LIST;
        cache_typedef_type("va_list", va_ts);
        cache_typedef_type("__va_list", va_ts);
        cache_typedef_type("__builtin_va_list", va_ts);
        cache_typedef_type("__gnuc_va_list", va_ts);

        // size_t / uintptr_t etc. → u64 on 64-bit platforms
        TypeSpec u64_ts{};
        u64_ts.array_size = -1;
        u64_ts.array_rank = 0;
        u64_ts.is_ptr_to_array = false;
        u64_ts.base = TB_ULONGLONG;  // 64-bit unsigned
        cache_typedef_type("size_t", u64_ts);
        cache_typedef_type("uintptr_t", u64_ts);
        cache_typedef_type("uintmax_t", u64_ts);
        cache_typedef_type("uint64_t", u64_ts);
        cache_typedef_type("uint_least64_t", u64_ts);
        cache_typedef_type("uint_fast64_t", u64_ts);

        TypeSpec i64_ts{};
        i64_ts.array_size = -1;
        i64_ts.array_rank = 0;
        i64_ts.is_ptr_to_array = false;
        i64_ts.base = TB_LONGLONG;
        cache_typedef_type("ssize_t", i64_ts);
        cache_typedef_type("ptrdiff_t", i64_ts);
        cache_typedef_type("intptr_t", i64_ts);
        cache_typedef_type("intmax_t", i64_ts);
        cache_typedef_type("int64_t", i64_ts);
        cache_typedef_type("int_least64_t", i64_ts);
        cache_typedef_type("int_fast64_t", i64_ts);
        cache_typedef_type("off_t", i64_ts);

        TypeSpec u32_ts{};
        u32_ts.array_size = -1;
        u32_ts.array_rank = 0;
        u32_ts.is_ptr_to_array = false;
        u32_ts.base = TB_UINT;
        cache_typedef_type("uint32_t", u32_ts);
        cache_typedef_type("uint_least32_t", u32_ts);
        cache_typedef_type("uint_fast32_t", u32_ts);

        TypeSpec i32_ts{};
        i32_ts.array_size = -1;
        i32_ts.array_rank = 0;
        i32_ts.is_ptr_to_array = false;
        i32_ts.base = TB_INT;
        cache_typedef_type("int32_t", i32_ts);
        cache_typedef_type("int_least32_t", i32_ts);
        cache_typedef_type("int_fast32_t", i32_ts);

        TypeSpec u16_ts{};
        u16_ts.array_size = -1;
        u16_ts.array_rank = 0;
        u16_ts.is_ptr_to_array = false;
        u16_ts.base = TB_USHORT;
        cache_typedef_type("uint16_t", u16_ts);
        cache_typedef_type("uint_least16_t", u16_ts);
        cache_typedef_type("uint_fast16_t", u16_ts);

        TypeSpec i16_ts{};
        i16_ts.array_size = -1;
        i16_ts.array_rank = 0;
        i16_ts.is_ptr_to_array = false;
        i16_ts.base = TB_SHORT;
        cache_typedef_type("int16_t", i16_ts);
        cache_typedef_type("int_least16_t", i16_ts);
        cache_typedef_type("int_fast16_t", i16_ts);

        TypeSpec u8_ts{};
        u8_ts.array_size = -1;
        u8_ts.array_rank = 0;
        u8_ts.is_ptr_to_array = false;
        u8_ts.base = TB_UCHAR;
        cache_typedef_type("uint8_t", u8_ts);
        cache_typedef_type("uint_least8_t", u8_ts);
        cache_typedef_type("uint_fast8_t", u8_ts);

        TypeSpec i8_ts{};
        i8_ts.array_size = -1;
        i8_ts.array_rank = 0;
        i8_ts.is_ptr_to_array = false;
        i8_ts.base = TB_SCHAR;
        cache_typedef_type("int8_t", i8_ts);
        cache_typedef_type("int_least8_t", i8_ts);
        cache_typedef_type("int_fast8_t", i8_ts);

        // wchar_t on macOS/ARM64 is i32
        TypeSpec wchar_ts{};
        wchar_ts.array_size = -1;
        wchar_ts.array_rank = 0;
        wchar_ts.is_ptr_to_array = false;
        wchar_ts.base = TB_INT;
        cache_typedef_type("wchar_t", wchar_ts);
        cache_typedef_type("wint_t", wchar_ts);

        TypeSpec true_ts{};
        true_ts.array_size = -1;
        true_ts.array_rank = 0;
        true_ts.is_ptr_to_array = false;
        true_ts.base = TB_STRUCT;
        true_ts.tag = arena_.strdup("__true_type");
        cache_typedef_type("__true_type", true_ts);
        cache_typedef_type("std::__true_type", true_ts);
        cache_typedef_type("std::__8::__true_type", true_ts);

        TypeSpec false_ts{};
        false_ts.array_size = -1;
        false_ts.array_rank = 0;
        false_ts.is_ptr_to_array = false;
        false_ts.base = TB_STRUCT;
        false_ts.tag = arena_.strdup("__false_type");
        cache_typedef_type("__false_type", false_ts);
        cache_typedef_type("std::__false_type", false_ts);
        cache_typedef_type("std::__8::__false_type", false_ts);
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

void Parser::handle_pragma_exec(const std::string& args) {
    if (args == "host") {
        execution_domain_ = ExecutionDomain::Host;
    } else if (args == "device") {
        execution_domain_ = ExecutionDomain::Device;
    }
}

void Parser::set_parser_debug(bool enabled) {
    parser_debug_channels_ = enabled ? ParseDebugAll : ParseDebugNone;
}

void Parser::set_parser_debug_channels(unsigned channels) {
    parser_debug_channels_ = channels;
}

bool Parser::parser_debug_enabled() const {
    return parser_debug_channels_ != ParseDebugNone;
}

bool Parser::parse_debug_event_visible(const char* kind) const {
    if (!parser_debug_enabled()) return false;
    if (!kind) return (parser_debug_channels_ & ParseDebugGeneral) != 0;
    if (std::strncmp(kind, "tentative_", 10) == 0) {
        return (parser_debug_channels_ & ParseDebugTentative) != 0;
    }
    if (std::strncmp(kind, "injected_parse_", 15) == 0) {
        return (parser_debug_channels_ & ParseDebugInjected) != 0;
    }
    return (parser_debug_channels_ & ParseDebugGeneral) != 0;
}

void Parser::clear_parse_debug_state() {
    parse_context_stack_.clear();
    parse_debug_events_.clear();
    best_parse_failure_ = ParseFailure{};
    best_parse_stack_token_index_ = -1;
    best_parse_stack_trace_.clear();
}

void Parser::reset_parse_debug_progress() {
    parse_debug_started_at_ = {};
    parse_debug_last_progress_at_ = {};
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
    note_parse_debug_event_for(kind, nullptr, detail);
}

void Parser::note_parse_debug_event_for(const char* kind,
                                        const char* function_name,
                                        const char* detail) {
    if (!parser_debug_enabled()) return;

    const auto now = std::chrono::steady_clock::now();
    if (parse_debug_started_at_ == std::chrono::steady_clock::time_point{}) {
        parse_debug_started_at_ = now;
    }
    if (parse_debug_last_progress_at_ == std::chrono::steady_clock::time_point{}) {
        parse_debug_last_progress_at_ = now;
    }

    ParseDebugEvent event;
    event.kind = kind ? kind : "";
    event.detail = detail ? detail : "";
    event.token_index = !at_end() ? pos_ : (pos_ > 0 ? pos_ - 1 : -1);
    event.line = !at_end() ? cur().line : (pos_ > 0 ? tokens_[pos_ - 1].line : 1);
    event.column = !at_end() ? cur().column : 1;
    if (function_name && function_name[0]) {
        event.function_name = function_name;
    } else if (!parse_context_stack_.empty()) {
        event.function_name = parse_context_stack_.back().function_name;
    }
    parse_debug_events_.push_back(std::move(event));
    if (static_cast<int>(parse_debug_events_.size()) > max_parse_debug_events_) {
        parse_debug_events_.erase(parse_debug_events_.begin());
    }
    if (kind &&
        (std::strncmp(kind, "tentative_", 10) == 0 ||
         std::strncmp(kind, "injected_parse_", 15) == 0)) {
        return;
    }
    if (!parse_context_stack_.empty()) {
        const int token_index = parse_debug_events_.back().token_index;
        const int token_delta = token_index - best_parse_stack_token_index_;
        const std::string current_function = parse_context_stack_.back().function_name;
        const bool preserving_wrapper_prefix_snapshot =
            token_delta > 0 &&
            current_stack_is_prefix_of_best(parse_context_stack_,
                                            best_parse_stack_trace_);
        const bool preserving_qualified_type_probe_snapshot =
            token_delta > 0 &&
            (is_top_level_wrapper_failure(current_function) ||
             is_qualified_type_trace_wrapper(current_function)) &&
            stack_contains_qualified_type_trace(best_parse_stack_trace_);
        const bool replace_snapshot =
            best_parse_stack_token_index_ < 0 ||
            (!(preserving_wrapper_prefix_snapshot ||
               preserving_qualified_type_probe_snapshot) &&
             token_delta > 1) ||
            (token_delta > 0 &&
             parse_context_stack_.size() >= best_parse_stack_trace_.size()) ||
            (token_delta == 0 &&
             parse_context_stack_.size() > best_parse_stack_trace_.size());
        if (replace_snapshot) {
            best_parse_stack_token_index_ = token_index;
            best_parse_stack_trace_.clear();
            best_parse_stack_trace_.reserve(parse_context_stack_.size());
            for (const ParseContextFrame& frame : parse_context_stack_) {
                best_parse_stack_trace_.push_back(frame.function_name);
            }
        }
    }

    maybe_emit_parse_debug_progress();
}

void Parser::maybe_emit_parse_debug_progress() {
    if (!parser_debug_enabled()) return;
    if (parse_debug_events_.empty()) return;

    const auto now = std::chrono::steady_clock::now();
    const auto interval =
        std::chrono::milliseconds(parse_debug_progress_interval_ms_);
    if (now - parse_debug_last_progress_at_ < interval) return;

    const ParseDebugEvent& event = parse_debug_events_.back();
    const auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - parse_debug_started_at_)
            .count();
    fprintf(stderr,
            "[pdebug] progress elapsed_ms=%lld token_index=%d line=%d col=%d",
            static_cast<long long>(elapsed_ms), event.token_index, event.line,
            event.column);
    if (event.token_index >= 0 &&
        event.token_index < static_cast<int>(tokens_.size()) &&
        token_files_ && tokens_[event.token_index].file_id != kInvalidFile) {
        fprintf(stderr, " file=%s",
                std::string(token_files_->lookup(tokens_[event.token_index].file_id)).c_str());
    }
    if (!event.function_name.empty()) {
        fprintf(stderr, " fn=%s", event.function_name.c_str());
    }
    if (event.token_index >= 0 &&
        event.token_index < static_cast<int>(tokens_.size())) {
        fprintf(stderr, " token_kind=%s token='%s'",
                token_kind_name(tokens_[event.token_index].kind),
                std::string(token_spelling(tokens_[event.token_index])).c_str());
    }
    fprintf(stderr, "\n");
    fflush(stderr);

    parse_debug_last_progress_at_ = now;
}

void Parser::note_tentative_parse_event(TentativeParseMode mode,
                                        const char* kind,
                                        int start_pos,
                                        int end_pos) {
    int* counter = nullptr;
    switch (mode) {
        case TentativeParseMode::Heavy:
            if (std::strcmp(kind, "tentative_enter") == 0)
                counter = &tentative_parse_stats_.heavy_enter;
            else if (std::strcmp(kind, "tentative_commit") == 0)
                counter = &tentative_parse_stats_.heavy_commit;
            else if (std::strcmp(kind, "tentative_rollback") == 0)
                counter = &tentative_parse_stats_.heavy_rollback;
            break;
        case TentativeParseMode::Lite:
            if (std::strcmp(kind, "tentative_enter") == 0)
                counter = &tentative_parse_stats_.lite_enter;
            else if (std::strcmp(kind, "tentative_commit") == 0)
                counter = &tentative_parse_stats_.lite_commit;
            else if (std::strcmp(kind, "tentative_rollback") == 0)
                counter = &tentative_parse_stats_.lite_rollback;
            break;
    }
    if (counter) ++(*counter);

    const std::string detail =
        format_tentative_parse_detail(mode, kind, start_pos, end_pos);
    note_parse_debug_event(kind, detail.c_str());
}

void Parser::note_parse_failure(const char* expected,
                                const char* detail,
                                bool committed) {
    ParseFailure failure;
    failure.active = true;
    failure.committed = committed;
    failure.token_index = !at_end() ? pos_ : (pos_ > 0 ? pos_ - 1 : -1);
    failure.token_kind = !at_end() ? cur().kind : TokenKind::EndOfFile;
    failure.line = !at_end() ? cur().line : (pos_ > 0 ? tokens_[pos_ - 1].line : 1);
    failure.column = !at_end() ? cur().column : 1;
    failure.expected = expected ? expected : "";
    failure.got = at_end() ? "<eof>" : token_spelling(cur());
    failure.detail = detail ? detail : "";
    if (!parse_context_stack_.empty()) {
        failure.function_name = parse_context_stack_.back().function_name;
    }
    for (const ParseContextFrame& frame : parse_context_stack_) {
        failure.stack_trace.push_back(frame.function_name);
    }

    if (should_replace_best_parse_failure(best_parse_failure_, failure)) {
        best_parse_failure_ = std::move(failure);
    }

    note_parse_debug_event(committed ? "fail" : "soft_fail", detail);
}

void Parser::note_parse_failure_message(const char* detail, bool committed) {
    note_parse_failure(nullptr, detail, committed);
}

std::vector<std::string> Parser::best_debug_summary_stack() const {
    const bool top_level_wrapper_failure =
        is_top_level_wrapper_failure(best_parse_failure_.function_name);
    std::vector<std::string> summary_stack;
    if (best_parse_stack_trace_.size() > best_parse_failure_.stack_trace.size() &&
        (best_parse_stack_token_index_ >= best_parse_failure_.token_index ||
         top_level_wrapper_failure)) {
        std::vector<std::string> candidate = best_parse_stack_trace_;
        if (best_parse_failure_.function_name == "parse_top_level" &&
            !candidate.empty() &&
            is_qualified_type_trace_leaf(candidate.back())) {
            candidate.pop_back();
        }
        size_t common_prefix = 0;
        while (common_prefix < candidate.size() &&
               common_prefix < best_parse_failure_.stack_trace.size() &&
               candidate[common_prefix] ==
                   best_parse_failure_.stack_trace[common_prefix]) {
            ++common_prefix;
        }
        if (top_level_wrapper_failure && common_prefix > 0 &&
            common_prefix < candidate.size() &&
            best_parse_failure_.function_name ==
                "parse_top_level_parameter_list") {
            std::vector<std::string> merged = candidate;
            merged.insert(merged.end(),
                          best_parse_failure_.stack_trace.begin() + common_prefix,
                          best_parse_failure_.stack_trace.end());
            summary_stack = normalize_summary_stack(merged);
            return merge_leading_top_level_qualified_probe(parse_debug_events_,
                                                           summary_stack);
        }
        if (!best_parse_failure_.function_name.empty() &&
            std::find(candidate.begin(),
                      candidate.end(),
                      best_parse_failure_.function_name) !=
                candidate.end()) {
            summary_stack = normalize_summary_stack(candidate);
            return merge_leading_top_level_qualified_probe(parse_debug_events_,
                                                           summary_stack);
        }
        if (best_parse_failure_.function_name == "parse_top_level" &&
            !candidate.empty()) {
            summary_stack = normalize_summary_stack(candidate);
            return merge_leading_top_level_qualified_probe(parse_debug_events_,
                                                           summary_stack);
        }
    }
    summary_stack = normalize_summary_stack(best_parse_failure_.stack_trace);
    return merge_leading_top_level_qualified_probe(parse_debug_events_,
                                                   summary_stack);
}

std::string Parser::format_best_parse_failure() const {
    if (!best_parse_failure_.active) return {};

    std::ostringstream oss;
    const std::vector<std::string> summary_stack = best_debug_summary_stack();
    const std::string* summary_leaf =
        select_best_parse_summary_leaf(summary_stack, best_parse_failure_);
    if (summary_leaf && !summary_leaf->empty()) {
        oss << "parse_fn=" << *summary_leaf;
    } else if (!best_parse_failure_.function_name.empty()) {
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
    if (parser_debug_enabled()) {
        if (oss.tellp() > 0) oss << " ";
        oss << "token_index=" << best_parse_failure_.token_index
            << " token_kind=" << token_kind_name(best_parse_failure_.token_kind)
            << " token_window=\""
            << format_parse_failure_token_window(best_parse_failure_) << "\"";
    }
    return oss.str();
}

std::string Parser::format_parse_failure_token_window(
    const ParseFailure& failure) const {
    if (tokens_.empty()) return {};

    const int center =
        failure.token_index >= 0
            ? std::min(failure.token_index, static_cast<int>(tokens_.size()) - 1)
            : static_cast<int>(tokens_.size()) - 1;
    const int start = std::max(0, center - 2);
    const int end =
        std::min(static_cast<int>(tokens_.size()) - 1, center + 2);

    std::ostringstream oss;
    for (int i = start; i <= end; ++i) {
        if (i > start) oss << " | ";
        oss << format_debug_token_entry(tokens_[i], token_spelling(tokens_[i]),
                                        i, i == center);
    }
    return oss.str();
}

std::string Parser::format_tentative_parse_detail(TentativeParseMode mode,
                                                  const char* kind,
                                                  int start_pos,
                                                  int end_pos) const {
    std::ostringstream oss;
    oss << "mode="
        << (mode == TentativeParseMode::Heavy ? "heavy" : "lite")
        << " start=" << start_pos
        << " end=" << end_pos;
    if (kind && kind[0]) {
        int count = 0;
        if (mode == TentativeParseMode::Heavy) {
            if (std::strcmp(kind, "tentative_enter") == 0)
                count = tentative_parse_stats_.heavy_enter;
            else if (std::strcmp(kind, "tentative_commit") == 0)
                count = tentative_parse_stats_.heavy_commit;
            else if (std::strcmp(kind, "tentative_rollback") == 0)
                count = tentative_parse_stats_.heavy_rollback;
        } else {
            if (std::strcmp(kind, "tentative_enter") == 0)
                count = tentative_parse_stats_.lite_enter;
            else if (std::strcmp(kind, "tentative_commit") == 0)
                count = tentative_parse_stats_.lite_commit;
            else if (std::strcmp(kind, "tentative_rollback") == 0)
                count = tentative_parse_stats_.lite_rollback;
        }
        if (count > 0) oss << " count=" << count;
    }
    return oss.str();
}

void Parser::dump_parse_debug_trace() const {
    if (!parser_debug_enabled() || parse_debug_events_.empty()) return;
    bool has_visible_event = false;
    for (const ParseDebugEvent& event : parse_debug_events_) {
        if (parse_debug_event_visible(event.kind.c_str())) {
            has_visible_event = true;
            break;
        }
    }
    if (!has_visible_event) return;
    fprintf(stderr, "%s:%d:%d: note: parser debug trace follows\n",
            best_parse_failure_.active ? diag_file_at(best_parse_failure_.token_index)
                                       : source_file_.c_str(),
            best_parse_failure_.active ? best_parse_failure_.line : 1,
            best_parse_failure_.active ? best_parse_failure_.column : 1);
    for (const ParseDebugEvent& event : parse_debug_events_) {
        if (!parse_debug_event_visible(event.kind.c_str())) continue;
        fprintf(stderr, "[pdebug] kind=%s",
                event.kind.c_str());
        if (!event.function_name.empty())
            fprintf(stderr, " fn=%s", event.function_name.c_str());
        fprintf(stderr, " line=%d col=%d", event.line, event.column);
        if (!event.detail.empty()) fprintf(stderr, " detail=\"%s\"", event.detail.c_str());
        fprintf(stderr, "\n");
    }
    const std::vector<std::string> summary_stack = best_debug_summary_stack();
    if (best_parse_failure_.active && !summary_stack.empty()) {
        const std::vector<const std::string*> compact_stack =
            collapse_adjacent_stack_frames(summary_stack);
        fprintf(stderr, "[pdebug] stack:");
        for (const std::string* fn : compact_stack) {
            fprintf(stderr, " -> %s", fn->c_str());
        }
        fprintf(stderr, "\n");
    }
}

const char* Parser::diag_file_at(int token_index) const {
    if (token_index >= 0 && token_index < static_cast<int>(tokens_.size()) &&
        token_files_ && tokens_[token_index].file_id != kInvalidFile) {
        const std::string file = std::string(token_files_->lookup(tokens_[token_index].file_id));
        if (!file.empty()) return arena_.strdup(file);
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
        static Token eof = [] {
            Token token{};
            token.kind = TokenKind::EndOfFile;
            return token;
        }();
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

bool Parser::peek_next_is(TokenKind k) const {
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
    return resolve_visible_name_from_namespace_stack(
        namespace_stack_, name, [&](int context_id, std::string* resolved) {
            auto alias_it = using_value_aliases_.find(context_id);
            if (alias_it != using_value_aliases_.end()) {
                auto value_it = alias_it->second.find(name);
                if (value_it != alias_it->second.end()) {
                    *resolved = value_it->second;
                    return true;
                }
            }
            return lookup_value_in_context(context_id, name, resolved);
        });
}

std::string Parser::resolve_visible_type_name(std::string_view name) const {
    const std::string spelled(name);
    return resolve_visible_name_from_namespace_stack(
        namespace_stack_, spelled,
        [&](int context_id, std::string* resolved) {
            auto alias_it = using_value_aliases_.find(context_id);
            if (alias_it != using_value_aliases_.end()) {
                auto value_it = alias_it->second.find(spelled);
                if (value_it != alias_it->second.end() &&
                    has_typedef_type(value_it->second)) {
                    *resolved = value_it->second;
                    return true;
                }
            }
            return lookup_type_in_context(context_id, spelled, resolved);
        });
}

std::string Parser::resolve_visible_concept_name(const std::string& name) const {
    return resolve_visible_name_from_namespace_stack(
        namespace_stack_, name, [&](int context_id, std::string* resolved) {
            return lookup_concept_in_context(context_id, name, resolved);
        });
}

bool Parser::is_concept_name(const std::string& name) const {
    if (name.empty()) return false;
    if (concept_names_.count(name) > 0) return true;
    return concept_names_.count(resolve_visible_concept_name(name)) > 0;
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
    const std::string key = make_namespace_context_key(parent_id, name);
    auto it = named_namespace_contexts_.find(key);
    if (it != named_namespace_contexts_.end()) return it->second;

    const NamespaceContext& parent = namespace_contexts_[parent_id];
    std::string canonical =
        build_canonical_namespace_name(parent.canonical_name, name);

    const int id = static_cast<int>(namespace_contexts_.size());
    namespace_contexts_.push_back(
        NamespaceContext{id, parent_id, false, arena_.strdup(name.c_str()),
                         arena_.strdup(canonical.c_str())});
    named_namespace_contexts_[key] = id;
    return id;
}

int Parser::create_anonymous_namespace_context(int parent_id) {
    const NamespaceContext& parent = namespace_contexts_[parent_id];
    std::string canonical = build_canonical_namespace_name(
        parent.canonical_name, "__anon_ns_" + std::to_string(anon_counter_++));

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
            const std::string key = make_namespace_context_key(context_id, segment);
            auto it = named_namespace_contexts_.find(key);
            if (it == named_namespace_contexts_.end()) return -1;
            context_id = it->second;
        }
        return context_id;
    };

    return resolve_namespace_id_from_stack(namespace_stack_,
                                           name.is_global_qualified,
                                           follow_path);
}

int Parser::resolve_namespace_name(const QualifiedNameRef& name) const {
    auto follow_name = [&](int start_id) -> int {
        int context_id = start_id;
        for (const std::string& segment : name.qualifier_segments) {
            const std::string key = make_namespace_context_key(context_id, segment);
            auto it = named_namespace_contexts_.find(key);
            if (it == named_namespace_contexts_.end()) return -1;
            context_id = it->second;
        }
        const std::string final_key =
            make_namespace_context_key(context_id, name.base_name);
        auto it = named_namespace_contexts_.find(final_key);
        if (it == named_namespace_contexts_.end()) return -1;
        return it->second;
    };

    return resolve_namespace_id_from_stack(namespace_stack_,
                                           name.is_global_qualified,
                                           follow_name);
}

bool Parser::lookup_value_in_context(int context_id, const std::string& name,
                                     std::string* resolved) const {
    const std::string candidate = canonical_name_in_context(context_id, name);
    if (has_var_type(candidate) || has_known_fn_name(candidate)) {
        *resolved = candidate;
        return true;
    }
    if (context_id == 0 && (has_var_type(name) || has_known_fn_name(name))) {
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
    if (has_typedef_type(candidate)) {
        *resolved = candidate;
        return true;
    }
    if (context_id == 0 && has_typedef_type(name)) {
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

bool Parser::lookup_concept_in_context(int context_id, const std::string& name,
                                       std::string* resolved) const {
    const std::string candidate = canonical_name_in_context(context_id, name);
    if (concept_names_.count(candidate)) {
        *resolved = candidate;
        return true;
    }
    if (context_id == 0 && concept_names_.count(name)) {
        *resolved = name;
        return true;
    }

    auto anon_it = anonymous_namespace_children_.find(context_id);
    if (anon_it != anonymous_namespace_children_.end()) {
        for (int anon_id : anon_it->second) {
            if (lookup_concept_in_context(anon_id, name, resolved)) return true;
        }
    }

    auto using_it = using_namespace_contexts_.find(context_id);
    if (using_it != using_namespace_contexts_.end()) {
        for (int imported_id : using_it->second) {
            if (lookup_concept_in_context(imported_id, name, resolved)) return true;
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
    result.base_name = std::string(token_spelling(tokens_[p]));
    ++p;
    while (p + 1 < static_cast<int>(tokens_.size()) &&
           tokens_[p].kind == TokenKind::ColonColon &&
           tokens_[p + 1].kind == TokenKind::Identifier) {
        result.qualifier_segments.push_back(result.base_name);
        result.base_name = std::string(token_spelling(tokens_[p + 1]));
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
    populate_qualified_name_symbol_ids(&result);
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
            << token_spelling(cur()) << "' at line " << cur().line;
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
        token_mutations_.push_back({pos_, tokens_[pos_]});
        tokens_[pos_].kind = TokenKind::Greater;
        set_parser_owned_spelling(tokens_[pos_], ">");
        return true;
    }

    if (check(TokenKind::GreaterEqual)) {
        // Consume one > and leave = in the token stream.
        token_mutations_.push_back({pos_, tokens_[pos_]});
        tokens_[pos_].kind = TokenKind::Assign;
        set_parser_owned_spelling(tokens_[pos_], "=");
        return true;
    }

    if (check(TokenKind::GreaterGreaterAssign)) {
        // Consume one > and leave >= in the token stream.
        token_mutations_.push_back({pos_, tokens_[pos_]});
        tokens_[pos_].kind = TokenKind::GreaterEqual;
        set_parser_owned_spelling(tokens_[pos_], ">=");
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
            << token_spelling(cur()) << "' at line " << cur().line;
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
    reset_parse_debug_progress();

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
                    std::string(token_spelling(cur())).c_str());
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
    n->column = 1;
    n->file = arena_.strdup(source_file_);
    int loc_index = -1;
    if (pos_ >= 0 && pos_ < static_cast<int>(tokens_.size()) &&
        tokens_[pos_].line == line) {
        loc_index = pos_;
    } else if (pos_ > 0 && pos_ - 1 < static_cast<int>(tokens_.size()) &&
               tokens_[pos_ - 1].line == line) {
        loc_index = pos_ - 1;
    } else if (pos_ >= 0 && pos_ < static_cast<int>(tokens_.size())) {
        loc_index = pos_;
    } else if (!tokens_.empty()) {
        loc_index = static_cast<int>(tokens_.size()) - 1;
    }
    if (loc_index >= 0 && loc_index < static_cast<int>(tokens_.size())) {
        n->column = tokens_[loc_index].column;
        if (token_files_ && tokens_[loc_index].file_id != kInvalidFile) {
            const std::string file =
                std::string(token_files_->lookup(tokens_[loc_index].file_id));
            if (!file.empty()) n->file = arena_.strdup(file);
        }
    }
    n->ival = -1;  // -1 = not a bitfield (for struct field declarations)
    n->builtin_id = BuiltinId::Unknown;
    n->type.array_size = -1;
    n->type.array_rank = 0;
    for (int i = 0; i < 8; ++i) n->type.array_dims[i] = -1;
    n->type.inner_rank = -1;
    n->type.is_ptr_to_array = false;
    n->execution_domain = ExecutionDomain::Host;
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
        case OP_SPACESHIP: return "operator_spaceship";
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
        case NK_LAMBDA:       return "Lambda";
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
        case NK_STATIC_ASSERT:return "StaticAssert";
        case NK_DECL:         return "Decl";
        case NK_FUNCTION:     return "Function";
        case NK_GLOBAL_VAR:   return "GlobalVar";
        case NK_STRUCT_DEF:   return "StructDef";
        case NK_ENUM_DEF:     return "EnumDef";
        case NK_OFFSETOF:     return "Offsetof";
        case NK_PRAGMA_WEAK:  return "PragmaWeak";
        case NK_PRAGMA_EXEC:  return "PragmaExec";
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
        case NK_PRAGMA_EXEC: printf("(%s)", n->name ? n->name : "?"); break;
        case NK_STRUCT_DEF: printf("(%s%s)", n->is_union ? "union " : "struct ", n->name ? n->name : "?"); break;
        case NK_ENUM_DEF:  printf("(enum %s)", n->name ? n->name : "?"); break;
        case NK_GOTO:      printf("(%s)", n->name ? n->name : "?"); break;
        case NK_LABEL:     printf("(%s)", n->name ? n->name : "?"); break;
        case NK_ASM:       printf("(%s)", n->is_volatile_asm ? "volatile" : "plain"); break;
        case NK_LAMBDA: {
            const char* capture = "[]";
            if (n->lambda_capture_default == LCD_BY_REFERENCE) capture = "[&]";
            else if (n->lambda_capture_default == LCD_BY_COPY) capture = "[=]";
            printf("(%s", capture);
            if (n->lambda_has_parameter_list) printf(",()");
            printf(")");
            break;
        }
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
