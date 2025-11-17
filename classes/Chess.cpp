#include "Chess.h"
#include "BitBoard.h"
#include <limits>
#include <cmath>
#include <sstream>
#include <cctype>
#include <vector>


Chess::Chess()
{
    _grid = new Grid(8, 8);
    generateKnightMoveBitboards();
    generateKingMoveBitboards();
    initializeBitboards();
}

Chess::~Chess()
{
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

    // --- Handle optional FEN fields if provided ---
    if (!activeColor.empty()) {

    }

    if (!castling.empty() && castling != "-") {

    }

    if (!enpassant.empty() && enpassant != "-") {

    }

    if (!halfmove.empty()) {
        
    }

    if (!fullmove.empty()) {

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
    return pieceColor == currentPlayer;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
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
    for (const BitMove& move : validMoves) {
        if (move.to == dstIndex) {
            Bit* dstBit = dstSquare->bit();
            if (dstBit) dstSquare->destroyBit();
            return true;
        }
    }
    return false;
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
    _whiteKing.setData(0ULL);
    _blackPawns.setData(0ULL);
    _blackKnights.setData(0ULL);
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
                case King: _whiteKing |= mask; break;
                default: break;
            }
        } else {
            _allBlackPieces |= mask;
            switch (piece) {
                case Pawn: _blackPawns |= mask; break;
                case Knight: _blackKnights |= mask; break;
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
        
        default:
            break;
    }
    
    return moves;
}
