// Copyright (C) by Josh Blum. See LICENSE.txt for licensing information.

#ifndef INCLUDED_GRAS_BLOCK_I
#define INCLUDED_GRAS_BLOCK_I

%module GRAS_Block

%{
#include <gras/block.hpp>
%}

%include <gras/exception.i>
%import <gras/element.i>
%import <gras/tags.i>
%include <gras/tag_iter.i>
%import <gras/sbuffer.i>
%include <gras/buffer_queue.hpp>
%include <gras/thread_pool.hpp>
%include <gras/block_config.hpp>
%include <gras/block.hpp>

////////////////////////////////////////////////////////////////////////
// Create pythonic gateway to get and set
////////////////////////////////////////////////////////////////////////
%extend gras::Block
{
    %insert("python")
    %{
        def set(self, key, value):
            self._set_property(key, PMC_M(value))

        def get(self, key):
            return self._get_property(key)()

        def post_output_msg(self, which_output, value):
            if not isinstance(value, PMCC): value = PMC_M(value)
            self._post_output_msg(which_output, value)

        def post_input_msg(self, which_input, value):
            if not isinstance(value, PMCC): value = PMC_M(value)
            self._post_input_msg(which_input, value)

    %}
}

#endif /*INCLUDED_GRAS_BLOCK_I*/
