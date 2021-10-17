#include <clang-c/Index.h>
#include <clang-c/CXCompilationDatabase.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>
#include <iostream>
#include <fstream>
#include <sstream>
#include "indexer.h"

std::string to_string(CXString str) noexcept
{
    std::string ans(clang_getCString(str));
    clang_disposeString(str);
    return ans;
}

std::string convert_to_full_name(std::string &USR, CXCursor cursor) noexcept
{
    if (USR[3] == 'F') {
        return to_string(clang_getCursorDisplayName(cursor));
    }
    std::stringstream ss;
    for (int i = 5;; ++i) {
        if (USR[i] == '#') {
            break;
        } else if (USR[i] == '@') {
            if (USR[i + 1] == 'F') {
                break;
            }
            i += 3;
            ss << "::";
        }
        ss << USR[i];
    }
    ss << "::" << to_string(clang_getCursorDisplayName(cursor));
    return ss.str();
}

bool has_function_body(CXCursorKind kind) {
    return kind == CXCursor_FunctionDecl ||
            kind == CXCursor_CXXMethod ||
            kind == CXCursor_Constructor ||
            kind == CXCursor_Destructor;
}

struct ParseDefinitionBodyState
{
    std::string *code { nullptr };
    unsigned indent { 0 };
    unsigned compound { 0 };
    std::unordered_map<std::string, std::pair<std::string, std::string>> *database { nullptr };
};

void parse_and_annotate_line(CXCursor current_cursor, ParseDefinitionBodyState *state, bool no_br)
{
    CXToken *tokens { nullptr }; unsigned tokens_size { 0 };
    auto current_source_range { clang_getCursorExtent(current_cursor) };
    auto translation_unit { clang_Cursor_getTranslationUnit(current_cursor) };
    clang_tokenize(translation_unit, current_source_range, &tokens, &tokens_size);
    std::vector<CXCursor> token_mapped_cursors(tokens_size, clang_getNullCursor());
    clang_annotateTokens(translation_unit, tokens, tokens_size, token_mapped_cursors.data());
    std::stringstream ss;
    unsigned prev_token_line, prev_token_column;
    for (unsigned i = 0; i < tokens_size; ++i) {
        auto token_spelling { clang_getTokenSpelling(translation_unit, tokens[i]) };
        std::string_view token_sv(clang_getCString(token_spelling));
        auto token_kind { clang_getTokenKind(tokens[i]) };
        auto token_source_location { clang_getTokenLocation(translation_unit, tokens[i]) };
        unsigned token_begin_offset, token_begin_line, token_begin_column;
        clang_getFileLocation(token_source_location,
                              nullptr,
                              &token_begin_line,
                              &token_begin_column,
                              nullptr);
        if (i == 0) {
            prev_token_column = token_begin_column;
            prev_token_line = token_begin_line;
        }
        if (prev_token_line != token_begin_line) {
            prev_token_column = token_begin_column - 1;
        }
        unsigned diff { token_begin_column - prev_token_column };
        while (diff--) {
            ss << "&nbsp;";
        }
        if (token_sv[0] == '<') {
            ss << "&lt;";
        } else if (token_kind == CXToken_Identifier) {
            auto referenced_cursor { clang_getCursorReferenced(token_mapped_cursors[i]) };
            auto referenced_cursor_kind { clang_getCursorKind(referenced_cursor) };
            if (has_function_body(referenced_cursor_kind)) {
                auto referenced_cursor_usr { clang_getCursorUSR(referenced_cursor) };
                std::string_view usr_sv(clang_getCString(referenced_cursor_usr));
                auto it = state->database->find(std::string(usr_sv));
                if (it != state->database->end()) {
                    ss << "<a href=\"" << usr_sv << "\">" << token_sv << "</a>";
                } else {
                    ss << token_sv;
                }
                clang_disposeString(referenced_cursor_usr);
            } else {
                ss << token_sv;
            }
        } else {
            ss << token_sv;
        }
        prev_token_column = token_begin_column + token_sv.size();
        prev_token_line = token_begin_line;
        clang_disposeString(token_spelling);
    }
    if (!no_br) {
        ss << "<br>";
    } else {
        ss << " ";
    }
    state->code->append(ss.str());
    clang_disposeTokens(translation_unit, tokens, tokens_size);
}

CXChildVisitResult parse_definition_body(CXCursor current_cursor, CXCursor parent, CXClientData client_data) noexcept;

CXChildVisitResult parse_if(CXCursor current_cursor, CXCursor parent, CXClientData client_data) noexcept
{
    auto *state = reinterpret_cast<ParseDefinitionBodyState *>(client_data);
    auto current_cursor_kind { clang_getCursorKind(current_cursor) };
    if (current_cursor_kind == CXCursor_CompoundStmt) {
        //state->code->append("<br>");
        if (state->compound > 0) {
            for (unsigned i = 0; i < state->indent; ++i) {
                state->code->append("&nbsp;");
            }
            state->code->append("else<br>");
        }
        state->indent += 4;
        clang_visitChildren(current_cursor, parse_definition_body, state);
        state->indent -= 4;
        ++state->compound;
    } else if (current_cursor_kind == CXCursor_IfStmt) {
        for (unsigned i = 0; i < state->indent; ++i) {
            state->code->append("&nbsp;");
        }
        state->code->append("else if ");
        unsigned tmp = state->compound;
        state->compound = 0;
        clang_visitChildren(current_cursor, parse_if, state);
        state->compound = tmp;
    } else {
        parse_and_annotate_line(current_cursor, state, false);
    }
    return CXChildVisit_Continue;
}

CXChildVisitResult parse_while(CXCursor current_cursor, CXCursor parent, CXClientData client_data) noexcept
{
    auto *state = reinterpret_cast<ParseDefinitionBodyState *>(client_data);
    auto current_cursor_kind { clang_getCursorKind(current_cursor) };
    if (current_cursor_kind == CXCursor_CompoundStmt) {
        state->indent += 4;
        clang_visitChildren(current_cursor, parse_definition_body, state);
        state->indent -= 4;
    } else {
        parse_and_annotate_line(current_cursor, state, false);
    }
    return CXChildVisit_Continue;
}

CXChildVisitResult parse_for(CXCursor current_cursor, CXCursor parent, CXClientData client_data) noexcept
{
    auto *state = reinterpret_cast<ParseDefinitionBodyState *>(client_data);
    auto current_cursor_kind { clang_getCursorKind(current_cursor) };
    if (current_cursor_kind == CXCursor_CompoundStmt) {
        state->code->append("<br>");
        state->indent += 4;
        clang_visitChildren(current_cursor, parse_definition_body, state);
        state->indent -= 4;
    } else {
        parse_and_annotate_line(current_cursor, state, true);
    }
    return CXChildVisit_Continue;
}


CXChildVisitResult parse_definition_body(CXCursor current_cursor, CXCursor parent, CXClientData client_data) noexcept
{
    auto *state = reinterpret_cast<ParseDefinitionBodyState *>(client_data);
    CXCursorKind current_cursor_kind = clang_getCursorKind(current_cursor);
    if (current_cursor_kind == CXCursor_IfStmt) {
        for (unsigned i = 0; i < state->indent; ++i) {
            state->code->append("&nbsp;");
        }
        state->code->append("if ");
        clang_visitChildren(current_cursor, parse_if, state);
        state->compound = 0;
    } else if (current_cursor_kind == CXCursor_ForStmt) {
        for (unsigned i = 0; i < state->indent; ++i) {
            state->code->append("&nbsp;");
        }
        state->code->append("for ");
        clang_visitChildren(current_cursor, parse_for, state);
    } else if (current_cursor_kind == CXCursor_WhileStmt) {
        for (unsigned i = 0; i < state->indent; ++i) {
            state->code->append("&nbsp;");
        }
        state->code->append("while ");
        clang_visitChildren(current_cursor, parse_while, state);
    } else {
        for (unsigned i = 0; i < state->indent; ++i) {
            state->code->append("&nbsp;");
        }
        parse_and_annotate_line(current_cursor, state, false);
    }
    return CXChildVisit_Continue;
}

CXChildVisitResult find_definition_body(CXCursor current_cursor, CXCursor parent, CXClientData client_data) noexcept
{
    CXCursorKind const current_cursor_kind { clang_getCursorKind(current_cursor) };
    if (current_cursor_kind == CXCursor_CompoundStmt) {
        clang_visitChildren(current_cursor, parse_definition_body, client_data);
        return CXChildVisit_Break;
    }
    return CXChildVisit_Continue;
}

CXChildVisitResult parse_function_definitions(CXCursor current_cursor, CXCursor parent, CXClientData client_data) noexcept
{
    auto *database = reinterpret_cast<std::unordered_map<std::string, std::pair<std::string, std::string>> *>(client_data);
    CXCursorKind const current_cursor_kind { clang_getCursorKind(current_cursor) };
    CXSourceLocation const current_token_location { clang_getCursorLocation(current_cursor) };
    if (clang_Location_isInSystemHeader(current_token_location)) {
        return CXChildVisit_Continue;
    }
    if (has_function_body(current_cursor_kind) &&
            clang_isCursorDefinition(current_cursor)) {
        std::string cursor_usr = to_string(clang_getCursorUSR(current_cursor));
        std::string full_function_name = convert_to_full_name(cursor_usr, current_cursor);
        auto it = database->emplace(cursor_usr, std::make_pair(full_function_name, ""));
        std::string *pref = &it.first->second.second;
        ParseDefinitionBodyState pdbs;
        pdbs.code = pref;
        pdbs.database = database;
        clang_visitChildren(current_cursor, find_definition_body, &pdbs);
    }
    return CXChildVisit_Recurse;
}

CXChildVisitResult find_function_definitions(CXCursor current_cursor, CXCursor parent, CXClientData client_data) noexcept
{
    auto *database = reinterpret_cast<std::unordered_map<std::string, std::pair<std::string, std::string>> *>(client_data);
    CXCursorKind const current_cursor_kind { clang_getCursorKind(current_cursor) };
    CXSourceLocation const current_token_location { clang_getCursorLocation(current_cursor) };
    if (clang_Location_isInSystemHeader(current_token_location)) {
        return CXChildVisit_Continue;
    }
    if (has_function_body(current_cursor_kind) &&
            clang_isCursorDefinition(current_cursor)) {
        std::string cursor_usr = to_string(clang_getCursorUSR(current_cursor));
        std::string full_function_name = convert_to_full_name(cursor_usr, current_cursor);
        database->emplace(cursor_usr, std::make_pair(full_function_name, ""));
    }
    return CXChildVisit_Recurse;
}

void run_indexer(std::unordered_map<std::string, std::pair<std::string, std::string>> &hmap)
{
    CXIndex index = clang_createIndex(0, 1);
    CXCompilationDatabase_Error error_code;
    CXCompilationDatabase c_database = clang_CompilationDatabase_fromDirectory("/home/user/Workspace/build-CodeViewer-Desktop-Debug", &error_code);
    if (error_code != CXCompilationDatabase_NoError) {
        std::exit(1);
    }
    CXCompileCommands c_commands = clang_CompilationDatabase_getAllCompileCommands(c_database);
    unsigned compile_commands_size { clang_CompileCommands_getSize(c_commands) };
    std::vector<CXTranslationUnit> translation_units(compile_commands_size);
    std::vector<const char *> vec {"-I", "/usr/lib/clang/12.0.1/include/"};
    for (int i = 0; i < compile_commands_size; ++i) {
        CXCompileCommand c_command = clang_CompileCommands_getCommand(c_commands, i);
        CXString file_name = clang_CompileCommand_getFilename(c_command);
        for (int j = 0; j < clang_CompileCommand_getNumArgs(c_command) - 2; ++j) {
            vec.push_back(clang_getCString(clang_CompileCommand_getArg(c_command, j)));
        }
        CXErrorCode parse_error = clang_parseTranslationUnit2(
                    index,
                    clang_getCString(file_name),
                    vec.data(),
                    vec.size(),
                    nullptr,
                    0,
                    CXTranslationUnit_None,
                    &translation_units[i]
                    );
        if (parse_error != CXError_Success) {
            std::exit(parse_error);
        }
        CXCursor cursor = clang_getTranslationUnitCursor(translation_units[i]);
        clang_visitChildren(cursor, find_function_definitions, &hmap);
        clang_suspendTranslationUnit(translation_units[i]);
        clang_disposeString(file_name);
    }
    for (int i = 0; i < compile_commands_size; ++i) {
        clang_reparseTranslationUnit(translation_units[i], 0, nullptr, 0);
        CXCursor cursor = clang_getTranslationUnitCursor(translation_units[i]);
        clang_visitChildren(cursor, parse_function_definitions, &hmap);
        clang_disposeTranslationUnit(translation_units[i]);
    }
    clang_CompileCommands_dispose(c_commands);
    clang_CompilationDatabase_dispose(c_database);
    clang_disposeIndex(index);
}
