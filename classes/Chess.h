#pragma once
#include "Game.h"
#include "Grid.h"
#include "BitBoard.h"
#include <vector>

constexpr int pieceSize = 80;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    
    // Bitboard helpers
    void initializeBitboards();
    void updateBitboards();
    void generateKnightMoveBitboards();
    void generateKingMoveBitboards();
    BitboardElement generateKnightMoves(int square) const;
    BitboardElement generateKingMoves(int square) const;
    std::vector<BitMove> generatePawnMoves(int square, bool isWhite) const;
    std::vector<BitMove> generateValidMoves(int square) const;
    int squareToIndex(int x, int y) const { return y * 8 + x; }
    void indexToSquare(int index, int& x, int& y) const { x = index % 8; y = index / 8; }

    Grid* _grid;
    
    // Bitboards for each piece type and color
    BitboardElement _whitePawns;
    BitboardElement _whiteKnights;
    BitboardElement _whiteKing;
    BitboardElement _blackPawns;
    BitboardElement _blackKnights;
    BitboardElement _blackKing;
    BitboardElement _allWhitePieces;
    BitboardElement _allBlackPieces;
    BitboardElement _allPieces;
    
    // Precomputed move tables
    BitboardElement _knightMoves[64];
    BitboardElement _kingMoves[64];
};