/*
 * chessBoard.cpp
 *
 * 12-01-2025 by madpl
 */
#include <iostream>
#include <cmath>
#include "defines.hpp"
#include "chessBoard.hpp"
#include "moveLogger.hpp"


ChessBoard::ChessBoard(sf::RenderWindow& window, MoveLogger& logger, SoundManager& sndManager):
	m_window(window),
	m_moveLogger(logger),
	m_sndManager(sndManager),
	m_enPassantTarget(-1, -1)
{
	#ifdef DEBUG
	std::cout << "[DEBUG] ctor ChessBoard\n";
	#endif
	
	m_board = std::vector<std::vector<Piece>>(8, std::vector<Piece>(8, Piece()));
}


ChessBoard::~ChessBoard()
{
	#ifdef DEBUG
	std::cout << "[DEBUG] dtor ChessBoard\n";
	#endif
	
	if(not m_board.empty())
	{
		for(int y = 0; y < 8; ++y)
			if(not m_board[y].empty())
				m_board[y].clear();
		
		m_board.clear();
	}
}


void ChessBoard::setInitialPositions()
{
	for(int y = 0; y < 8; ++y)
	{
		for(int x = 0; x < 8; ++x)
		{
			if(y == 1)
				m_board[y][x] = Piece(PieceType::PAWN, 'W');
			
			else if(y == 6)
				m_board[y][x] = Piece(PieceType::PAWN, 'B');
			
			else if(y == 0 or y == 7)
			{
				char color = (y == 0) ? 'W' : 'B';
				
				if(x == 0 or x == 7)
					m_board[y][x] = Piece(PieceType::ROOK, color);
				
				if(x == 1 or x == 6)
					m_board[y][x] = Piece(PieceType::KNIGHT, color);
				
				if(x == 2 or x == 5)
					m_board[y][x] = Piece(PieceType::BISHOP, color);
				
				if(x == 3)
					m_board[y][x] = Piece(PieceType::QUEEN, color);
				
				if(x == 4)
					m_board[y][x] = Piece(PieceType::KING, color);
			}
		}
	}
}


bool ChessBoard::isPieceAt(int x, int y) const
{
	return m_board[y][x].m_type != PieceType::NONE;
}


bool ChessBoard::isValidMove(int startX, int startY, int endX, int endY) const
{
	if(startX == endX and startY == endY)
		return false;

	if(m_board[startY][startX].m_color == m_board[endY][endX].m_color)
		return false;

	Piece piece = m_board[startY][startX];
	int dx = abs(endX - startX);
	int dy = abs(endY - startY);

	switch(piece.m_type)
	{
		case PieceType::PAWN:
			return validatePawnMove(piece, startX, startY, endX, endY, dx, dy);
		
		case PieceType::ROOK:
			return (dx == 0 or dy == 0) and isPathClear(startX, startY, endX, endY);
		
		case PieceType::KNIGHT:
			return (dx == 2 and dy == 1) or (dx == 1 and dy == 2);
		
		case PieceType::BISHOP:
			return dx == dy and isPathClear(startX, startY, endX, endY);
		
		case PieceType::QUEEN:
			return (dx == dy or dx == 0 or dy == 0) and isPathClear(startX, startY, endX, endY);
		
		case PieceType::KING:
			return validateKingMove(piece, startX, startY, endX, endY, dx, dy);
		
		default:
			return false;
	}
}


bool ChessBoard::validatePawnMove(const Piece& pawn, int startX, int startY, int endX, int endY, int dx, int dy) const
{
	int direction = (pawn.m_color == 'W') ? 1 : -1;

	// Normal move forward
	if(dx == 0)
	{
		if(dy == 1 and m_board[endY][endX].m_type == PieceType::NONE)
			return startY + direction == endY;
		
		if(dy == 2 and ((pawn.m_color == 'W' and startY == 1) or (pawn.m_color == 'B' and startY == 6)))
			return (m_board[startY + direction][startX].m_type == PieceType::NONE and
					m_board[endY][endX].m_type == PieceType::NONE);
	}

	if(dx == 1 and dy == 1)
	{
		// if the target square contains an opponent's piece - standard capture
		if(m_board[endY][endX].m_type != PieceType::NONE)
			return (m_board[endY][endX].m_color != pawn.m_color);
		else
		{
			// try beating test en passant:
			// we check if the target field corresponds to the saved en passant position.
			if(sf::Vector2i(endX, endY) == m_enPassantTarget)
			{
				// position of a pawn that could have been captured en passant is "behind" the target square,
				// i.e. towards the opponent – ​​we calculate his Y coordinate.
				int capturedPawnY = endY - direction;
				if(capturedPawnY >= 0 and capturedPawnY < 8)
				{
					if(m_board[capturedPawnY][endX].m_type == PieceType::PAWN and
					   m_board[capturedPawnY][endX].m_color != pawn.m_color)
						return true;
				}
			}
		}
	}

	return false;
}


bool ChessBoard::validateKingMove(const Piece& king, int startX, int startY, int endX, int endY, int dx, int dy) const
{
	if(dx <= 1 and dy <= 1)
		return true;

	// Castling check (basic version)
	if(dx == 2 and dy == 0 and !isInCheck(king.m_color))
	{
		int rookX = (endX > startX) ? 7 : 0;
		
		if(m_board[startY][rookX].m_type == PieceType::ROOK and m_board[startY][rookX].m_color == king.m_color)
			return isPathClear(startX, startY, rookX, startY);
	}

	return false;
}


bool ChessBoard::isPathClear(int startX, int startY, int endX, int endY) const
{
	int xDir = (endX - startX) ? (endX - startX) / abs(endX - startX) : 0;
	int yDir = (endY - startY) ? (endY - startY) / abs(endY - startY) : 0;

	int x = startX + xDir;
	int y = startY + yDir;

	while(x != endX or y != endY)
	{
		if(m_board[y][x].m_type != PieceType::NONE)
			return false;
		
		x += xDir;
		y += yDir;
	}

	return true;
}


bool ChessBoard::isInCheck(char kingColor) const
{
	int kingX = -1, kingY = -1;

	for(int y = 0; y < 8; ++y)
	{
		for(int x = 0; x < 8; ++x)
		{
			if(m_board[y][x].m_type == PieceType::KING and m_board[y][x].m_color == kingColor)
			{
				kingX = x;
				kingY = y;
				
				break;
			}
		}
		
		if(kingX != -1)
			break;
	}

	if(kingX == -1 or kingY == -1)
	{
		#ifdef DEBUG
		std::cout << "[DEBUG] Error: King not found on m_board!\n";
		#endif
		
		return false;
	}

	// Check if any opponent's piece can attack the king
	for(int y = 0; y < 8; ++y)
	{
		for(int x = 0; x < 8; ++x)
		{
			if(m_board[y][x].m_color != kingColor and m_board[y][x].m_type != PieceType::NONE)
			{
				if(isValidMove(x, y, kingX, kingY))
				{
					#ifdef DEBUG
					std::cout << "[DEBUG] Piece (" << pieceTypeToString(m_board[y][x].m_type) 
								<< ") at (" << x << ", " << y << ") attack king!\n";
					#endif
					
					return true;
				}
			}
		}
	}

	return false;
}


std::string ChessBoard::pieceTypeToString(PieceType type) const
{
	switch(type)
	{
		case PieceType::PAWN:
			return "Pawn";
		
		case PieceType::ROOK:
			return "Rook";
		
		case PieceType::KNIGHT:
			return "Knight";
		
		case PieceType::BISHOP:
			return "Bishop";
		
		case PieceType::QUEEN:
			return "Queen";
		
		case PieceType::KING:
			return "King";
		
		default:
			return "Unknown";
	}
}


void ChessBoard::movePiece(int startX, int startY, int endX, int endY)
{
	Piece movingPiece = m_board[startY][startX];
	int dx = abs(endX - startX);
	int dy = abs(endY - startY);
	int direction = (movingPiece.m_color == 'W') ? 1 : -1;
	bool enPassantCapture = false;

	// detection of strike en passant:
	// if a pawn moves diagonally (dx == 1 && dy == 1) to an empty square
	// and the target field corresponds to the saved en passant position, then we have en passant.
	if(movingPiece.m_type == PieceType::PAWN and dx == 1 and dy == 1 and m_board[endY][endX].m_type == PieceType::NONE)
	{
		if(sf::Vector2i(endX, endY) == m_enPassantTarget)
			enPassantCapture = true;
	}

	// making a move – we move a pawn
	m_board[endY][endX] = movingPiece;
	m_board[startY][startX] = Piece();

	// Jeśli ruch był biciem en passant, usuwamy pionka przeciwnika
	// if move was capture en passant, then we remove opponent pawn
	if(enPassantCapture)
	{
		int capturedPawnY = endY - direction;
		m_board[capturedPawnY][endX] = Piece();
	}

	// setting the en passant possibilities:
	// if a pawn has moved two squares, we set the square through which it "jumped"
	if(movingPiece.m_type == PieceType::PAWN and dy == 2)
		m_enPassantTarget = sf::Vector2i(startX, startY + direction);
	else
		// otherwise we clear the possibility en passant
		m_enPassantTarget = sf::Vector2i(-1, -1);

	char currentPlayerColor = m_board[endY][endX].m_color;
	char opponentColor = (currentPlayerColor == 'W') ? 'B' : 'W';

	if(isInCheck(opponentColor))
	{
		m_moveLogger.updateCheckStatus("King is check!");
		m_sndManager.play("check");
		
		#ifdef DEBUG
		std::cout << (opponentColor == 'W' ? "[DEBUG] black" : "[DEBUG] white") 
					<< " king is in check!\n";
		#endif
	}
	else
		m_moveLogger.updateCheckStatus("");
}


std::string ChessBoard::toChess(int x, int y)
{
	std::string str;

	str += static_cast<char>(97 + x);
	str += static_cast<char>(7 - y + 49);

	return str;
}


sf::Vector2i ChessBoard::toCoords(char col, char row)
{
	int x = static_cast<int>(col - 'a');
	int y = 7 - static_cast<int>(row - '1');
	
	return sf::Vector2i(x, y);
}


bool ChessBoard::atBoard(sf::Vector2i Start, sf::Vector2i End)
{
	if((Start.x >= 0 and Start.x < 8) and (Start.y >= 0 and Start.y < 8) and
	   (End.x >= 0 and End.x < 8) and (End.y >= 0 and End.y < 8))
		return true;
	
	return false;
}


bool ChessBoard::castling(std::string& str, std::string& position, sf::Vector2i& rookStart, sf::Vector2i& rookEnd)
{
	bool result = false;
	
	// castling if the king has not yet moved
	if(str == "e1g1") // king's move
		if(position.find("e1") == -1)
		{
			// "h1f1"
			rookStart = toCoords('h', '1');
			rookEnd = toCoords('f', '1');
			result = true;
		}

	if(str == "e8g8")
		if(position.find("e8") == -1)
		{
			// "h8f8"
			rookStart = toCoords('h', '8');
			rookEnd = toCoords('f', '8');
			result = true;
		}

	if(str == "e1c1")
		if(position.find("e1") == -1)
		{
			// "a1d1"
			rookStart = toCoords('a', '1');
			rookEnd = toCoords('d', '1');
			result = true;
		}

	if(str == "e8c8") 
		if(position.find("e8") == -1)
		{
			// "a8d8"
			rookStart = toCoords('a', '8');
			rookEnd = toCoords('d', '8');
			result = true;
		}
	
	return result;
}


void ChessBoard::saveGame(const std::string& filename)
{
	std::ofstream file(filename);
	
	if(not file)
	{
		std::cerr << "Error: Unable to open file for saving!\n";
		
		return;
	}
	
	for(int y = 0; y < 8; ++y)
	{
		for(int x = 0; x < 8; ++x)
		{
			const Piece& piece = m_board[y][x];
			
			file << static_cast<int>(piece.m_type) << " " 
					<< piece.m_color << " " 
					<< x << " " << y << "\n";
		}
	}
	
	file << "ENPASSANT " << m_enPassantTarget.x << " " << m_enPassantTarget.y << "\n";
	
	file.close();
}


void ChessBoard::loadGame(const std::string& filename)
{
	std::ifstream file(filename);
	
	if(not file)
	{
		std::cerr << "Error: Unable to open file for loading!\n";
		
		return;
	}
	
	m_board = std::vector<std::vector<Piece>>(8, std::vector<Piece>(8, Piece()));
	std::string line;
	
	while(std::getline(file, line))
	{
		std::istringstream iss(line);
		std::string type;
		int x, y;
		char color;
		
		iss >> type;
		if(type == "ENPASSANT")
		{
			iss >> x >> y;
			m_enPassantTarget = sf::Vector2i(x, y);
			std::cout << "Loaded En Passant: " << x << " " << y << std::endl;
		}
		else
		{
			try
			{
				int pieceType = std::stoi(type);
				if(not (iss >> color >> x >> y))
				{
					std::cerr << "Error parsing line: " << line << std::endl;
					
					continue;
				}
				
				if(x < 0 || x >= 8 || y < 0 || y >= 8)
				{
					std::cerr << "Error: Invalid piece position " << x << " " << y << std::endl;
					
					continue;
				}
				
				m_board[y][x] = Piece(static_cast<PieceType>(pieceType), color);
				
				std::cout << "Loaded: " << pieceType 
							<< " " << color << " " 
							<< x << " " << y << std::endl;
			}
			catch(const std::exception& e)
			{
				std::cerr << "Error parsing line: " << line << std::endl;
			}
		}
	}
	
	file.close();
}


const Piece& ChessBoard::getPiece(int x, int y) const
{
	return m_board[y][x];
}


const sf::Vector2i& ChessBoard::getEnPassantTarget() const
{
	return m_enPassantTarget;
}


void ChessBoard::draw(sf::Texture& boardTexture, sf::Texture& figuresTexture) 
{
	sf::Sprite boardSprite(boardTexture);
	sf::Sprite pieceSprite(figuresTexture);
	
	boardSprite.setPosition(0, 0);
	m_window.draw(boardSprite);
	
	for(int y = 0; y < 8; ++y) 
	{
		for(int x = 0; x < 8; ++x) 
		{
			Piece piece = m_board[y][x];
			if(piece.m_type == PieceType::NONE)
				continue;
			
			int pieceIndex = static_cast<int>(piece.m_type);
			int colorOffset = (piece.m_color == 'B') ? TILE_SIZE : 0;
			
			pieceSprite.setTextureRect(sf::IntRect(pieceIndex * TILE_SIZE, colorOffset, TILE_SIZE, TILE_SIZE));
			pieceSprite.setPosition(x * TILE_SIZE + OFFSET, y * TILE_SIZE + OFFSET);
			pieceSprite.setOrigin(0, 1);
			
			m_window.draw(pieceSprite);
		}
	}
}
