#include "Test.h"

#include <sstream>

#include "slang/diagnostics/DiagnosticEngine.h"
#include "slang/parsing/Parser.h"
#include "slang/parsing/Preprocessor.h"
#include "slang/symbols/CompilationUnitSymbols.h"
#include "slang/util/ScopeGuard.h"
#include "slang/text/SourceManager.h"

std::string findTestDir() {
    auto path = fs::current_path();
    while (!fs::exists(path / "tests")) {
        path = path.parent_path();
        ASSERT(!path.empty());
    }

    return (path / "tests/unittests/data/").string();
}

void setupSourceManager(SourceManager& sourceManager) {
    auto testDir = findTestDir();
    sourceManager.addUserDirectory(testDir);
    sourceManager.addSystemDirectory(testDir);
    sourceManager.addSystemDirectory(string_view(testDir + "system/"));
}

SourceManager& getSourceManager() {
    static SourceManager* sourceManager = nullptr;
    if (!sourceManager) {
        auto testDir = findTestDir();
        sourceManager = new SourceManager();
        sourceManager->setDisableProximatePaths(true);
        setupSourceManager(*sourceManager);
    }
    return *sourceManager;
}

bool withinUlp(double a, double b) {
    static_assert(sizeof(double) == sizeof(int64_t));
    int64_t ia, ib;
    memcpy(&ia, &a, sizeof(double));
    memcpy(&ib, &b, sizeof(double));
    return std::abs(ia - ib) <= 1;
}

std::string report(const Diagnostics& diags) {
    if (diags.empty())
        return "";

    return DiagnosticEngine::reportAll(SyntaxTree::getDefaultSourceManager(), diags);
}

std::string reportGlobalDiags(const Diagnostics& diagnostics) {
    return DiagnosticEngine::reportAll(getSourceManager(), diagnostics);
}

std::string to_string(const Diagnostic& diag) {
    return DiagnosticEngine::reportAll(getSourceManager(), span(&diag, 1));
}

std::tuple<Token, Diagnostics> lexToken(string_view text) {
    Preprocessor preprocessor(getSourceManager());
    ScopeGuard _([&] { alloc.steal(std::move(preprocessor).getAllocator()); });
    preprocessor.pushSource(text);

    Token token = preprocessor.next();
    REQUIRE(token);
    
    return { token, std::move(preprocessor).getDiagnostics() };
}

Token lexRawToken(string_view text) {
    diagnostics.clear();
    auto buffer = getSourceManager().assignText(text);
    Lexer lexer(buffer, alloc, diagnostics);

    Token token = lexer.lex();
    REQUIRE(token);
    return token;
}

std::tuple<const ModuleDeclarationSyntax&, Diagnostics> parseModule(const std::string& text) {
    Preprocessor preprocessor(getSourceManager());
    preprocessor.pushSource(text);
    ScopeGuard _([&] { alloc.steal(std::move(preprocessor).getAllocator()); });

    Parser parser(preprocessor);
    return { parser.parseModule(), std::move(preprocessor).getDiagnostics() };
}

std::tuple<const ClassDeclarationSyntax&, Diagnostics> parseClass(const std::string& text) {
    Preprocessor preprocessor(getSourceManager());
    preprocessor.pushSource(text);
    ScopeGuard _([&] { alloc.steal(std::move(preprocessor).getAllocator()); });

    Parser parser(preprocessor);
    return { parser.parseClass(), std::move(preprocessor).getDiagnostics() };
}

std::tuple<const MemberSyntax&, Diagnostics> parseMember(const std::string& text) {
    Preprocessor preprocessor(getSourceManager());
    preprocessor.pushSource(text);
    ScopeGuard _([&] { alloc.steal(std::move(preprocessor).getAllocator()); });

    Parser parser(preprocessor);
    MemberSyntax* member = parser.parseSingleMember(SyntaxKind::ModuleDeclaration);
    REQUIRE(member);
    return { *member, std::move(preprocessor).getDiagnostics() };
}

std::tuple<const StatementSyntax&, Diagnostics> parseStatement(const std::string& text) {
    Preprocessor preprocessor(getSourceManager());
    preprocessor.pushSource(text);
    ScopeGuard _([&] { alloc.steal(std::move(preprocessor).getAllocator()); });

    Parser parser(preprocessor);
    return { parser.parseStatement(), std::move(preprocessor).getDiagnostics() };
}

std::tuple<const ExpressionSyntax&, Diagnostics> parseExpression(const std::string& text) {
    Preprocessor preprocessor(getSourceManager());
    preprocessor.pushSource(text);
    ScopeGuard _([&] { alloc.steal(std::move(preprocessor).getAllocator()); });

    Parser parser(preprocessor);
    return { parser.parseExpression(), std::move(preprocessor).getDiagnostics() };
}

std::tuple<const CompilationUnitSyntax&, Diagnostics> parseCompilationUnit(const std::string& text) {
    Preprocessor preprocessor(getSourceManager());
    preprocessor.pushSource(text);
    ScopeGuard _([&] { alloc.steal(std::move(preprocessor).getAllocator()); });

    Parser parser(preprocessor);
    return { parser.parseCompilationUnit(), std::move(preprocessor).getDiagnostics() };
}

const InstanceSymbol& evalModule(std::shared_ptr<SyntaxTree> syntax, Compilation& compilation) {
    compilation.addSyntaxTree(syntax);
    const RootSymbol& root = compilation.getRoot();

    REQUIRE(root.topInstances.size() > 0);
    return *root.topInstances[0];
}

bool LogicExactlyEqualMatcher::match(const logic_t& t) const {
    return exactlyEqual(t, value);
}

std::string LogicExactlyEqualMatcher::describe() const {
    std::ostringstream ss;
    ss << "equals " << value;
    return ss.str();
}

bool SVIntExactlyEqualMatcher::match(const SVInt& t) const {
    return exactlyEqual(t, value);
}

std::string SVIntExactlyEqualMatcher::describe() const {
    std::ostringstream ss;
    ss << "equals " << value;
    return ss.str();
}
