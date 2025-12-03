#pragma once
#include "Game.h"
#include "Grid.h"
#include "BitBoard.h"
#include <vector>
#include <cstdint>

constexpr int pieceSize = 80;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;
    bool gameHasAI() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    void clearBoardHighlights() override;
    void updateAI() override;
    void enableAIForColor(int playerNumber);
    void disableAI();
    void setPreferredAIColor(int playerNumber);
    bool isAIEnabled() const;
    int preferredAIColor() const;

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
    std::vector<BitMove> generateAllLegalMoves(bool forWhite);
    std::vector<BitMove> filterLegalMoves(const std::string& state,
                                          const std::vector<BitMove>& moves,
                                          bool isWhiteTurn) const;
    bool isKingInCheck(const std::string& state, bool whiteKing) const;
    bool isSquareAttacked(const std::string& state, int square, bool byWhite) const;

    // Helpers for negamax search on serialized board state
    std::vector<BitMove> generateAllLegalMovesFromState(const std::string& state,
                                                        bool isWhiteTurn) const;
    std::vector<BitMove> generatePawnMovesFromState(int square,
                                                    bool isWhite,
                                                    uint64_t allPieces,
                                                    uint64_t enemyPieces) const;
    std::vector<BitMove> generateAllMoves(const std::string& state, int playerColor) const;
    std::string applyMoveToState(const std::string& state, const BitMove& move) const;
    int evaluateBoard(const std::string& state) const;

    int negamax(std::string& state,
                int depth,
                int alpha,
                int beta,
                int playerColor);
    int squareToIndex(int x, int y) const { return y * 8 + x; }
    void indexToSquare(int index, int& x, int& y) const { x = index % 8; y = index / 8; }

    Grid* _grid;
    
    // Bitboards for each piece type and color
    BitboardElement _whitePawns;
    BitboardElement _whiteKnights;
    BitboardElement _whiteBishops;
    BitboardElement _whiteRooks;
    BitboardElement _whiteQueens;
    BitboardElement _whiteKing;
    BitboardElement _blackPawns;
    BitboardElement _blackKnights;
    BitboardElement _blackBishops;
    BitboardElement _blackRooks;
    BitboardElement _blackQueens;
    BitboardElement _blackKing;
    BitboardElement _allWhitePieces;
    BitboardElement _allBlackPieces;
    BitboardElement _allPieces;
    
    // Precomputed move tables
    BitboardElement _knightMoves[64];
    BitboardElement _kingMoves[64];
    
    // For tracking highlighted squares
    std::vector<ChessSquare*> _highlightedSquares;
        int _countMoves;
    int _preferredAIColor;
};