/*
 * utils.hpp
 * 
 * 20-01-2025 by madpl
 */
#pragma once

#include <iostream>
#include "stockHandle.hpp"


int initialCommand(Stockfish& engine);
std::string getNextMove(Stockfish& engine, std::string& position);
