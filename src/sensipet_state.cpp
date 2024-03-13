#include "sensipet_state.h"

void SensiPetState::create_transition(Action action_type, SensiPetState *result_state)
{
    state_transistions[action_type] = result_state;
}

SensiPetState *SensiPetState::get_transition(Action action_type)
{
    if (state_transistions.count(action_type) <= 0)
    {
        return nullptr;
    }

    return state_transistions[action_type];
}