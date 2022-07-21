#pragma once

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning( \
        disable : 4459) // annoying warning about global "alloc" being shadowed by locals
#endif

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "slang/compilation/Compilation.h"
#include "slang/diagnostics/AllDiags.h"
#include "slang/syntax/SyntaxTree.h"

namespace slang {

extern BumpAllocator alloc;
extern Diagnostics diagnostics;

class InstanceSymbol;
struct ClassDeclarationSyntax;
struct CompilationUnitSyntax;
struct ExpressionSyntax;
struct MemberSyntax;
struct ModuleDeclarationSyntax;
struct StatementSyntax;

} // namespace slang

using namespace slang;

#define CHECK_DIAGNOSTICS_EMPTY(diagnostics)  \
    do {                                      \
        diagnostics.sort(getSourceManager()); \
        if (!diagnostics.empty())             \
            FAIL_CHECK(reportGlobalDiags(diagnostics));  \
    } while (0)

#define NO_COMPILATION_ERRORS                          \
    do {                                               \
        auto& diags = compilation.getAllDiagnostics(); \
        if (!diags.empty()) {                          \
            FAIL_CHECK(report(diags));                 \
        }                                              \
    } while (0)

#define NO_SESSION_ERRORS                      \
    do {                                       \
        auto diags = session.getDiagnostics(); \
        if (!diags.empty()) {                  \
            FAIL_CHECK(report(diags));         \
        }                                      \
    } while (0)

std::string findTestDir();
void setupSourceManager(SourceManager& sourceManager);
SourceManager& getSourceManager();

bool withinUlp(double a, double b);

std::string report(const Diagnostics& diags);
std::string reportGlobalDiags(const Diagnostics& diagnostics);
std::string to_string(const Diagnostic& diag);

std::tuple<Token, Diagnostics> lexToken(string_view text);
Token lexRawToken(string_view text);

std::tuple<const ModuleDeclarationSyntax&, Diagnostics> parseModule(const std::string& text);
std::tuple<const ClassDeclarationSyntax&, Diagnostics> parseClass(const std::string& text);
std::tuple<const MemberSyntax&, Diagnostics> parseMember(const std::string& text);
std::tuple<const StatementSyntax&, Diagnostics> parseStatement(const std::string& text);
std::tuple<const ExpressionSyntax&, Diagnostics> parseExpression(const std::string& text);
std::tuple<const CompilationUnitSyntax&, Diagnostics> parseCompilationUnit(const std::string& text);
const InstanceSymbol& evalModule(std::shared_ptr<SyntaxTree> syntax, Compilation& compilation);

class LogicExactlyEqualMatcher : public Catch::Matchers::MatcherGenericBase {
public:
    explicit LogicExactlyEqualMatcher(logic_t v) : value(v) {}

    bool match(const logic_t& t) const;
    std::string describe() const final;

private:
    logic_t value;
};

inline LogicExactlyEqualMatcher exactlyEquals(logic_t v) {
    return LogicExactlyEqualMatcher(v);
}

class SVIntExactlyEqualMatcher : public Catch::Matchers::MatcherGenericBase {
public:
    explicit SVIntExactlyEqualMatcher(SVInt v) : value(v) {}

    bool match(const SVInt& t) const;
    std::string describe() const final;

private:
    SVInt value;
};

inline SVIntExactlyEqualMatcher exactlyEquals(SVInt v) {
    return SVIntExactlyEqualMatcher(v);
}
