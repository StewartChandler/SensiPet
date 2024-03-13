#include "sensipet.h"

SensiPet::SensiPet()
{
}

SensiPet::~SensiPet()
{
}

void SensiPet::update_state(Action action)
{
    if (!current_state)
    {
        return;
    }

    set_current_state(current_state->get_transition(action));
}

SensiPetState *SensiPet::get_current_state()
{
    return current_state;
}

void SensiPet::set_current_state(SensiPetState *state)
{
    if (current_state)
    {
        current_state->get_event_queue()->break_dispatch();
    }

    current_state = state;
    current_state->get_event_queue()->dispatch_forever();
}