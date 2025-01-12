/*
 * piece.hpp
 *
 * 12-01-2025 by madpl
 */
#pragma once


enum class PieceType
{
	ROOK,
	KNIGHT,
	BISHOP,
	QUEEN,
	KING,
	PAWN,
	NONE
};


struct Piece
{
public:
    Piece(PieceType t = PieceType::NONE, char c = ' '):
		type(t),
		color(c)
	{}
    
public:
    PieceType type;
    char color; // 'W' for white, 'B' for black
};
