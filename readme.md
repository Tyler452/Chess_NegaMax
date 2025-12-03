# Chess Game

## What's Implemented

- **FEN String Setup** - Board initializes from FEN notation
- **Pawns** - Move forward 1 or 2 squares from start, capture diagonally
- **Knights** - L-shaped moves, can jump over pieces
- **King** - Moves one square in any direction
- **Captures** - Pieces can take opponent pieces
- **Turn-Based** - Players alternate turns
- **Full Bitboard Move Gen** - Bishops, rooks, and queens use magic bitboards for sliding moves
- **Negamax AI** - Alpha-beta pruned search (depth 5 in release, >=3 guaranteed) with material evaluation

## Chess AI Summary

- **Search Depth:** The AI currently searches 5 plies deep (configurable, minimum target depth of 3) using negamax with alpha/beta pruning. On Windows Release builds it comfortably evaluates ~10^7 nodes per turn.
- **Evaluation:** Using the simple numbers(P=100, N=200, B=230, R=400, Q=900, K=2000). Positive scores favor White; the score is multiplied by the side to move during negamax.
- **Color Support:** By default the AI plays as Black, but the UI toggle allows either color.
- **Strength:** With depth 5 and pruning, it avoids blunders, captures loose pieces, and will beat casual players in the middlegame. Without positional heuristics it can still be outplayed strategically.
- **Challenges:** One challeneg was getting the negamax to work as intended. Another Challenge was getting the legal moves.

- ## Video
  https://github.com/user-attachments/assets/d80c8ba1-bacc-4d2b-9055-8de940a5f848

