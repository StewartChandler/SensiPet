#pragma once

#include "action.h"
#include <unordered_map>

/// @brief Keeps track of details for a current state, as well as actions that lead this state into others.
class SensiPetState
{
  private:
    std::unordered_map<Action, SensiPetState *> state_transistions;

  public:
    // Create a transition from <action_type> to <result_state>.
    void create_transition(Action action_type, SensiPetState *result_state);

    // Get the next state from this state given the provided action. Returns nullptr if no such transition exists.
    SensiPetState *get_transition(Action action_type);

    // TODO: Need to figure out how we want to store and run what this particular state should run.
};