# Tic-Tac-Toe

A strategic twist on classic Tic Tac Toe: **Tic Tac Bomb**.

## Rules Implemented

- 3×3 grid with classic 3-in-a-row win condition.
- Move expiration system (marks expire after configurable turns).
- Move points earned when placing marks.
- Abilities:
  - **Bomb**: destroy a marked cell.
  - **Shield**: protect your mark from bomb/steal.
  - **Heal**: restore your expired or destroyed mark.
  - **Steal**: convert an opponent mark into your own.

## Python API

Core implementation lives in:

- `tic_tac_bomb.py`

Run tests:

```bash
python -m unittest discover -s tests -v
```
