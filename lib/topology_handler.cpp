//
// Copyright 2012 Josh Blum
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <gras_impl/block_actor.hpp>

using namespace gnuradio;

template <typename V, typename Sig>
void fill_item_sizes_from_sig(V &v, const Sig &s, const size_t size)
{
    v.resize(size);
    for (size_t i = 0; i < v.size(); i++)
    {
        v[i] = s->sizeof_stream_item(i);
    }
}

template <typename V, typename T>
void resize_fill_grow(V &v, const size_t new_len, const T &fill)
{
    if (v.size() >= new_len) return; //dont ever shrink it
    v.resize(new_len, fill);
}

template <typename V>
void resize_fill_back(V &v, const size_t new_len)
{
    if (v.empty()) v.resize(1);
    resize_fill_grow(v, new_len, v.back());
}

void BlockActor::handle_topology(
    const Apology::WorkerTopologyMessage &,
    const Theron::Address from
){
    MESSAGE_TRACER();

    const size_t num_inputs = this->get_num_inputs();
    const size_t num_outputs = this->get_num_outputs();

    //call check_topology on block before committing settings
    this->block_ptr->check_topology(num_inputs, num_outputs);

    //fill the item sizes from the IO signatures
    fill_item_sizes_from_sig(this->input_items_sizes, block_ptr->input_signature(), num_inputs);
    fill_item_sizes_from_sig(this->output_items_sizes, block_ptr->output_signature(), num_outputs);

    //resize and fill port properties
    resize_fill_back(this->input_configs, num_inputs);
    resize_fill_back(this->output_configs, num_outputs);

    //resize the bytes consumed/produced
    resize_fill_grow(this->items_consumed, num_inputs, 0);
    resize_fill_grow(this->items_produced, num_outputs, 0);

    //resize all work buffers to match current connections
    this->work_input_items.resize(num_inputs);
    this->work_output_items.resize(num_outputs);
    this->work_ninput_items.resize(num_inputs);
    this->fcast_ninput_items.resize(num_inputs);
    this->input_items.resize(num_inputs);
    this->output_items.resize(num_outputs);
    this->consume_items.resize(num_inputs, 0);
    this->consume_called.resize(num_inputs, false);
    this->produce_items.resize(num_outputs, 0);
    this->input_queues.resize(num_inputs);
    this->output_queues.resize(num_outputs);
    this->forecast_enable = num_outputs != 0 and num_inputs != 0;

    this->input_tokens.resize(num_inputs);
    this->output_tokens.resize(num_outputs);
    this->inputs_done.resize(num_inputs);
    this->outputs_done.resize(num_outputs);
    this->output_allocation_hints.resize(num_outputs);

    //resize tags vector to match sizes
    this->input_tags_changed.resize(num_inputs);
    this->input_tags.resize(num_inputs);

    //a block looses all connections, allow it to free
    if (num_inputs == 0 and num_outputs == 0)
    {
        this->mark_done();
    }

    this->topology_init = true;
    this->handle_update_inputs(UpdateInputsMessage(), Theron::Address());

    this->Send(0, from); //ACK
}

void BlockActor::handle_update_inputs(
    const UpdateInputsMessage &,
    const Theron::Address
){
    const size_t num_inputs = this->get_num_inputs();
    const size_t num_outputs = this->get_num_outputs();

    //impose input reserve requirements based on relative rate and output multiple
    resize_fill_grow(this->input_reserve_items, num_inputs, 1);
    std::vector<size_t> input_lookahead_items(num_inputs);
    for (size_t i = 0; i < num_inputs; i++)
    {
        input_lookahead_items[i] = this->input_configs[i].lookahead_items;

        //TODO, this is a little cheap, we only look at output multiple [0]
        const size_t multiple = (num_outputs)?this->output_configs.front().reserve_items:1;
        if (this->enable_fixed_rate)
        {
            this->input_reserve_items[i] = size_t(std::ceil(multiple/this->relative_rate));
        }
        if (this->input_reserve_items[i] == 0) this->input_reserve_items[i] = 1;
    }

    //init the history comprehension on input queues
    this->input_queues.init(input_lookahead_items, this->input_reserve_items, this->input_items_sizes);
}
