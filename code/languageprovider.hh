#pragma once

#include <cstddef>

// Copies the provider's UTF-8 string for id into destination when one exists.
bool Language_Provider_Load_String(int id, char * destination, std::size_t capacity);
