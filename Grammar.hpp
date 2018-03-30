#ifndef SWEET_LALR_GRAMMAR_HPP_INCLUDED
#define SWEET_LALR_GRAMMAR_HPP_INCLUDED

#include "SymbolType.hpp"
#include "LexemeType.hpp"
#include "Associativity.hpp"
#include "RegexToken.hpp"
#include <vector>

namespace sweet
{

namespace lalr
{

class GrammarSymbol;
class GrammarProduction;
class GrammarAction;
class LexerErrorPolicy;
class RegexCompiler;
class ParserErrorPolicy;
class GrammarCompiler;
class ParserStateMachine;

class Grammar
{
    std::string identifier_;
    std::vector<std::unique_ptr<GrammarSymbol>> symbols_; ///< The symbols in the grammar.
    std::vector<std::unique_ptr<GrammarProduction>> productions_; ///< The productions in the grammar.
    std::vector<std::unique_ptr<GrammarAction>> actions_; ///< The actions in the grammar.
    std::vector<RegexToken> whitespace_tokens_;
    bool active_whitespace_directive_;
    bool active_precedence_directive_;
    Associativity associativity_;
    int precedence_;
    GrammarProduction* active_production_;
    GrammarSymbol* active_symbol_;
    GrammarSymbol* start_symbol_; ///< The start symbol.
    GrammarSymbol* end_symbol_; ///< The end symbol.
    GrammarSymbol* error_symbol_; ///< The error symbol.
    std::unique_ptr<GrammarCompiler> parser_allocations_;
    std::unique_ptr<RegexCompiler> lexer_allocations_;
    std::unique_ptr<RegexCompiler> whitespace_lexer_allocations_;

public:
    Grammar();
    ~Grammar();
    const std::string& identifier() const;
    std::vector<std::unique_ptr<GrammarSymbol>>& symbols();
    std::vector<std::unique_ptr<GrammarProduction>>& productions();
    std::vector<std::unique_ptr<GrammarAction>>& actions();
    const std::vector<RegexToken>& whitespace_tokens() const;
    GrammarSymbol* start_symbol() const;
    GrammarSymbol* end_symbol() const;
    GrammarSymbol* error_symbol() const;
    Grammar& grammar( const std::string& identifier );
    Grammar& left( int line );
    Grammar& right( int line );
    Grammar& none( int line );
    Grammar& whitespace();
    Grammar& precedence();
    Grammar& production( const char* identifier, int line );
    Grammar& end_production();
    Grammar& end_expression();
    Grammar& error();
    Grammar& action( const char* identifier );
    Grammar& literal( const char* literal, int line );
    Grammar& regex( const char* regex, int line );
    Grammar& identifier( const char* identifier, int line );
    bool parse( const char* begin, const char* end );
    // bool generate( ParserStateMachine* parser_state_machine, ParserErrorPolicy* parser_error_policy = nullptr, LexerErrorPolicy* lexer_error_policy = nullptr );

private:
    GrammarSymbol* literal_symbol( const char* lexeme, int line );
    GrammarSymbol* regex_symbol( const char* lexeme, int line );
    GrammarSymbol* non_terminal_symbol( const char* lexeme, int line );
    GrammarSymbol* add_symbol( const char* lexeme, int line, LexemeType lexeme_type, SymbolType symbol_type );
    GrammarProduction* add_production( GrammarSymbol* symbol );
    GrammarAction* add_action( const char* id );
};

}

}

#endif
