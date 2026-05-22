import unittest

from tic_tac_bomb import TicTacBomb


class TicTacBombTests(unittest.TestCase):
    def test_placing_marks_awards_move_points(self):
        game = TicTacBomb()

        game.place_mark(0, 0)  # X
        game.place_mark(1, 1)  # O

        self.assertEqual(game.move_points["X"], 1)
        self.assertEqual(game.move_points["O"], 1)

    def test_marks_expire_and_can_be_healed(self):
        game = TicTacBomb(expiration_turns=2, ability_costs={"heal": 1})

        game.place_mark(0, 0)  # X
        game.place_mark(1, 1)  # O

        game.use_heal(0, 0)

        self.assertEqual(game.snapshot()[0][0], "X")

    def test_bomb_destroys_mark(self):
        game = TicTacBomb(ability_costs={"bomb": 1})

        game.place_mark(1, 1)  # X
        game.place_mark(0, 0)  # O
        game.use_bomb(0, 0)

        self.assertIsNone(game.snapshot()[0][0])

    def test_shield_blocks_bomb_and_steal(self):
        game = TicTacBomb(ability_costs={"shield": 1, "bomb": 0, "steal": 0})

        game.place_mark(0, 0)  # X
        game.place_mark(1, 1)  # O
        game.use_shield(0, 0)

        with self.assertRaises(ValueError):
            game.use_bomb(0, 0)

        with self.assertRaises(ValueError):
            game.use_steal(0, 0)

    def test_steal_converts_opponent_mark(self):
        game = TicTacBomb(ability_costs={"steal": 1})

        game.place_mark(0, 0)  # X
        game.place_mark(1, 1)  # O
        game.use_steal(1, 1)

        self.assertEqual(game.snapshot()[1][1], "X")

    def test_three_in_a_row_wins(self):
        game = TicTacBomb(expiration_turns=100)

        game.place_mark(0, 0)  # X
        game.place_mark(1, 0)  # O
        game.place_mark(0, 1)  # X
        game.place_mark(1, 1)  # O
        game.place_mark(0, 2)  # X

        self.assertEqual(game.get_winner(), "X")


if __name__ == "__main__":
    unittest.main()
