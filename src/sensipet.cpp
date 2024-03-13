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

    current_state = current_state->get_transition(action);
}

SensiPetState *SensiPet::get_current_state()
{
    return current_state;
}

void SensiPet::set_current_state(SensiPetState *state)
{
    current_state = state;
}