#pragma once
#include "ast/tree.h"
#include "array.h"
#include <stdexcept>

class Tree
{
public:
    Tree( char *buf)
        : idents_{}
    {
        root_ = read_ast_tree( &buf, &idents_);
        if ( root_ == nullptr )
        {
            array_dtor();
            throw std::runtime_error{ "Tree ctor failed"};
        }
    }

    ~Tree()
    {
        array_dtor();
    }

    ast_node* root() const { return root_; }

private:
    void array_dtor() noexcept
    {
        char **data = static_cast<char**>( idents_.data);
        for (size_t i = 0; i < idents_.size; i++) {
            free( data[i]);
        }

        free_array( &idents_, sizeof( char*));
        idents_.data = nullptr;
    }

private:
    array idents_;
    ast_node* root_;
};



