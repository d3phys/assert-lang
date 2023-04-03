#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <memory>
#include <iostream>
#include <map>
#include <vector>
#include "logs.h"
#include "ast/tree.h"

/**
 * About
 *
 *
 */
class IRGenerator
{
public:
    IRGenerator( std::string name)
        : context_{}
        , builder_{ std::make_unique<llvm::IRBuilder<>>( context_)}
        , module_{ std::make_unique<llvm::Module>( name, context_)}
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

    void declare_functions( const ast_node* node);

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

    llvm::Value* get_element_ptr( const std::string& ident,
                                  size_t shift = 0)
    {
        return get_element_ptr( std::move( ident),
                                llvm::ConstantInt::get( llvm::Type::getInt64Ty( context_), shift));
    }

    llvm::Value* get_element_ptr( const std::string& ident,
                                  llvm::Value* shift)
    {
$$
        std::fprintf( logs, "Try to find: %s\n", ident.c_str());
        Allocation alloc = scopes_.get( ident);

        llvm::Value *idxs[] = {
            llvm::ConstantInt::get( llvm::Type::getInt64Ty( context_), 0),
            shift
        };
        return builder_->CreateGEP( alloc.type, alloc.value, idxs);
    }

    struct Allocation
    {
        llvm::Value* value;
        llvm::Type* type;
    };

    using Scope = std::map<std::string, Allocation>;

    class SymbolTable
        : public std::vector<Scope>
    {
    public:
        Allocation get( const std::string& ident) const
        {
            Allocation value = find( std::move( ident));
            if ( !value.value )
            {
                throw std::out_of_range{ "find variable symbol not found!"};
            }

            return value;
        }

        Scope& emplace_back()
        {
            std::vector<Scope>::emplace_back();
            return back();
        }

        bool is_global() const { return !!(size() == 1); }

        Allocation find( const std::string& ident) const
        {
            for ( const Scope& symtab : *this )
            {
                auto symbol = symtab.find( ident);
                if ( symbol != symtab.end() )
                {
                    return symbol->second;
                }
            }

            return Allocation{};
        }
    };


    SymbolTable scopes_;
};


