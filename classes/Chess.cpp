#include "Chess.h"
#include "BitBoard.h"
#include "MagicBitboards.h"
#include <limits>
#include <cmath>
#include <sstream>
#include <cctype>
#include <vector>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <algorithm>


namespace {
    constexpr int posInfinite = 1000000;
    constexpr int negInfinite = -posInfinite;
    constexpr int WhiteColor = 1;
    constexpr int BlackColor = -1;
    constexpr int defaultSearchDepth = 3;
}

Chess::Chess()
{
    _grid = new Grid(8, 8);
    _countMoves = 0;
    _preferredAIColor = 1; // default AI plays black unless user selects otherwise
    initMagicBitboards();
    generateKnightMoveBitboards();
    generateKingMoveBitboards();
    initializeBitboards();
}

Chess::~Chess()
{
    cleanupMagicBitboards();
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);
    int tag = static_cast<int>(piece) + (playerNumber == 1 ? 128 : 0);
    bit->setGameTag(tag);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

bool Chess::gameHasAI()
{
    return true;
}

void Chess::enableAIForColor(int playerNumber)
{
    if (playerNumber < 0 || playerNumber >= static_cast<int>(_players.size())) {
        return;
    }

    _preferredAIColor = playerNumber;
    for (Player* player : _players) {
        if (player) {
            player->setAIPlayer(false);
        }
    }

    _players.at(playerNumber)->setAIPlayer(true);
    _gameOptions.AIPlayer = playerNumber;
    _gameOptions.AIPlaying = true;
    _gameOptions.AIvsAI = false;
}

void Chess::disableAI()
{
    for (Player* player : _players) {
        if (player) {
            player->setAIPlayer(false);
        }
    }
    _gameOptions.AIPlayer = -1;
    _gameOptions.AIPlaying = false;
}

void Chess::setPreferredAIColor(int playerNumber)
{
    if (playerNumber < 0 || playerNumber >= static_cast<int>(_players.size())) {
        return;
    }
    _preferredAIColor = playerNumber;
    if (_gameOptions.AIPlaying) {
        enableAIForColor(playerNumber);
    }
}

bool Chess::isAIEnabled() const
{
    return _gameOptions.AIPlaying;
}

int Chess::preferredAIColor() const
{
    return _preferredAIColor;
}

void Chess::FENtoBoard(const std::string& fen) {
    _grid->forEachSquare([](ChessSquare* square, int, int) {
        if (square) square->destroyBit();
    });

    std::istringstream iss(fen);
    std::string placement, activeColor, castling, enpassant, halfmove, fullmove;
    iss >> placement >> activeColor >> castling >> enpassant >> halfmove >> fullmove;

    if (placement.empty())
        return;

    std::vector<std::string> ranks;
    {
        std::istringstream rankStream(placement);
        std::string rank;
        while (std::getline(rankStream, rank, '/'))
            ranks.push_back(rank);
    }

    if (ranks.size() != 8) {
        std::cerr << "Invalid FEN: expected 8 ranks, got " << ranks.size() << "\n";
        return;
    }

    for (size_t r = 0; r < 8; ++r) {
        const std::string& rankStr = ranks[r];
        int y = 7 - static_cast<int>(r);
        int x = 0;

        for (char c : rankStr) {
            if (std::isdigit(static_cast<unsigned char>(c))) {
                x += c - '0';
                continue;
            }

            if (x >= 8) break;

            bool isWhite = std::isupper(static_cast<unsigned char>(c));
            char lower = std::tolower(static_cast<unsigned char>(c));

            ChessPiece piece = NoPiece;
            switch (lower) {
                case 'p': piece = Pawn; break;
                case 'n': piece = Knight; break;
                case 'b': piece = Bishop; break;
                case 'r': piece = Rook; break;
                case 'q': piece = Queen; break;
                case 'k': piece = King; break;
                default:  piece = NoPiece; break;
            }

            if (piece != NoPiece) {
                Bit* bit = PieceForPlayer(isWhite ? 0 : 1, piece);
                if (ChessSquare* square = _grid->getSquare(x, y)) {
                    square->dropBitAtPoint(bit, square->getPosition());
                }
            }
            ++x;
        }
    }

    updateBitboards();
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    
    if (pieceColor != currentPlayer) return false;
    
    // Clear any previous highlights
    clearBoardHighlights();
    
    // Update bitboards and generate valid moves
    updateBitboards();
    
    ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
    if (!srcSquare) return true;
    
    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();
    int srcIndex = squareToIndex(srcX, srcY);
    
    // Generate all valid moves for this piece
    std::vector<BitMove> validMoves = generateValidMoves(srcIndex);
    
    // Highlight all valid destination squares
    for (const auto& move : validMoves) {
        int destX, destY;
        indexToSquare(move.to, destX, destY);
        ChessSquare* destSquare = _grid->getSquare(destX, destY);
        if (destSquare) {
            destSquare->setHighlighted(true);
            _highlightedSquares.push_back(destSquare);
        }
    }
    
    return true;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    // Clear highlights since we're attempting a move
    clearBoardHighlights();
    
    updateBitboards();
    ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);
    if (!srcSquare || !dstSquare) return false;
    
    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();
    int dstX = dstSquare->getColumn();
    int dstY = dstSquare->getRow();
    int srcIndex = squareToIndex(srcX, srcY);
    int dstIndex = squareToIndex(dstX, dstY);
    
    std::vector<BitMove> validMoves = generateValidMoves(srcIndex);
    
    // Check if this is a valid move
    for (const BitMove& move : validMoves) {
        if (move.to == dstIndex) {
            // Valid move; actual capture/removal happens when the engine finalizes the move
            return true;
        }
    }
    
    return false;
}

void Chess::clearBoardHighlights()
{
    for (ChessSquare* square : _highlightedSquares) {
        if (square) {
            square->setHighlighted(false);
        }
    }
    _highlightedSquares.clear();
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

void Chess::initializeBitboards()
{
    _whitePawns.setData(0ULL);
    _whiteKnights.setData(0ULL);
    _whiteBishops.setData(0ULL);
    _whiteRooks.setData(0ULL);
    _whiteQueens.setData(0ULL);
    _whiteKing.setData(0ULL);
    _blackPawns.setData(0ULL);
    _blackKnights.setData(0ULL);
    _blackBishops.setData(0ULL);
    _blackRooks.setData(0ULL);
    _blackQueens.setData(0ULL);
    _blackKing.setData(0ULL);
    _allWhitePieces.setData(0ULL);
    _allBlackPieces.setData(0ULL);
    _allPieces.setData(0ULL);
}

void Chess::updateBitboards()
{
    initializeBitboards();
    
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* bit = square->bit();
        if (!bit) return;
        
        int index = squareToIndex(x, y);
        uint64_t mask = 1ULL << index;
        
        bool isWhite = (bit->gameTag() < 128);
        ChessPiece piece = static_cast<ChessPiece>(bit->gameTag() % 128);
        
        if (isWhite) {
            _allWhitePieces |= mask;
            switch (piece) {
                case Pawn: _whitePawns |= mask; break;
                case Knight: _whiteKnights |= mask; break;
                case Bishop: _whiteBishops |= mask; break;
                case Rook: _whiteRooks |= mask; break;
                case Queen: _whiteQueens |= mask; break;
                case King: _whiteKing |= mask; break;
                default: break;
            }
        } else {
            _allBlackPieces |= mask;
            switch (piece) {
                case Pawn: _blackPawns |= mask; break;
                case Knight: _blackKnights |= mask; break;
                case Bishop: _blackBishops |= mask; break;
                case Rook: _blackRooks |= mask; break;
                case Queen: _blackQueens |= mask; break;
                case King: _blackKing |= mask; break;
                default: break;
            }
        }
    });
    
    _allPieces.setData(_allWhitePieces.getData() | _allBlackPieces.getData());
}

void Chess::generateKnightMoveBitboards()
{
    for (int square = 0; square < 64; ++square) {
        _knightMoves[square] = generateKnightMoves(square);
    }
}

void Chess::generateKingMoveBitboards()
{
    for (int square = 0; square < 64; ++square) {
        _kingMoves[square] = generateKingMoves(square);
    }
}

BitboardElement Chess::generateKnightMoves(int square) const
{
    uint64_t moves = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    
    int knightOffsets[8][2] = {
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };
    
    for (int i = 0; i < 8; ++i) {
        int newFile = file + knightOffsets[i][0];
        int newRank = rank + knightOffsets[i][1];
        
        if (newFile >= 0 && newFile < 8 && newRank >= 0 && newRank < 8) {
            int targetSquare = newRank * 8 + newFile;
            moves |= (1ULL << targetSquare);
        }
    }
    
    return BitboardElement(moves);
}

BitboardElement Chess::generateKingMoves(int square) const
{
    uint64_t moves = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    
    int kingOffsets[8][2] = {
        {0, 1}, {0, -1}, {1, 0}, {-1, 0},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    
    for (int i = 0; i < 8; ++i) {
        int newFile = file + kingOffsets[i][0];
        int newRank = rank + kingOffsets[i][1];
        
        if (newFile >= 0 && newFile < 8 && newRank >= 0 && newRank < 8) {
            int targetSquare = newRank * 8 + newFile;
            moves |= (1ULL << targetSquare);
        }
    }
    
    return BitboardElement(moves);
}

std::vector<BitMove> Chess::generatePawnMoves(int square, bool isWhite) const
{
    std::vector<BitMove> moves;
    int rank = square / 8;
    int file = square % 8;
    
    int direction = isWhite ? 1 : -1;
    int startRank = isWhite ? 1 : 6;
    
    int targetRank = rank + direction;
    if (targetRank >= 0 && targetRank < 8) {
        int targetSquare = targetRank * 8 + file;
        uint64_t targetMask = 1ULL << targetSquare;
        
        if ((_allPieces.getData() & targetMask) == 0) {
            moves.emplace_back(square, targetSquare, Pawn);
            
            if (rank == startRank) {
                int doublePushRank = rank + 2 * direction;
                int doublePushSquare = doublePushRank * 8 + file;
                uint64_t doublePushMask = 1ULL << doublePushSquare;
                
                if ((_allPieces.getData() & doublePushMask) == 0) {
                    moves.emplace_back(square, doublePushSquare, Pawn);
                }
            }
        }
    }
    
    for (int captureFile : {file - 1, file + 1}) {
        if (captureFile >= 0 && captureFile < 8) {
            int captureRank = rank + direction;
            if (captureRank >= 0 && captureRank < 8) {
                int captureSquare = captureRank * 8 + captureFile;
                uint64_t captureMask = 1ULL << captureSquare;
                
                uint64_t opponentPieces = isWhite ? _allBlackPieces.getData() : _allWhitePieces.getData();
                if (opponentPieces & captureMask) {
                    moves.emplace_back(square, captureSquare, Pawn);
                }
            }
        }
    }
    
    return moves;
}

std::vector<BitMove> Chess::generateValidMoves(int square) const
{
    std::vector<BitMove> moves;
    
    int x, y;
    indexToSquare(square, x, y);
    ChessSquare* sq = _grid->getSquare(x, y);
    if (!sq || !sq->bit()) return moves;
    
    Bit* bit = sq->bit();
    bool isWhite = (bit->gameTag() < 128);
    ChessPiece piece = static_cast<ChessPiece>(bit->gameTag() % 128);
    
    uint64_t friendlyPieces = isWhite ? _allWhitePieces.getData() : _allBlackPieces.getData();
    uint64_t opponentPieces = isWhite ? _allBlackPieces.getData() : _allWhitePieces.getData();
    
    switch (piece) {
        case Knight: {
            BitboardElement validMoves(_knightMoves[square].getData() & ~friendlyPieces);
            validMoves.forEachBit([&](int targetSquare) {
                moves.emplace_back(square, targetSquare, Knight);
            });
            break;
        }
        
        case King: {
            BitboardElement validMoves(_kingMoves[square].getData() & ~friendlyPieces);
            validMoves.forEachBit([&](int targetSquare) {
                moves.emplace_back(square, targetSquare, King);
            });
            break;
        }
        
        case Pawn: {
            moves = generatePawnMoves(square, isWhite);
            break;
        }
        
        case Rook: {
            uint64_t rookMoves = getRookAttacks(square, _allPieces.getData()) & ~friendlyPieces;
            BitboardElement validMoves(rookMoves);
            validMoves.forEachBit([&](int targetSquare) {
                moves.emplace_back(square, targetSquare, Rook);
            });
            break;
        }
        
        case Bishop: {
            uint64_t bishopMoves = getBishopAttacks(square, _allPieces.getData()) & ~friendlyPieces;
            BitboardElement validMoves(bishopMoves);
            validMoves.forEachBit([&](int targetSquare) {
                moves.emplace_back(square, targetSquare, Bishop);
            });
            break;
        }
        
        case Queen: {
            uint64_t queenMoves = getQueenAttacks(square, _allPieces.getData()) & ~friendlyPieces;
            BitboardElement validMoves(queenMoves);
            validMoves.forEachBit([&](int targetSquare) {
                moves.emplace_back(square, targetSquare, Queen);
            });
            break;
        }
        
        default:
            break;
    }
    
    return moves;
}

void Chess::updateAI()
{
    updateBitboards();

    const int playerColor = (getCurrentPlayer()->playerNumber() == 0) ? WhiteColor : BlackColor;
    std::string state = stateString();

    std::vector<BitMove> moves = generateAllMoves(state, playerColor);
    if (moves.empty()) {
        return;
    }

    const auto searchStart = std::chrono::steady_clock::now();
    _countMoves = 0;

    BitMove bestMove;
    int bestVal = negInfinite;

    for (const BitMove& move : moves) {
        char boardSave = state[move.to];
        char pieceMoving = state[move.from];

        state[move.to] = pieceMoving;
        state[move.from] = '0';

        int moveVal = -negamax(state, defaultSearchDepth - 1, negInfinite, posInfinite, -playerColor);

        state[move.from] = pieceMoving;
        state[move.to] = boardSave;

        if (moveVal > bestVal) {
            bestVal = moveVal;
            bestMove = move;
        }
    }

    if (bestVal == negInfinite) {
        return;
    }

    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - searchStart).count();
    const double nodesPerSecond = seconds > 0.0 ? static_cast<double>(_countMoves) / seconds : 0.0;
    std::cout << "Negamax depth " << defaultSearchDepth
              << " score " << bestVal
              << " nodes " << _countMoves
              << " (" << std::fixed << std::setprecision(2) << nodesPerSecond
              << " nodes/s)" << std::defaultfloat << std::endl;

    int srcX = bestMove.from % 8;
    int srcY = bestMove.from / 8;
    int dstX = bestMove.to % 8;
    int dstY = bestMove.to / 8;

    ChessSquare* srcSquare = _grid->getSquare(srcX, srcY);
    ChessSquare* dstSquare = _grid->getSquare(dstX, dstY);
    if (!srcSquare || !dstSquare) {
        return;
    }

    Bit* bit = srcSquare->bit();
    if (!bit) {
        return;
    }

    if (dstSquare->bit()) {
        dstSquare->destroyBit();
    }

    dstSquare->dropBitAtPoint(bit, dstSquare->getPosition());
    srcSquare->setBit(nullptr);
    bitMovedFromTo(*bit, *srcSquare, *dstSquare);
}

std::vector<BitMove> Chess::generateAllLegalMoves(bool forWhite)
{
    std::vector<BitMove> moves;
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        if (!square) return;
        Bit* bit = square->bit();
        if (!bit) return;
        bool isWhitePiece = bit->gameTag() < 128;
        if (isWhitePiece != forWhite) return;
        int index = squareToIndex(x, y);
        auto pieceMoves = generateValidMoves(index);
        moves.insert(moves.end(), pieceMoves.begin(), pieceMoves.end());
    });
    return moves;
}

std::vector<BitMove> Chess::generatePawnMovesFromState(int square,
                                                       bool isWhite,
                                                       uint64_t allPieces,
                                                       uint64_t enemyPieces) const
{
    std::vector<BitMove> moves;

    int rank = square / 8;
    int file = square % 8;
    int direction = isWhite ? 1 : -1;
    int startRank = isWhite ? 1 : 6;

    int forwardRank = rank + direction;
    if (forwardRank >= 0 && forwardRank < 8) {
        int oneStepSquare = forwardRank * 8 + file;
        uint64_t oneStepMask = 1ULL << oneStepSquare;
        if ((allPieces & oneStepMask) == 0) {
            moves.emplace_back(square, oneStepSquare, Pawn);

            if (rank == startRank) {
                int doubleRank = rank + 2 * direction;
                if (doubleRank >= 0 && doubleRank < 8) {
                    int doubleSquare = doubleRank * 8 + file;
                    uint64_t doubleMask = 1ULL << doubleSquare;
                    if ((allPieces & doubleMask) == 0) {
                        moves.emplace_back(square, doubleSquare, Pawn);
                    }
                }
            }
        }
    }

    for (int fileOffset : {-1, 1}) {
        int captureFile = file + fileOffset;
        int captureRank = rank + direction;
        if (captureFile < 0 || captureFile >= 8 || captureRank < 0 || captureRank >= 8) {
            continue;
        }

        int captureSquare = captureRank * 8 + captureFile;
        uint64_t captureMask = 1ULL << captureSquare;
        if (enemyPieces & captureMask) {
            moves.emplace_back(square, captureSquare, Pawn);
        }
    }

    return moves;
}

std::vector<BitMove> Chess::generateAllLegalMovesFromState(const std::string& state,
                                                           bool isWhiteTurn) const
{
    std::vector<BitMove> moves;
    if (state.size() < 64) {
        return moves;
    }

    uint64_t whitePieces = 0ULL;
    uint64_t blackPieces = 0ULL;

    for (int square = 0; square < 64; ++square) {
        char c = state[square];
        if (c == '0') {
            continue;
        }

        bool isWhitePiece = std::isupper(static_cast<unsigned char>(c));
        uint64_t mask = 1ULL << square;
        if (isWhitePiece) {
            whitePieces |= mask;
        } else {
            blackPieces |= mask;
        }
    }

    uint64_t allPieces = whitePieces | blackPieces;
    uint64_t friendlyPieces = isWhiteTurn ? whitePieces : blackPieces;
    uint64_t enemyPieces = isWhiteTurn ? blackPieces : whitePieces;

    for (int square = 0; square < 64; ++square) {
        char c = state[square];
        if (c == '0') {
            continue;
        }

        bool isWhitePiece = std::isupper(static_cast<unsigned char>(c));
        if (isWhitePiece != isWhiteTurn) {
            continue;
        }

        ChessPiece piece = NoPiece;
        switch (std::tolower(static_cast<unsigned char>(c))) {
            case 'p': piece = Pawn; break;
            case 'n': piece = Knight; break;
            case 'b': piece = Bishop; break;
            case 'r': piece = Rook; break;
            case 'q': piece = Queen; break;
            case 'k': piece = King; break;
            default: break;
        }

        if (piece == NoPiece) {
            continue;
        }

        switch (piece) {
            case Pawn: {
                auto pawnMoves = generatePawnMovesFromState(square, isWhiteTurn, allPieces, enemyPieces);
                moves.insert(moves.end(), pawnMoves.begin(), pawnMoves.end());
                break;
            }
            case Knight: {
                uint64_t knightMoves = _knightMoves[square].getData() & ~friendlyPieces;
                BitboardElement validMoves(knightMoves);
                validMoves.forEachBit([&](int targetSquare) {
                    moves.emplace_back(square, targetSquare, Knight);
                });
                break;
            }
            case King: {
                uint64_t kingMoves = _kingMoves[square].getData() & ~friendlyPieces;
                BitboardElement validMoves(kingMoves);
                validMoves.forEachBit([&](int targetSquare) {
                    moves.emplace_back(square, targetSquare, King);
                });
                break;
            }
            case Rook: {
                uint64_t rookMoves = getRookAttacks(square, allPieces) & ~friendlyPieces;
                BitboardElement validMoves(rookMoves);
                validMoves.forEachBit([&](int targetSquare) {
                    moves.emplace_back(square, targetSquare, Rook);
                });
                break;
            }
            case Bishop: {
                uint64_t bishopMoves = getBishopAttacks(square, allPieces) & ~friendlyPieces;
                BitboardElement validMoves(bishopMoves);
                validMoves.forEachBit([&](int targetSquare) {
                    moves.emplace_back(square, targetSquare, Bishop);
                });
                break;
            }
            case Queen: {
                uint64_t queenMoves = getQueenAttacks(square, allPieces) & ~friendlyPieces;
                BitboardElement validMoves(queenMoves);
                validMoves.forEachBit([&](int targetSquare) {
                    moves.emplace_back(square, targetSquare, Queen);
                });
                break;
            }
            default:
                break;
        }
    }

    return moves;
}

std::vector<BitMove> Chess::generateAllMoves(const std::string& state, int playerColor) const
{
    bool isWhiteTurn = (playerColor == WhiteColor);
    return generateAllLegalMovesFromState(state, isWhiteTurn);
}

std::string Chess::applyMoveToState(const std::string& state, const BitMove& move) const
{
    if (state.size() < 64) {
        return state;
    }

    std::string nextState = state;
    if (move.from >= nextState.size() || move.to >= nextState.size()) {
        return nextState;
    }

    nextState[move.to] = nextState[move.from];
    nextState[move.from] = '0';
    return nextState;
}

int Chess::evaluateBoard(const std::string& state) const
{
    static const std::map<char, int> evaluateScores = {
        {'P', 100},  {'p', -100},
        {'N', 200},  {'n', -200},
        {'B', 230},  {'b', -230},
        {'R', 400},  {'r', -400},
        {'Q', 900},  {'q', -900},
        {'K', 2000}, {'k', -2000},
        {'0', 0}
    };

    int value = 0;
    for (char ch : state) {
        auto it = evaluateScores.find(ch);
        if (it != evaluateScores.end()) {
            value += it->second;
        }
    }
    return value;
}

int Chess::negamax(std::string& state, int depth, int alpha, int beta, int playerColor)
{
    _countMoves++;

    if (depth == 0) {
        return evaluateBoard(state) * playerColor;
    }

    std::vector<BitMove> newMoves = generateAllMoves(state, playerColor);
    if (newMoves.empty()) {
        return evaluateBoard(state) * playerColor;
    }

    int bestVal = negInfinite;

    for (const BitMove& move : newMoves) {
        char boardSave = state[move.to];
        char pieceMoving = state[move.from];

        state[move.to] = pieceMoving;
        state[move.from] = '0';

        int score = -negamax(state, depth - 1, -beta, -alpha, -playerColor);

        state[move.from] = pieceMoving;
        state[move.to] = boardSave;

        bestVal = std::max(bestVal, score);
        alpha = std::max(alpha, bestVal);
        if (alpha >= beta) {
            break;
        }
    }

    return bestVal;
}

