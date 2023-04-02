#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <memory>
#include <map>
#include <vector>
#include "ast/tree.h"

/**
 * About
 *
 *
 */
class IRGenerator
{
public:
    IRGenerator()
        : context_{}
        , builder_{ std::make_unique<llvm::IRBuilder<>>( context_)}
        , module_{}
    {}

public:
    llvm::Module* compile( const ast_node* root);

private:
    llvm::Value* compile_stdcall( const ast_node* node);
    llvm::Value* compile_define ( const ast_node* node);
    llvm::Value* compile_return ( const ast_node* node);
    llvm::Value* compile_assign ( const ast_node* node);
    llvm::Value* compile_expr   ( const ast_node* node);
    llvm::Value* compile_while  ( const ast_node* node);
    llvm::Value* compile_call   ( const ast_node* node);
    llvm::Value* compile_stmt   ( const ast_node* node);
    llvm::Value* compile_if     ( const ast_node* node);

    llvm::Value* find_symbol  ( const char* name);

    struct Declaration
    {
        std::string name;
        std::size_t n_params;
    };

    void define_symbol( const char* symbol);

private:
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    std::unique_ptr<llvm::Module> module_;

    using Scope = std::map<std::string, llvm::Value*>;

    class SymbolTable
        : public std::vector<Scope>
    {
    public:
        llvm::Value* get( const std::string& ident) const
        {
            llvm::Value* value = find( std::move( ident));
            if ( !value )
            {
                throw std::out_of_range{ "find variable symbol failed"};
            }

            return value;
        }

        llvm::Value* find( const std::string& ident) const
        {
            for ( auto&& symtab : *this )
            {
                auto symbol = symtab.find( ident);
                if ( symbol != symtab.end() )
                {
                    return symbol->second;
                }
            }

            return nullptr;
        }

        Scope& emplace_back()
        {
            std::vector<Scope>::emplace_back();
            return back();
        }

        bool is_global() const { return !!(size() == 1); }
    };

    SymbolTable scopes_;
};


