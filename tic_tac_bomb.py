from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple


Coordinate = Tuple[int, int]


@dataclass
class Cell:
    owner: Optional[str] = None
    shielded: bool = False
    placed_turn: int = -1


class TicTacBomb:
    ABILITY_COSTS = {
        "bomb": 2,
        "shield": 1,
        "heal": 2,
        "steal": 3,
    }

    def __init__(
        self,
        size: int = 3,
        expiration_turns: int = 4,
        ability_costs: Optional[Dict[str, int]] = None,
    ) -> None:
        self.size = size
        self.expiration_turns = expiration_turns
        self.board: List[List[Cell]] = [
            [Cell() for _ in range(size)] for _ in range(size)
        ]
        self.players = ("X", "O")
        self.current_player_index = 0
        self.turn_count = 0
        self.move_points = {player: 0 for player in self.players}
        self.recoverable: Dict[str, List[Coordinate]] = {
            player: [] for player in self.players
        }
        self.ability_costs = {**self.ABILITY_COSTS, **(ability_costs or {})}

    @property
    def current_player(self) -> str:
        return self.players[self.current_player_index]

    def place_mark(self, row: int, col: int) -> None:
        self._expire_marks()
        cell = self._cell_at(row, col)
        if cell.owner is not None:
            raise ValueError("Cell is already occupied")

        player = self.current_player
        self.board[row][col] = Cell(owner=player, placed_turn=self.turn_count)
        self.move_points[player] += 1
        self._advance_turn()

    def use_bomb(self, row: int, col: int) -> None:
        self._expire_marks()
        self._spend_points("bomb")

        cell = self._cell_at(row, col)
        if cell.owner is None:
            raise ValueError("Cannot bomb an empty cell")
        if cell.shielded:
            raise ValueError("Cell is shielded")

        self.recoverable[cell.owner].append((row, col))
        self.board[row][col] = Cell()
        self._advance_turn()

    def use_shield(self, row: int, col: int) -> None:
        self._expire_marks()
        self._spend_points("shield")

        cell = self._cell_at(row, col)
        if cell.owner != self.current_player:
            raise ValueError("Shield can only be used on your own mark")
        cell.shielded = True
        self._advance_turn()

    def use_heal(self, row: int, col: int) -> None:
        self._expire_marks()
        self._spend_points("heal")

        cell = self._cell_at(row, col)
        if cell.owner is not None:
            raise ValueError("Cannot heal onto an occupied cell")

        player = self.current_player
        if (row, col) not in self.recoverable[player]:
            raise ValueError("No recoverable mark for this player at this cell")

        self.recoverable[player].remove((row, col))
        self.board[row][col] = Cell(owner=player, placed_turn=self.turn_count)
        self._advance_turn()

    def use_steal(self, row: int, col: int) -> None:
        self._expire_marks()
        self._spend_points("steal")

        cell = self._cell_at(row, col)
        if cell.owner is None:
            raise ValueError("Cannot steal an empty cell")
        if cell.owner == self.current_player:
            raise ValueError("Cannot steal your own mark")
        if cell.shielded:
            raise ValueError("Cell is shielded")

        self.board[row][col] = Cell(
            owner=self.current_player,
            placed_turn=self.turn_count,
        )
        self._advance_turn()

    def get_winner(self) -> Optional[str]:
        lines = []
        lines.extend(self.board)
        lines.extend(
            [
                [self.board[row][col] for row in range(self.size)]
                for col in range(self.size)
            ]
        )
        lines.append([self.board[i][i] for i in range(self.size)])
        lines.append([
            self.board[i][self.size - 1 - i] for i in range(self.size)
        ])

        for line in lines:
            owners = [cell.owner for cell in line]
            if owners[0] is not None and all(owner == owners[0] for owner in owners):
                return owners[0]
        return None

    def snapshot(self) -> List[List[Optional[str]]]:
        return [[cell.owner for cell in row] for row in self.board]

    def _expire_marks(self) -> None:
        for row in range(self.size):
            for col in range(self.size):
                cell = self.board[row][col]
                if (
                    cell.owner is not None
                    and self.turn_count - cell.placed_turn >= self.expiration_turns
                ):
                    self.recoverable[cell.owner].append((row, col))
                    self.board[row][col] = Cell()

    def _spend_points(self, ability: str) -> None:
        player = self.current_player
        cost = self.ability_costs[ability]
        if self.move_points[player] < cost:
            raise ValueError(f"Not enough points to use {ability}")
        self.move_points[player] -= cost

    def _advance_turn(self) -> None:
        self.turn_count += 1
        self.current_player_index = 1 - self.current_player_index

    def _cell_at(self, row: int, col: int) -> Cell:
        if not (0 <= row < self.size and 0 <= col < self.size):
            raise ValueError("Cell position out of bounds")
        return self.board[row][col]
