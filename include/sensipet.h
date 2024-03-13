#pragma once

#include "sensipet_state.h"
#include "sensipet_stats.h"

/// @brief Main class for handling the SensiPet
class SensiPet
{
  private:
    SensiPetStats pet_stats;

    SensiPetState *current_state = nullptr;

  public:
    SensiPet();
    ~SensiPet();

    void update_state(Action action);
    SensiPetState *get_current_state();
    void set_current_state(SensiPetState *state);

    // Getters and setters for pet stats
    inline uint16_t get_loneliness()
    {
        return pet_stats.loneliness;
    }
    inline uint16_t get_hunger()
    {
        return pet_stats.hunger;
    }
    inline uint16_t get_comfort()
    {
        return pet_stats.comfort;
    }
};