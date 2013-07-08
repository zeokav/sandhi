// Copyright (C) by Josh Blum. See LICENSE.txt for licensing information.

#include <gras/block.hpp>
#include <gras/factory.hpp>

struct MyBlock : gras::Block
{
    MyBlock(void):
        gras::Block("MyBlock")
    {
        this->register_call("get_num", &MyBlock::get_num);
    }

    int get_num(void)
    {
        return 42;
    }

    //dummy work
    void work(const InputItems &, const OutputItems &){}
};

gras::Block *make_my_block(void)
{
    return new MyBlock();
}

GRAS_REGISTER_FACTORY("/tests/my_block", make_my_block)
